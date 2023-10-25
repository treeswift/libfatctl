#ifndef _FATCTL_DIRS_H_
#define _FATCTL_DIRS_H_

#include <sys/types.h>
#include <dirent.h>

#include "fatctl/path.h"

/* BEGIN_DECLS */   
#ifdef __cplusplus
extern "C" {
#endif

DIR* fdopendir(int dirfd);

int openat(int dirfd, const char* relpath, int flags, ...);
int mkdirat(int dirfd, const char* relpath, mode_t mode);
int renameat(int dirfd, const char *relpath, int newdirfd, const char *newrelpath);
int unlinkat(int dirfd, const char* relpath, int flags);

/* END_DECLS */
#ifdef __cplusplus
}
#endif

#endif /* _FATCTL_DIRS_H_ */
