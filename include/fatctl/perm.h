#ifndef _FATCTL_PERM_H_
#define _FATCTL_PERM_H_

#include "fatctl/mode.h"
#include "fatctl/path.h"

/* BEGIN_DECLS */
#ifdef __cplusplus
extern "C" {
#endif

int faccessat(int dirfd, const char* pathname, int mode, int flags);

/**
 * The Microsoft CRT defines _chmod (and its alias chmod) as:
 *  int chmod(const char* path, int mode);
 */
int fchmod(int fd, mode_t mode);
int fchmodat(int dirfd, const char* relpath, mode_t mode, int flags);

/* END_DECLS */
#ifdef __cplusplus
}
#endif

#endif /* _FATCTL_PERM_H_ */
