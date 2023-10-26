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

/* Default implementations */
HANDLE GetOSFHandle(int fd) { return (HANDLE)_get_osfhandle(fd); }
int OpenHandleFd(HANDLE h, int flags) { return _open_osfhandle((intptr_t)h, flags); }

handle_from_posix_fd_func const kDefFd2HandleFunc = &GetOSFHandle;
posix_fd_from_handle_func const kDefHandle2FdFunc = &OpenHandleFd;

/* Global state. */
static Fd2HANDLE _curFd2HandleImpl = DefFd2Handle();
static HANDLE2Fd _curHandle2FdImpl = DefHandle2Fd();

void SetFd2Handle(Fd2HANDLE delegate) {
    _curFd2HandleImpl = delegate;
}

Fd2HANDLE DefFd2Handle() {
    return kDefFd2HandleFunc;
}

void SetHandle2Fd(HANDLE2Fd delegate) {
    _curHandle2FdImpl = delegate;
}

HANDLE2Fd DefHandle2Fd() {
    return kDefHandle2FdFunc;
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
    return func ? SetFd2Handle(func), 0 : (errno = EINVAL, -1);
}

int set_handle_from_posix_fd_hook(handle_from_posix_fd_hook hook, void * hint) {
    return hook ? SetFd2Handle([=](int fd){ return hook(fd, hint); }), 0 : (errno = EINVAL, -1);
}

HANDLE get_handle_from_posix_fd(int fd) {
    return _curFd2HandleImpl(fd);
}

int set_posix_fd_from_handle_func(posix_fd_from_handle_func func) {
    return func ? SetHandle2Fd(func), 0 : (errno = EINVAL, -1);
}

posix_fd_from_handle_func def_posix_fd_from_handle_func() {
    return kDefHandle2FdFunc;
}

int set_posix_fd_from_handle_hook(posix_fd_from_handle_hook hook, void* hint) {
    return hook ? SetHandle2Fd([=](HANDLE h, int flags){ return hook(h, flags, hint); }), 0 : (errno = EINVAL, -1);
}

int wrap_handle_as_posix_fd(HANDLE h, int flags) {
    return _curHandle2FdImpl(h, flags);
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
         _FATCTL_LOG("SetHandleInformation(%p, %lx) failed errno=%d LastError=%lu", h, newflags, errno, GetLastError());
        return errno = EPERM, false;
    }
    return true;
}

#ifdef _FATCTL_NTQUERYOBJECT
NTSTATUS NTAPI
NtQueryObject(HANDLE Handle,OBJECT_INFORMATION_CLASS ObjectInformationClass,
        PVOID ObjectInformation,ULONG ObjectInformationLength,PULONG ReturnLength) __attribute__((weak));

// ALSO: see NtQueryInformationFile 
// https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntqueryinformationfile
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
            // return wrap_handle_as_posix_fd(dh, access_flags);

            // ...if we could infer access_flags from an open `fd`.
            // There used to be a `_pioinfo` hack, but newer versions of Windows don't support it:
            //  see: https://github.com/oneclick/rubyinstaller2/issues/308
            //  see also: https://github.com/perl/perl5/issues/14826
            //  Examples of what _used_ to work can be found in STLPort:
            //  https://cpp.hotexamples.com/examples/-/-/_pioinfo/cpp-_pioinfo-function-examples.html
            // NOTE STLPort is mandatory-attribution license: http://www.stlport.org/doc/license.html
            // We can link to the symbol weakly and use our own fd->flags map for a fallback;
            //  then there are `dup`, `dup2`, but they are macros and can be redefined in `open.h`.
            // We can use NtQueryObject to get current access flags, but not the current stream flags.
            // Also, we can try writing 0 bytes to the stream, but again, it only reveals access mode.
            // Unfortunately, there does not seem to be a reliable method that works in all cases.
            // Back to square one:
            {
                int dupfd = dup(fd);
                return ((dupfd >= 0) && set_cloexec(dupfd)) ? dupfd : -1; // errno set internally
            }

        case F_DUPFD:
            /* arg is ignored */
            return dup(fd);

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
