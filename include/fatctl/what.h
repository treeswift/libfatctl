#ifndef _FATCTL_WHAT_H_
#define _FATCTL_WHAT_H_

/* BEGIN_DECLS */   
#ifdef __cplusplus
#include <string>

namespace fatctl
{

std::string Fd2PathStr(int fd);

/* fs::path ResolveRelative(int basefd, std::string relpath, int flags = 0); */
/* fs::path ResolveRelativeTarget(int basefd, std::string relpath, int flags = 0); */

} // namespace fatctl

extern "C" {
#endif

/**
 * Pure C API. It is the caller's responsibiity to either provide preallocated buffers or `free()`
 * returned C-strings. `errno` is set to `ERANGE` if the provided buffer is insufficiently large
 * to hold the returned C-string including the terminating '\0' character.
 */

/* fs::path Fd2Path(int fd); */
/* fs::path ResolveRelative(int basefd, std::string relpath, int flags = 0); */
/* fs::path ResolveRelativeTarget(int basefd, std::string relpath, int flags = 0); */

/* END_DECLS */
#ifdef __cplusplus
}
#endif

#endif /* _FATCTL_WHAT_H_ */