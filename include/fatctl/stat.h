#ifndef _FATCTL_STAT_H_
#define _FATCTL_STAT_H_

#include "fatctl/path.h"

/* BEGIN_DECLS */   
#ifdef __cplusplus
extern "C" {
#endif

int fstatat(int dirfd, const char *relpath, struct stat *statbuf, int flags);

/* END_DECLS */
#ifdef __cplusplus
}
#endif

#endif /* _FATCTL_STAT_H_ */
