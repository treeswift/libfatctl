#ifndef _FATCTL_WRAP_H_
#define _FATCTL_WRAP_H_

#include <windows.h>

/* BEGIN_DECLS */
#ifdef __cplusplus

#include <functional>

namespace fatctl
{

using Fd2HANDLE = std::function<HANDLE(int)>;
using HANDLE2Fd = std::function<int(HANDLE, int)>;

/**
 * normally it's HANDLE _get_osfhandle(int), assuming fd is a standard Windows CRT open().
 * however, alternative compatibility libraries may have their own file descriptor tables.
 * pure C equivalent (with function pointers): see [set_]handle_from_posix_fd_* API below.
 */

void SetFd2Handle(Fd2HANDLE delegate);
Fd2HANDLE DefFd2Handle();

void SetHandle2Fd(HANDLE2Fd delegate);
HANDLE2Fd DefHandle2Fd();

} // namespace fatctl

extern "C" {
#endif

/* pure C interfaces */

/* stateless (or "externally stateful") converter */
typedef HANDLE(*handle_from_posix_fd_func)(int fd);
int set_handle_from_posix_fd_func(handle_from_posix_fd_func func);

/* default converter. returns pointer to wrapped `_get_osfhandle` */
handle_from_posix_fd_func def_handle_from_posix_fd_func();

/* explicitly stateful converter. hint is an opaque table or library pointer */
typedef HANDLE(*handle_from_posix_fd_hook)(int fd, void* hint);
int set_handle_from_posix_fd_hook(handle_from_posix_fd_hook hook, void* hint);

/* Actual lookup function to use in client code. */
HANDLE get_handle_from_posix_fd(int fd);

/* Same as above, but for HANDLE registration as fd */

/* stateless (or "externally stateful") registration function */
typedef int(*posix_fd_from_handle_func)(HANDLE h, int flags);
int set_posix_fd_from_handle_func(posix_fd_from_handle_func func);

/* default registration function. returns pointer to wrapped `_open_osfhandle` */
posix_fd_from_handle_func def_posix_fd_from_handle_func();

/* explicitly stateful registration function. hint is an opaque table or library pointer */
typedef int(*posix_fd_from_handle_hook)(HANDLE h, int flags, void* hint);
int set_posix_fd_from_handle_hook(posix_fd_from_handle_hook hook, void* hint);

/* Actual registration function to use in client code */
int wrap_handle_as_posix_fd(HANDLE h, int flags);

/* END_DECLS */
#ifdef __cplusplus
}
#endif

#endif /* _FATCTL_WRAP_H_ */
