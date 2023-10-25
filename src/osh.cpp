#include "fatctl/wrap.h"
#include "fatctl/mode.h"
#include "fatctl/cntl.h"

#include <windows.h>
#ifdef _FATCTL_NTQUERYOBJECT
#include <winternl.h>
#endif
#include <io.h>

#include "dbg.h"

namespace fatctl {

// Global data. No thread safety here at all. AT ALL. Period.
// All custom logic here must be confined to program startup.

/* Default implementation. */
HANDLE GetOSFHandle(int fd) { return (HANDLE)_get_osfhandle(fd); }
handle_from_posix_fd_func const kDefFd2HandleFunc = &GetOSFHandle;

/* Global state. */
static Fd2HANDLE _curFd2HandleImpl = DefFd2Handle();

void SetFd2Handle(Fd2HANDLE delegate) {
    _curFd2HandleImpl = delegate;
}

Fd2HANDLE DefFd2Handle() {
    return kDefFd2HandleFunc;
}

} // namespace fatctl

namespace { using namespace fatctl; }

/* BEGIN_DECLS */   
#ifdef __cplusplus
extern "C" {
#endif

handle_from_posix_fd_func def_handle_from_posix_fd_func() {
    return kDefFd2HandleFunc;
}

int set_handle_from_posix_fd_func(handle_from_posix_fd_func func) {
    return func ? SetFd2Handle(func), 0 : (errno = EINVAL), -1;
}

int set_handle_from_posix_fd_hook(handle_from_posix_fd_hook hook, void * hint) {
    return hook ? SetFd2Handle([=](int fd){ return hook(fd, hint); }), 0 : (errno = EINVAL), -1;
}

HANDLE get_handle_from_posix_fd(int fd) {
    return _curFd2HandleImpl(fd);
}

static bool is_cloexec(int fd) {
    HANDLE h = get_handle_from_posix_fd(fd);
    DWORD flags;
    if(INVALID_HANDLE_VALUE != h && GetHandleInformation(h, &flags)) {
        return !(flags & HANDLE_FLAG_INHERIT);
    } else {
        return errno = EBADF, false;
    }
}

static bool set_cloexec(int fd, bool set = true) {
    HANDLE h = get_handle_from_posix_fd(fd);
    DWORD newflags = set ? 0 : HANDLE_FLAG_INHERIT;
    if(INVALID_HANDLE_VALUE == h) {
         _FATCTL_LOG("get_handle_from_posix_fd(%d): invalid %p errno=%d LastError=%lu", fd, h, errno, GetLastError());
        return errno = EBADF, false;
    } else if(!SetHandleInformation(h, HANDLE_FLAG_INHERIT, newflags)) {
         _FATCTL_LOG("SetHandleInformation(%p, %x) failed errno=%d LastError=%lu", h, newflags, errno, GetLastError());
        return errno = EPERM, false;
    }
    return true;
}

#ifdef _FATCTL_NTQUERYOBJECT
NTSTATUS NTAPI
NtQueryObject(HANDLE Handle,OBJECT_INFORMATION_CLASS ObjectInformationClass,
        PVOID ObjectInformation,ULONG ObjectInformationLength,PULONG ReturnLength) __attribute__((weak));
#endif

#define _FATCTL_GETARG _FATCTL_BITE(cmd, int, arg)
int fcntl(int fd, int cmd, ...) {
    switch(cmd) {
        case F_DUPFD_CLOEXEC:
            /* arg is ignored */
            // DuplicateHandle can supposedly open the handle and mark it as noinherit atomically.
            // In case of e.g. server opening constrained child processes, this is better from the
            // security purpose. We could replace `cloexec(dup(fd));` with `DuplicateHandle()`:

            // HANDLE h = get_handle_from_posix_fd(fd);
            // HANDLE p = GetCurrentProcess();
            // HANDLE dh = INVALID_HANDLE_VALUE;
            // if(!DuplicateHandle(p, h, p, &dh, 
            //         0 /* ignored, see DUPLICATE_SAME_ACCESS below */,
            //         FALSE /* inheritHandle */, DUPLICATE_SAME_ACCESS)) {
            //     return errno = BADF, -1;
            // }
            // return _open_osfhandle(dh, access_flags);

            // ...if we could infer access_flags from an open `fd`.
            // Unfortunately, there does not seem to be a reliable method. Back to square one:
            {
                int dupfd = _dup(fd);
                return ((dupfd >= 0) && set_cloexec(dupfd)) ? dupfd : -1; // errno set internally
            }

        case F_DUPFD:
            /* arg is ignored */
            return _dup(fd);

        case F_GETFD:
            /* arg is ignored */
            return is_cloexec(fd) ? FD_CLOEXEC : 0;

        case F_SETFD:
            {
                _FATCTL_GETARG;
                return set_cloexec(fd, arg & FD_CLOEXEC) ? 0 : -1; 
            }

        case F_GETFL:
            // The trickiest part: we must either examine the HANDLE or access internal CRT structures.
            // Fortunately, Toybox does not use F_GETFL. NtQueryObject might help, both here and above.
            (void) fd;
            return -1;

        case F_SETFL:
            // man fcntl: only "O_APPEND, O_ASYNC, O_DIRECT, O_NOATIME, and O_NONBLOCK" can be changed.
            (void) fd;
            return 0;

        // F_GETLK, F_SETLK, F_SETLKW

        default:
        // F_GETOWN, F_SETOWN
        // F_NOTIFY
        // F_GETPIPE_SZ, F_SETPIPE_SZ
        // ... fancier newer APIs ...
            (void) fd;
            return errno = EINVAL, -1;
    }
}

/* END_DECLS */
#ifdef __cplusplus
}
#endif
