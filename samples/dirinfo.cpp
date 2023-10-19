#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdio.h>
#include <windows.h>

#include "fatctl/open.h"

#include <assert.h>
#include <string>

namespace fatctl {

constexpr const char* kLPP = "\\\\?\\";
constexpr size_t kPSz = 4;

const char* TrimLPP(const char* str) {
    return strncmp(str, kLPP, kPSz) ? str : str + kPSz;
}

} // namespace fatctl

namespace {

using namespace fatctl;

} // anonymous

extern "C"
{

int fatctl_allcatch_open(const char* path, int mode, ...) {
    int fd = (mode & O_DIRECTORY) ? -1 : fatctl_fallback_open(path, mode); /* parse and forward args... */
    printf("fallback fd=%d errno=%d LastError=%lu\n", fd, errno, GetLastError());
    if(fd<0) {
        // iterate over various possibilities...
        // examine whether stock `close()` works!
        // TODO: parse `mode`
        SetLastError(errno = 0);
        HANDLE dir = CreateFileA(path, GENERIC_READ, FILE_SHARE_VALID_FLAGS, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
        printf("Got a directory handle: %p -> %s\n", dir, path);
        if(dir != INVALID_HANDLE_VALUE) {
            fd = _open_osfhandle((intptr_t)dir, _O_RDONLY);
            printf("Got a dirfd: %d errno=%d LastError=%lu\n", fd, errno, GetLastError());
        }
    }
    return fd;
}

} // extern "C"

int main(int argc, char** argv) {

    constexpr const char* kPath = "c:\\Rita"; // TODO use sysroot API
    int fd = _open(kPath, O_RDONLY | O_BINARY);
    printf("stock open(%s) -> fd=%d errno=%d LastError=%lu\n", kPath, fd, errno, GetLastError());
    assert((fd == -1) || errno);

    SetLastError(errno = 0);
    fd = open(kPath, O_RDONLY | O_BINARY);
    printf("wrapped open(%s) fd=%d errno=%d LastError=%lu\n", kPath, fd, errno, GetLastError());

    // reveal path name: trying GetFileInformationByHandleEx
    struct {
        FILE_NAME_INFO fni;
        wchar_t xtn[MAX_PATH];
    } pc;
    HANDLE dir = (HANDLE)_get_osfhandle(fd);
    if(GetFileInformationByHandleEx(dir, FileNameInfo, &pc.fni, sizeof(pc))) {
        size_t len = pc.fni.FileNameLength / sizeof(*pc.fni.FileName);
        assert(len < MAX_PATH);
        pc.fni.FileName[len] = L'\0';
        printf("GetFileInformationByHandleEx(FileNameInfo): path=%ls\n", pc.fni.FileName);
        fflush(stdout);
    }
    // reveal path name: trying GetFinalPathNameByHandleA (reveals drive letter)
    char lpath[MAX_PATH];
    if(GetFinalPathNameByHandleA(dir, &lpath[0], MAX_PATH, 0 /* flags */)) {
        printf("GetFinalPathNameByHandle(VOLUME_NAME_DOS): path=%s\n", TrimLPP(lpath));
    }
    FILE_STORAGE_INFO fsi;
    if(GetFileInformationByHandleEx(dir, FileStorageInfo, &fsi, sizeof(fsi))) {
        printf("blksize(emulated)=%lu blksize(fundamental)=%lu secsize=%lu\n",
            fsi.LogicalBytesPerSector,
            fsi.PhysicalBytesPerSectorForAtomicity,
            fsi.FileSystemEffectivePhysicalBytesPerSectorForAtomicity);
    }

    assert(!close(fd));

    return 0;
}