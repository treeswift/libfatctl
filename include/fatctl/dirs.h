#ifndef _FATCTL_DIRS_H_
#define _FATCTL_DIRS_H_

#include <sys/types.h>
#include <dirent.h>

/* BEGIN_DECLS */   
#ifdef __cplusplus
extern "C" {
#endif

DIR *fdopendir(int fd);

/* END_DECLS */
#ifdef __cplusplus
}
#endif

#endif /* _FATCTL_DIRS_H_ */