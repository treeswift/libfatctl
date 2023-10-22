
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "fatctl/wrap.h"
#include "fatctl/path.h"
#include "fatctl/open.h"
#include "fatctl/dirs.h"
#include "fatctl/stat.h"

#include "dir.h" /* MOREINFO do we need these internal APIs declared? */

#include <windows.h>
#include <errno.h>
#include <filesystem>
#include <functional>
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

namespace { namespace fs = std::__fs::filesystem; using namespace fatctl; }

namespace fatctl {

constexpr const char* kLPP = "\\\\?\\";
constexpr size_t kPSz = 4;

const char* TrimLPP(const char* str) {
    return strncmp(str, kLPP, kPSz) ? str : str + kPSz;
}

std::string PathWithDisk(HANDLE hFile) {
    // reveal path name: using GetFinalPathNameByHandleA (reveals drive letter!)
    char lpath[MAX_PATH];
    size_t len = GetFinalPathNameByHandleA(hFile, &lpath[0], MAX_PATH, 0 /* flags */);
    _FATCTL_LOG("handle %p -> path: %s", hFile, lpath);
    return len ? lpath[len] = '\0', TrimLPP(lpath) : "";
}

/**
 * Note that the Windows open file ownership model is different from *nix
 * in such a way that it's impossible to rename or delete a file that is
 * currently opened, as well as any folder on the path leading to it from
 * the volume root or the nearest reparse point. Therefore, as long as an
 * `fd` is held open, the path to the item it represents stays valid, and
 * using numeric `fd` instead of the path string becomes a matter of mere
 * preference. We rely on the std::filesystem C++ API for path resolution.
 */
std::string PathFromFd(int fd) {
    if(AT_FDCWD == fd) { return "."; }
    return PathWithDisk(get_handle_from_posix_fd(fd));
}

fs::path FsPathFromFd(int fd) {
    if(AT_FDCWD == fd) { return fs::current_path(); }
    return PathFromFd(fd); /* MOREINFO what is the locale in use here? */
}

fs::path ResolveRelative(int basefd, std::string relpath, int flags) {
    fs::path navpath = {relpath};
    if(navpath.is_absolute()) {
        _FATCTL_LOG("branch already absolute: %ls", navpath.c_str());
        return navpath;
    } else {
        fs::path abspath = FsPathFromFd(basefd) / navpath;
        _FATCTL_LOG("new absolute path: %ls", abspath.c_str());
        if(flags & AT_SYMLINK_NOFOLLOW) {
            return abspath;
        } else {
            return fs::weakly_canonical(abspath);
        }
    }
}

fs::path ResolveRelativeTarget(int basefd, std::string relpath, int flags) {
    if(relpath.empty() && (flags & AT_EMPTY_PATH)) {
        return FsPathFromFd(basefd);
    } else {
        return ResolveRelative(basefd, relpath, flags);
    }
}

fs::path ResolveRelativeLinkTarget(int basefd, std::string relpath, int flags) {
    if(!(flags & AT_SYMLINK_FOLLOW)) {
        flags |= AT_SYMLINK_NOFOLLOW;
    }
    return ResolveRelativeTarget(basefd, relpath, flags);
}

int WithPathPair(int oldfd, std::string oldrelp, int newfd, std::string newrelp, int flags, 
                 std::function<int(const char*, const char*)> op, bool hardlinking = false) {
    try {
        auto tpath = hardlinking ? ResolveRelativeLinkTarget(oldfd, oldrelp, flags)
                                     : ResolveRelativeTarget(oldfd, oldrelp, flags);
        auto npath = ResolveRelative(newfd, newrelp, flags);
        return op(tpath.u8string().c_str(), npath.u8string().c_str());
    } catch(const fs::filesystem_error& fse) {
        return errno = ENOENT, -1;
    }
}

// The remaining constants to consider are AT_REMOVEDIR and AT_NO_AUTOMOUNT.

} // namespace fatctl

extern "C"
{

#ifndef open
#error "open" must be a macro at this point
#endif

int open(const char* path, int flags, ...) {
    const bool create = O_CREAT & flags;
    const bool isdir = O_DIRECTORY & flags;
    if(create & isdir) return errno = EINVAL, -1;
    // TODO: if(flags & O_NOFOLLOW) { test for symbolic link here and fail if the file is one }
    if(create) { _FATCTL_GETMODE; return fatctl_fallback_openfm(path, flags, mode); }
    int fd = isdir ? -1 : fatctl_fallback_openf(path, flags);
    _FATCTL_LOG("fallback fd=%d errno=%d LastError=%lu", fd, errno, GetLastError());
    if(fd<0) {
        // iterate over various file types, such as symlink, reparse point, volume etc.
        SetLastError(errno = 0);
        HANDLE dir = CreateFileA(path, GENERIC_READ, FILE_SHARE_VALID_FLAGS, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
        _FATCTL_LOG("Got a directory handle: %p -> %s", dir, path);
        if(dir != INVALID_HANDLE_VALUE) {
            fd = _open_osfhandle((intptr_t)dir, _O_RDONLY); // FIXME!!!!! support overriding -- see TODO in wrap.h
            _FATCTL_LOG("Got a dirfd: %d errno=%d LastError=%lu", fd, errno, GetLastError());
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

DIR* fdopendir(int dirfd) {
    std::string dirpath = PathFromFd(dirfd);
    return dirpath.empty() ? errno = EINVAL, nullptr : opendir(dirpath.c_str());
}

// TODO lstat

// "at" APIs

int openat(int dirfd, const char* relpath, int flags, ...) {
    auto path = ResolveRelative(dirfd, relpath, flags);
    auto locpath = path.u8string();
    const char* cpath = locpath.c_str();
    const bool create = O_CREAT & flags;
    if(create) {
        _FATCTL_GETMODE;
        return open(cpath, flags, mode);
    } else {
        return open(cpath, flags);
    }
}

int mkdirat(int dirfd, const char* relpath, mode_t mode) {
    auto path = ResolveRelative(dirfd, relpath, 0);
    auto locpath = path.u8string();
    const char* cpath = locpath.c_str();
    return mkdir(cpath) || chmod(cpath, mode);
}

int renameat(int dirfd, const char *relpath, int newdirfd, const char *newrelpath) {
    return WithPathPair(dirfd, relpath, newdirfd, newrelpath, 0, &rename, false);
}

int unlinkat(int dirfd, const char* relpath, int flags) {
    auto path = ResolveRelative(dirfd, relpath, flags | AT_SYMLINK_NOFOLLOW);
    return (flags & AT_REMOVEDIR) ? rmdir(path.u8string().c_str()) : unlink(path.u8string().c_str());
}

int linkat(int trgfd, const char *relpath, int linkdir, const char *linkrelpath, int flags) {
    return WithPathPair(trgfd, relpath, linkdir, linkrelpath, flags, &link, true);
}

int symlinkat(const char *target, int linkdir, const char* linkrelpath) {
    auto path = ResolveRelative(linkdir, linkrelpath, 0);
    auto locpath = path.u8string();
    const char* linkpath = locpath.c_str();
    return symlink(target, linkpath);
}

ssize_t readlinkat(int dirfd, const char* relpath, char* buf, size_t bufsz) {
    auto path = ResolveRelative(dirfd, relpath, AT_SYMLINK_NOFOLLOW);
    return readlink(path.u8string().c_str(), buf, bufsz);
}

int fstatat(int dirfd, const char *relpath, struct stat* statbuf, int flags) {
    auto path = ResolveRelative(dirfd, relpath, flags);
    auto locpath = path.u8string();
    int filefd = open(locpath.c_str(), O_RDONLY);
    int retval = fstat(filefd, statbuf);
    return close(filefd), retval;
}

} // extern "C"
