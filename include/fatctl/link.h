#ifndef _FATCTL_LINK_H_
#define _FATCTL_LINK_H_

#include <sys/stat.h>
#include <stddef.h>

#include "fatctl/path.h"

/* BEGIN_DECLS */
#ifdef __cplusplus
extern "C" {
#endif

int link(const char* target, const char* linkpath);
int symlink(const char* target, const char* linkpath);
ssize_t readlink(const char* path, char *buf, size_t bufsz);

int lstat(const char* path, struct stat* statbuf);

int linkat(int trgfd, const char* relpath, int linkdir, const char* linkrelpath, int flags);
int symlinkat(const char* target, int linkdir, const char* linkrelpath);
ssize_t readlinkat(int dirfd, const char* relpath, char* buf, size_t bufsz);

/* END_DECLS */
#ifdef __cplusplus
}
#endif

#endif /* _FATCTL_LINK_H_ */
