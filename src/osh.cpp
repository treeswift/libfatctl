#include "fatctl/conf.h"

#include <windows.h>
#include <io.h>

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

int fcntl(int fd, int cmd, ...) {
    // F_GETFD, F_SETFD
    // F_GETFL, F_SETFL
    // F_GETLK, F_SETLK, F_SETLKW
    // F_GETOWN, F_SETOWN
    // F_NOTIFY
    // F_GETPIPE_SZ, F_SETPIPE_SZ

}

/* END_DECLS */
#ifdef __cplusplus
}
#endif
