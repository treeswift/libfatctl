#ifndef _FATCTL_PATH_H_
#define _FATCTL_PATH_H_

#include <fcntl.h>

/**
 * Relative path resolution rules.
 * Defining at least constants expected by Toybox:
 */

/**
 * This value of `dirfd` represents current working directory.
 */
#ifndef AT_FDCWD
#define AT_FDCWD -100
#endif

#define _FATCTL_RELPATH_AT_BASE (1<<8)

/**
 * If the relative path points to a symlink, use the path as is.
 * (Path components other than the leaf still may be symlinks.)
 */
#ifndef AT_SYMLINK_NOFOLLOW
#define AT_SYMLINK_NOFOLLOW (_FATCTL_RELPATH_AT_BASE<<0)
#endif

/**
 * Call `rmdir` on relative path target (affects `unlink`).
*/
#ifndef AT_REMOVEDIR
#define AT_REMOVEDIR        (_FATCTL_RELPATH_AT_BASE<<1)
#endif

/**
 * Resolve symlink if it is the target of `link`, `linkat`.
 */
#ifndef AT_SYMLINK_FOLLOW
#define AT_SYMLINK_FOLLOW   (_FATCTL_RELPATH_AT_BASE<<2)
#endif

/**
 * Operate on mount point rather than automounted volume.
 * (libfatctl is not currently aware of volume mounting)
 */
#ifndef AT_NO_AUTOMOUNT
#define AT_NO_AUTOMOUNT     (_FATCTL_RELPATH_AT_BASE<<3)
#endif

/* Treat empty relative path as '.' */
#ifndef AT_EMPTY_PATH
#define AT_EMPTY_PATH       (_FATCTL_RELPATH_AT_BASE<<4)
#endif

#endif /* _FATCTL_PATH_H_ */
