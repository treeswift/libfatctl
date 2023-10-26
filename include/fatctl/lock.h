#ifndef _FATCTL_LOCK_H_
#define _FATCTL_LOCK_H_

#include "fatctl/cntl.h"
#include <fnctl.h>

/* BEGIN_DECLS */   
#ifdef __cplusplus
extern "C" {
#endif

/**
 * `fcntl()` `cmd` values for locking (flag values match Linux)
 */
#ifndef F_GETLK
#define F_GETLK 5
#define F_SETLK 6
#define F_SETLKW 7

/* Non-POSIX specials (ditto): */
#define F_CANCELLK (_FATCTL_LINUX_COMPAT_OFFSET + 5)
#endif

/**
 * struct flock { short l_type } values
 */
#ifndef F_RDLCK
#define F_RDLCK 0
#define F_WRLCK 1
#define F_UNLCK 2
#endif

/**
 * fcntl() lock control structure
 */
struct flock {
    short l_type;   /* F_{RD|WR|UN}LCK */
    short l_whence; /* SEEK_{SET|CUR|END} */
    off_t l_start;  /* Range start */
    off_t l_len;    /* Range length */
    pid_t l_pid;    /* [out] Lock owner PID */
};

#ifdef _FATCTL_EMUFLOCK
/**
 * Opcodes for `flock()` (SHared, EXclusive, Non-Blocking, UNlock)
 */
#ifndef LOCK_SH
#define LOCK_SH (1<<0)
#define LOCK_EX (1<<1)
#define LOCK_NB (1<<2)
#define LOCK_UN (1<<3)
#endif

/* TODO struct flock { ... } */
int flock(int fd, int operation);
#endif

/* END_DECLS */
#ifdef __cplusplus
}
#endif

#endif /* _FATCTL_LOCK_H_ */
