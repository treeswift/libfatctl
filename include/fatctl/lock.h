#ifndef _FATCTL_LOCK_H_
#define _FATCTL_LOCK_H_

#include "fatctl/cntl.h"

/* BEGIN_DECLS */   
#ifdef __cplusplus
extern "C" {
#endif

/**
 * fcntl() flags for locking (flag values match Linux):
 */
#ifndef F_GETLK
#define F_GETLK 5
#define F_SETLK 6
#define F_SETLKW 7

/* Non-POSIX specials (ditto): */
#define F_CANCELLK (_FATCTL_LINUX_COMPAT_OFFSET + 5)
#endif

/* TODO struct flock { ... } */
int flock(int fd, int operation);

/* END_DECLS */
#ifdef __cplusplus
}
#endif

#endif /* _FATCTL_LOCK_H_ */
