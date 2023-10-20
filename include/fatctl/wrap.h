#ifndef _FATCTL_WRAP_H_
#define _FATCTL_WRAP_H_

#include <windows.h>

/* BEGIN_DECLS */   
#ifdef __cplusplus

#include <functional>

namespace fatctl
{

using Fd2HANDLE = std::function<HANDLE(int)>;

/**
 * normally it's HANDLE _get_osfhandle(int), assuming fd is a standard Windows CRT open().
 * however, alternative compatibility libraries may have their own file descriptor tables.
 * pure C equivalent (with function pointers): see [set_]handle_from_posix_fd_* API below.
 */

void SetFd2Handle(Fd2HANDLE delegate);
Fd2HANDLE DefFd2Handle();

} // namespace fatctl

extern "C" {
#endif

/* pure C interfaces */

/* stateless (or "externally stateful") converter */
typedef HANDLE(*handle_from_posix_fd_func)(int fd);
int set_handle_from_posix_fd_func(handle_from_posix_fd_func func);

/* default converter. returns pointer to wrapped _get_osfhandle */
handle_from_posix_fd_func def_handle_from_posix_fd_func();

/* explicitly stateful converter. hint is an opaque table or library pointer */
typedef HANDLE(*handle_from_posix_fd_hook)(int fd, void* hint);
int set_handle_from_posix_fd_hook(handle_from_posix_fd_hook hook, void * hint);

/* Actual lookup function to use in client code. */
HANDLE get_handle_from_posix_fd(int fd);

/* TODO int wrap_handle_as_posix_fd(HANDLE, flags) */

/* END_DECLS */
#ifdef __cplusplus
}
#endif


#endif /* _FATCTL_WRAP_H_ */