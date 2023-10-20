#ifndef _FATCTL_LINK_H_
#define _FATCTL_LINK_H_

#include <stddef.h>

/* BEGIN_DECLS */   
#ifdef __cplusplus
extern "C" {
#endif

int link(const char *target, const char *linkpath);
int symlink(const char *target, const char *linkpath);
ssize_t readlink(const char *pathname, char *buf, size_t bufsz);

/* TODO lstat */
/* TODO readlink */

/* END_DECLS */
#ifdef __cplusplus
}
#endif

#endif /* _FATCTL_LINK_H_ */