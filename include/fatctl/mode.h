#ifndef _FATCTL_BITE_H_
#define _FATCTL_BITE_H_

#include <sys/stat.h>
#include <sys/types.h>
#include <stdarg.h>

#define _FATCTL_BITE(past, type, var) \
    type var; \
    { \
        va_list maybe_##var; \
        va_start(maybe_##var, past); \
        var = va_arg(maybe_##var, type); \
        va_end(maybe_##var); \
    }

/* wish we were less verbose here but MinGW convention */
#ifndef _MODE_T_
#define _MODE_T_
typedef unsigned short _mode_t;
#ifndef	NO_OLDNAMES
typedef _mode_t	mode_t;
#endif
#endif /* _MODE_T_ */

#define _FATCTL_GETMODE _FATCTL_BITE(flags, mode_t, mode)

/**
 * MinGW uses single-bit format codes (regular file, character device, named pipe and directory).
 * Flag combinations are up for grabs, and we follow the way they are formed e.g. on Linux.
 */
#ifndef _S_IFBLK
#define _S_IFBLK  (_S_IFIFO|_S_IFCHR)
/* NOTE: Linux uses _S_IFDIR|_S_IFCHR */
#endif
#ifndef _S_IFLNK
#define	_S_IFLNK  (_S_IFREG|_S_IFCHR)
#endif
#ifndef _S_IFSOCK
#define	_S_IFSOCK (_S_IFREG|_S_IFDIR)
#endif

#ifndef S_IFBLK
#define S_IFBLK _S_IFBLK
#endif
#ifndef S_IFLNK
#define	S_IFLNK _S_IFLNK
#endif
#ifndef S_IFSOCK
#define	S_IFSOCK _S_IFSOCK
#endif

#ifndef S_ISBLK
#define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#endif
#ifndef S_ISLNK
#define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#endif
#ifndef S_ISSOCK
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)
#endif

#endif /* _FATCTL_BITE_H_ */
