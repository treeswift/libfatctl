#ifndef _FATCTL_PERM_H_
#define _FATCTL_PERM_H_

/**
 * These header files, and these functions, are missing on MinGW.
 * You may want to check out `libwusers` as a drop-in polyfill.
 */

#if __has_include(<pwd.h>)
#include <pwd.h>
#endif
#if __has_include(<grp.h>)
#include <grp.h>
#endif

#include "fatctl/mode.h"
#include "fatctl/path.h"

/* BEGIN_DECLS */   
#ifdef __cplusplus
extern "C" {
#endif

int faccessat(int dirfd, const char* pathname, int mode, int flags);

/* ROADMAP absolute lowest priority to implement */

/**
 * The Microsoft CRT defines _chmod (and its alias chmod) as:
 *  int chmod(const char* path, int mode);
 */
int fchmod(int fd, mode_t mode);
int fchmodat(int dirfd, const char* relpath, mode_t mode, int flags);

/* ROADMAP second lowest priority to implement */

#if defined(_UID_T_DEFINED_) && defined(_GID_T_DEFINED_)
int chown(const char *pathname, uid_t owner, gid_t group);
int fchown(int fd, uid_t owner, gid_t group);
int lchown(const char *pathname, uid_t owner, gid_t group);
int fchownat(int dirfd, const char *pathname, uid_t owner, gid_t group, int flags);
#endif

/* END_DECLS */
#ifdef __cplusplus
}
#endif

#endif /* _FATCTL_PERM_H_ */
