#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdio.h>
#include <windows.h>

#include "fatctl/open.h"
#include "fatdir.h"

#include <assert.h>

namespace {

using namespace fatctl;

} // anonymous

int main(int argc, char** argv) {

    constexpr const char* kPath = "c:\\Rita"; // TODO use sysroot API
    int fd = _open(kPath, O_RDONLY | O_BINARY);
    printf("stock open(%s) -> fd=%d errno=%d LastError=%lu\n", kPath, fd, errno, GetLastError());
    assert((fd == -1) || errno);

    SetLastError(errno = 0);
    fd = open(kPath, O_RDONLY | O_BINARY);
    printf("wrapped open(%s) fd=%d errno=%d LastError=%lu\n", kPath, fd, errno, GetLastError());
    assert(fd >= 0);

    HANDLE dir = (HANDLE)_get_osfhandle(fd);
    std::string with_drive = PathWithDisk(dir);
    /* TODO: replace ASCII with UTF-8 */
    printf("fd to path: %s\n", with_drive.c_str());

    // supplement blksize
    FILE_STORAGE_INFO fsi;
    if(GetFileInformationByHandleEx(dir, FileStorageInfo, &fsi, sizeof(fsi))) {
        printf("blksize(emulated)=%lu blksize(fundamental)=%lu secsize=%lu\n",
            fsi.LogicalBytesPerSector,
            fsi.PhysicalBytesPerSectorForAtomicity,
            fsi.FileSystemEffectivePhysicalBytesPerSectorForAtomicity);
    }

    // close dirfd
    assert(!close(fd));

    return 0;
}