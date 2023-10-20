
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "fatctl/wrap.h"
#include "fatctl/open.h"
#include "fatctl/dirs.h"
#include "dir.h"

#include <windows.h>
#include <errno.h>
#include <filesystem>
#include <string>

#include "fatctl/mode.h"

#include "dbg.h"

/* Make sure no old code has been reused: */
#define _FATCTL_ENSURE_MODE_IS_NEW(newmode) \
        static_assert(newmode != S_IFREG); \
        static_assert(newmode != S_IFCHR); \
        static_assert(newmode != S_IFDIR); \
        static_assert(newmode != S_IFIFO);
    _FATCTL_ENSURE_MODE_IS_NEW(S_IFBLK)
    _FATCTL_ENSURE_MODE_IS_NEW(S_IFLNK)
    _FATCTL_ENSURE_MODE_IS_NEW(S_IFSOCK)
#undef _FATCTL_ENSURE_MODE_IS_NEW
/* Now ensure uniqueness among the newly added: */
static_assert(S_IFBLK != S_IFLNK);
static_assert(S_IFBLK != S_IFSOCK);
static_assert(S_IFLNK != S_IFSOCK);

namespace fatctl {

constexpr const char* kLPP = "\\\\?\\";
constexpr size_t kPSz = 4;

const char* TrimLPP(const char* str) {
    return strncmp(str, kLPP, kPSz) ? str : str + kPSz;
}

std::string PathWithDisk(HANDLE dir) {
    // reveal path name: using GetFinalPathNameByHandleA (reveals drive letter!)
    char lpath[MAX_PATH];
    size_t len = GetFinalPathNameByHandleA(dir, &lpath[0], MAX_PATH, 0 /* flags */);
    return len ? std::string(TrimLPP(lpath), len) : std::string();
}

} // namespace fatctl

namespace { namespace fs = std::__fs::filesystem; using namespace fatctl; }

extern "C"
{

int fatctl_allcatch_open(const char* path, int flags, ...) {
    const bool create = O_CREAT & flags;
    const bool isdir = O_DIRECTORY & flags;
    if(create & isdir) return errno = EINVAL, -1;
    if(create) { _FATCTL_GETMODE; return fatctl_fallback_openfm(path, flags, mode); }
    int fd = isdir ? -1 : fatctl_fallback_open(path, flags);
    _FATCTL_LOG("fallback fd=%d errno=%d LastError=%lu\n", fd, errno, GetLastError());
    if(fd<0) {
        // iterate over various file types, such as symlink, reparse point, volume etc.
        SetLastError(errno = 0);
        HANDLE dir = CreateFileA(path, GENERIC_READ, FILE_SHARE_VALID_FLAGS, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
        _FATCTL_LOG("Got a directory handle: %p -> %s\n", dir, path);
        if(dir != INVALID_HANDLE_VALUE) {
            fd = _open_osfhandle((intptr_t)dir, _O_RDONLY); // FIXME!!!!! support overriding -- see TODO in wrap.h
            _FATCTL_LOG("Got a dirfd: %d errno=%d LastError=%lu\n", fd, errno, GetLastError());
        }
    }
    return fd;
}

int link(const char *target, const char *linkpath) {
    // TODO add error codes, condition checks, etc.
    try {
        return fs::create_hard_link({target}, {linkpath}), 0;
    } catch(const fs::filesystem_error& fse) {
        return errno = EIO, -1;
    }
}

int symlink(const char *target, const char *linkpath) {
    // TODO add error codes, condition checks, etc.
    try {
        return fs::create_symlink({target}, {linkpath}), 0;
    } catch(const fs::filesystem_error& fse) {
        return errno = EIO, -1;
    }
}

ssize_t readlink(const char *pathname, char *buf, size_t bufsz) {
    fs::path source{pathname};
    if(fs::is_symlink(source)) try {
        fs::path target = fs::read_symlink(source);
        std::string trg = target.string();
        bufsz = std::min(bufsz, trg.size());
        std::strncpy(buf, trg.data(), bufsz);
        return bufsz;
    } catch(const fs::filesystem_error& fse) {
        return errno = EIO, -1;
    } else {
        return errno = EINVAL, -1;
    }
}

DIR* fdopendir(int fd) {
    std::string dirpath = PathWithDisk(get_handle_from_posix_fd(fd));
    return dirpath.empty() ? errno = EINVAL, nullptr : opendir(dirpath.c_str());
}

} // extern "C"
