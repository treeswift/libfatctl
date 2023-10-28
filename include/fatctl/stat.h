#ifndef _FATCTL_STAT_H_
#define _FATCTL_STAT_H_

#ifndef stat
#include <sys/types.h>
#include <sys/stat.h>
#endif /* stat */
#include "fatctl/path.h"

/* BEGIN_DECLS */
#ifdef __cplusplus
extern "C" {
#endif

struct stat;

int fstatat(int dirfd, const char *relpath, struct stat* statbuf, int flags);

/* END_DECLS */
#ifdef __cplusplus
}
#endif

#endif /* _FATCTL_STAT_H_ */
