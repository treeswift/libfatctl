
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "fatctl/wrap.h"
#include "fatctl/path.h"
#include "fatctl/open.h"
#include "fatctl/dirs.h"
#include "fatctl/stat.h"
#include "fatctl/perm.h"
#include "fatctl/what.h"

#include <windows.h>
#ifdef _FATCTL_USEKTM
#include <ktmw32.h> /* {Create|Commit}Transaction */
#endif
#include <errno.h>
#include <filesystem>
#include <functional>
#include <string>

#include "fatctl/mode.h"

#include "std.h"
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

namespace { using namespace fatctl; }

namespace fatctl {

static constexpr const char* kLPP = "\\\\?\\";
static constexpr size_t kPSz = 4;

static const char* TrimLPP(const char* str) {
    return strncmp(str, kLPP, kPSz) ? str : str + kPSz;
}

static std::string PathWithDisk(HANDLE hFile) {
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
std::string Fd2PathStr(int fd) {
    if(AT_FDCWD == fd) { return "."; }
    return PathWithDisk(get_handle_from_posix_fd(fd));
}

fs::path Fd2Path(int fd) {
    if(AT_FDCWD == fd) { return fs::current_path(); }
    return Fd2PathStr(fd); /* MOREINFO what is the locale in use here? */
}

fs::path ResolveRelativePath(int basefd, std::string relpath, int flags) {
    fs::path navpath = {relpath};
    if(navpath.is_absolute()) {
        _FATCTL_LOG("branch already absolute: %ls", navpath.c_str());
        return navpath;
    } else {
        fs::path abspath = Fd2Path(basefd) / navpath;
        _FATCTL_LOG("new absolute path: %ls", abspath.c_str());
        if(!(flags & AT_SYMLINK_NOFOLLOW)) {
            abspath = fs::weakly_canonical(abspath);
            _FATCTL_LOG("weakly canonical path: %ls", abspath.c_str());
            errno = 0;
        }
        return abspath;
    }
}

fs::path ResolveRelativeTargetPath(int basefd, std::string relpath, int flags) {
    if(relpath.empty() && (flags & AT_EMPTY_PATH)) {
        return Fd2Path(basefd);
    } else {
        return ResolveRelativePath(basefd, relpath, flags);
    }
}

fs::path ResolveRelativeLinkTargetPath(int basefd, std::string relpath, int flags) {
    if(!(flags & AT_SYMLINK_FOLLOW)) {
        flags |= AT_SYMLINK_NOFOLLOW;
    }
    return ResolveRelativeTargetPath(basefd, relpath, flags);
}

std::string Path2StdString(const fs::path& path) {
    // ROADMAP make locale configurable -- at least as much as MinGW/CRT expect (issue #4)
    return path.u8string();
}

std::string ResolveRelative(int basefd, std::string relpath, int flags) {
    return Path2StdString(ResolveRelativePath(basefd, relpath, flags));
}

std::string ResolveRelativeTarget(int basefd, std::string relpath, int flags) {
    return Path2StdString(ResolveRelativeTargetPath(basefd, relpath, flags));
}

std::string ResolveRelativeLinkTarget(int basefd, std::string relpath, int flags) {
    return Path2StdString(ResolveRelativeLinkTargetPath(basefd, relpath, flags));
}

int WithPathPair(int oldfd, std::string oldrelp, int newfd, std::string newrelp, int flags, 
                 std::function<int(const char*, const char*)> op, bool hardlinking = false) {
    try {
        auto tpath = hardlinking ? ResolveRelativeLinkTargetPath(oldfd, oldrelp, flags)
                                     : ResolveRelativeTargetPath(oldfd, oldrelp, flags);
        auto npath = ResolveRelativePath(newfd, newrelp, flags);
        const auto ctpath = Path2StdString(tpath);
        const auto cnpath = Path2StdString(npath);
        _FATCTL_LOG("old/trg=%s", ctpath.c_str());
        _FATCTL_LOG("new/lnk=%s", cnpath.c_str());
        return op(ctpath.c_str(), cnpath.c_str());
    } catch(const fs::filesystem_error& fse) {
        _FATCTL_LOG("Pair op error: %s", fse.what());
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
        if(dir == INVALID_HANDLE_VALUE) return errno = ENOENT, -1;
        fd = _open_osfhandle((intptr_t)dir, _O_RDONLY); // FIXME!!!!! support overriding -- see TODO in wrap.h
        _FATCTL_LOG("Got a dirfd: %d errno=%d LastError=%lu", fd, errno, GetLastError());
    }
    return fd;
}

int link(const char *target, const char *linkpath) {
    // TODO add error codes, condition checks, etc.
    try {
        return fs::create_hard_link({target}, {linkpath}), 0;
    } catch(const fs::filesystem_error& fse) {
        _FATCTL_LOG("hardlink error: %s", fse.what());
        return errno = EIO, -1;
    }
}

#ifdef _FATCTL_USEKTM
extern __declspec(dllimport) HANDLE CreateTransaction(LPSECURITY_ATTRIBUTES lpTransactionAttributes, LPGUID UOW,
        DWORD CreateOptions, DWORD IsolationLevel, DWORD IsolationFlags, DWORD Timeout, LPWSTR Description)
                __attribute__((weak));

extern __declspec(dllimport) BOOL CommitTransaction(HANDLE TransactionHandle)
                __attribute__((weak));
#endif

int symlink(const char *target, const char *linkpath) {
    // TODO add error codes, condition checks, etc.
    try {
        fs::path trgpath = {target};
        fs::path lnkpath = {linkpath};
        fs::path relpath = trgpath.lexically_relative(lnkpath.parent_path());
        if(relpath.is_relative() && !relpath.has_parent_path()) {
            relpath = fs::path{"."} / relpath;
        }
        // if(fs::is_directory(trgpath)) {
        //     fs::create_directory_symlink(relpath, lnkpath);
        // } else {
        //     fs::create_symlink(relpath, lnkpath);
        // }
        std::wstring relstr = relpath.native(), lnkstr = lnkpath.native();
        DWORD flags = fs::is_directory(trgpath) ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0;
        bool success = CreateSymbolicLinkW(lnkstr.c_str(), relstr.c_str(), flags | SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE);
        if(!success && ERROR_INVALID_PARAMETER == GetLastError()) {
            success = CreateSymbolicLinkW(lnkstr.c_str(), relstr.c_str(), flags);
        }
#ifdef _FATCTL_USEKTM
        if(!success && ERROR_PRIVILEGE_NOT_HELD == GetLastError()) {
            HMODULE ktmdll = LoadLibraryA("ktmw32");
            decltype(&CreateTransaction) create = (decltype(&CreateTransaction)) GetProcAddress(ktmdll, "CreateTransaction");
            decltype(&CommitTransaction) commit = (decltype(&CommitTransaction)) GetProcAddress(ktmdll, "CommitTransaction");
            HANDLE transaction = (*create)(nullptr, 0, 0, 0, 0, 5000, const_cast<wchar_t*>(lnkstr.data()));
            success = CreateSymbolicLinkTransactedW(lnkstr.c_str(), relstr.c_str(), flags, transaction) && (*commit)(transaction);
        }
#endif
        _FATCTL_LOG("symlink(%ls, %ls): LastError=%lu", relstr.c_str(), lnkstr.c_str(), GetLastError());
        return success ? 0 : ((errno = (GetLastError() == ERROR_PRIVILEGE_NOT_HELD) ? EPERM : EIO), -1);
    } catch(const fs::filesystem_error& fse) {
        _FATCTL_LOG("symlink: LastError=%lu (%s)", GetLastError(), fse.what());
        return errno = ENOENT, -1;
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
        _FATCTL_LOG("readlink error: %s", fse.what());
        return errno = EIO, -1;
    } else {
        return errno = EINVAL, -1;
    }
}

DIR* fdopendir(int dirfd) {
    std::string dirpath = Fd2PathStr(dirfd);
    return dirpath.empty() ? errno = EINVAL, nullptr : opendir(dirpath.c_str());
}

// TODO lstat

// "at" APIs

int openat(int dirfd, const char* relpath, int flags, ...) {
    auto path = ResolveRelativePath(dirfd, relpath, flags);
    auto locpath = Path2StdString(path);
    const char* cpath = locpath.c_str();
    const bool create = O_CREAT & flags;
    if(create) {
        _FATCTL_GETMODE;
        _FATCTL_LOG("create(%s, %x, 0%o)", cpath, flags, mode);
        return open(cpath, flags, mode);
    } else {
        _FATCTL_LOG("open(%s, %x)", cpath, flags);
        return open(cpath, flags);
    }
}

int mkdirat(int dirfd, const char* relpath, mode_t mode) {
    auto path = ResolveRelativePath(dirfd, relpath, 0);
    auto locpath = Path2StdString(path);
    const char* cpath = locpath.c_str();
    printf("pre-mkdir errno=%d\n", errno);
    const int mkdval = mkdir(cpath);
    _FATCTL_LOG("mkdir(%s)=%d errno=%d", cpath, mkdval, errno);
    _FATCTL_LOG("now: chmod(%s, 0%o)", cpath, mode);
    return mkdval < 0 ? mkdval : chmod(cpath, mode);
}

int renameat(int dirfd, const char *relpath, int newdirfd, const char *newrelpath) {
    return WithPathPair(dirfd, relpath, newdirfd, newrelpath, 0, &rename, false);
}

int unlinkat(int dirfd, const char* relpath, int flags) {
    auto path = ResolveRelativePath(dirfd, relpath, flags | AT_SYMLINK_NOFOLLOW);
    auto locpath = Path2StdString(path);
    return (flags & AT_REMOVEDIR) ? rmdir(locpath.c_str()) : unlink(locpath.c_str());
}

int linkat(int trgfd, const char *relpath, int linkdir, const char *linkrelpath, int flags) {
    return WithPathPair(trgfd, relpath, linkdir, linkrelpath, flags, &link, true);
}

int symlinkat(const char *target, int linkdir, const char* linkrelpath) {
    auto path = ResolveRelativePath(linkdir, linkrelpath, 0);
    auto locpath = Path2StdString(path);
    const char* linkpath = locpath.c_str();
    return symlink(target, linkpath);
}

ssize_t readlinkat(int dirfd, const char* relpath, char* buf, size_t bufsz) {
    auto path = ResolveRelativePath(dirfd, relpath, AT_SYMLINK_NOFOLLOW);
    auto locpath = Path2StdString(path);
    return readlink(locpath.c_str(), buf, bufsz);
}

int fstatat(int dirfd, const char *relpath, struct stat* statbuf, int flags) {
    auto path = ResolveRelativePath(dirfd, relpath, flags);
    auto locpath = Path2StdString(path);
    int filefd = open(locpath.c_str(), O_RDONLY);
    int retval = fstat(filefd, statbuf);
    return close(filefd), retval;
}

int faccessat(int dirfd, const char* relpath, int mode, int flags) {
    auto path = ResolveRelativePath(dirfd, relpath, flags);
    auto locpath = Path2StdString(path);
    return access(locpath.c_str(), mode);
}

int fchmod(int fd, mode_t mode) {
    auto path = Fd2PathStr(fd);
    return chmod(path.c_str(), mode);
}

int fchmodat(int dirfd, const char* relpath, mode_t mode, int flags) {
    auto path = ResolveRelativePath(dirfd, relpath, flags);
    auto locpath = Path2StdString(path);
    return chmod(locpath.c_str(), mode);
}

} // extern "C"
