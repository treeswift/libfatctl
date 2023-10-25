#ifndef _FATCTL_WHAT_H_
#define _FATCTL_WHAT_H_

/* BEGIN_DECLS */   
#ifdef __cplusplus
#include <string>

namespace fatctl
{

std::string Fd2PathStr(int fd);
std::string ResolveRelative(int basefd, std::string relpath, int flags = 0);
std::string ResolveRelativeTarget(int basefd, std::string relpath, int flags = 0);
std::string ResolveRelativeLinkTarget(int basefd, std::string relpath, int flags = 0);

} // namespace fatctl

extern "C" {
#endif

/**
 * Pure C API. It is the caller's responsibiity to either provide preallocated buffers or `free()`
 * returned C-strings. `errno` is set to `ERANGE` if the provided buffer is insufficiently large
 * to hold the returned C-string including the terminating '\0' character.
 */

/* TODO same as above but in pure C */

/* END_DECLS */
#ifdef __cplusplus
}
#endif

#endif /* _FATCTL_WHAT_H_ */
