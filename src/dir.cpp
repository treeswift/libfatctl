#include "fatctl/open.h"
#include "fatctl/dirs.h"
#include "fatdir.h"

#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <windows.h>
#include <errno.h>
#include <string>

namespace fatctl {

constexpr const char* kLPP = "\\\\?\\";
constexpr size_t kPSz = 4;

const char* TrimLPP(const char* str) {
    return strncmp(str, kLPP, kPSz) ? str : str + kPSz;
}

// std::wstring PathFromRoot(HANDLE dir) {
//     // reveal path name: using GetFileInformationByHandleEx
//     struct {
//         FILE_NAME_INFO fni;
//         wchar_t xtn[MAX_PATH];
//     } pc;
//     if(GetFileInformationByHandleEx(dir, FileNameInfo, &pc.fni, sizeof(pc))) {
//         size_t len = pc.fni.FileNameLength / sizeof(*pc.fni.FileName);
//         assert(len < MAX_PATH);
//         // pc.fni.FileName[len] = L'\0';
//         return std::wstring(pc.fni.FileName, pc.fni.FileName + len);
//     }
//     return {};
// }

std::string PathWithDisk(HANDLE dir) {
    // reveal path name: using GetFinalPathNameByHandleA (reveals drive letter)
    char lpath[MAX_PATH];
    size_t len = GetFinalPathNameByHandleA(dir, &lpath[0], MAX_PATH, 0 /* flags */);
    return len ? std::string(TrimLPP(lpath), len) : std::string();
}

} // namespace fatctl

namespace { using namespace fatctl; }

extern "C"
{

int fatctl_allcatch_open(const char* path, int mode, ...) {
    int fd = (mode & O_DIRECTORY) ? -1 : fatctl_fallback_open(path, mode); /* parse and forward args... */
    //LOGprintf("fallback fd=%d errno=%d LastError=%lu\n", fd, errno, GetLastError());
    if(fd<0) {
        // iterate over various possibilities...
        // TODO: parse `mode`
        SetLastError(errno = 0);
        HANDLE dir = CreateFileA(path, GENERIC_READ, FILE_SHARE_VALID_FLAGS, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
        //LOGprintf("Got a directory handle: %p -> %s\n", dir, path);
        if(dir != INVALID_HANDLE_VALUE) {
            fd = _open_osfhandle((intptr_t)dir, _O_RDONLY);
            //LOGprintf("Got a dirfd: %d errno=%d LastError=%lu\n", fd, errno, GetLastError());
        }
    }
    return fd;
}

DIR* fdopendir(int fd) {
    std::string dirpath = PathWithDisk((HANDLE)_get_osfhandle(fd)); // TODO replace with customizable lookup
    return dirpath.empty() ? errno = EINVAL, nullptr : opendir(dirpath.c_str());
}

} // extern "C"
