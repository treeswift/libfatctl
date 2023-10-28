#ifndef _FATCTL_CNTL_H_
#define _FATCTL_CNTL_H_

/* BEGIN_DECLS */
#ifdef __cplusplus
extern "C" {
#endif

/* `cmd` values (lock-related ones are defined separately in `lock.h`): */

#ifndef F_DUPFD
#define F_DUPFD 0
#endif

#ifndef F_GETFD
#define F_GETFD 1
#endif

#ifndef F_SETFD
#define F_SETFD 2
#endif

#ifndef F_GETFL
#define F_GETFL 3
#endif

#ifndef F_SETFL
#define F_SETFL 4
#endif

/* Extra `cmd` values used by Linux-originated tools: */
#define _FATCTL_LINUX_COMPAT_OFFSET 0x400

#ifndef F_DUPFD_CLOEXEC
/* F_(SET|GET)LEASE are 0..1 and F_CANCELLK (lock.h) is 5 */
#define F_DUPFD_CLOEXEC (_FATCTL_LINUX_COMPAT_OFFSET + 6)
#endif

/**
 * There is no mask to check against, as the customary rules are quite lax:
 * only known bits matter. So, only define individual flag constant(s) here.
 */

#ifndef FD_CLOEXEC
#define FD_CLOEXEC 1
#endif

int fcntl(int fd, int cmd, ...);

/* END_DECLS */
#ifdef __cplusplus
}
#endif

#endif /* _FATCTL_CNTL_H_ */
