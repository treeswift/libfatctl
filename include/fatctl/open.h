#ifndef _FATCTL_OPEN_H_
#define _FATCTL_OPEN_H_

#include "fatctl/mode.h"

/* MOREINFO consider removing this import and making this file a "textual header"
     to support alternative implementations of `open` (win32iocompat will tell) */
#include <unistd.h>

/* Extra _O_*, O_* flags to use with `open` */

#ifndef _O_DIRECTORY
#ifdef O_DIRECTORY
#define _O_DIRECTORY O_DIRECTORY
#else
#define _O_DIRECTORY 0x1000000
#endif
#endif

#ifndef _O_NOFOLLOW
#ifdef O_NOFOLLOW
#define _O_NOFOLLOW O_NOFOLLOW
#else
#define _O_NOFOLLOW 0x2000000
#endif
#endif

#ifndef _O_CLOEXEC
#define _O_CLOEXEC _O_NOINHERIT
#endif

#if !defined(NO_OLDNAMES) || defined(_POSIX)
#ifndef O_DIRECTORY
#define O_DIRECTORY _O_DIRECTORY
#endif
#ifndef O_NOFOLLOW
#define O_NOFOLLOW _O_NOFOLLOW
#endif
#ifndef O_CLOEXEC
#define O_CLOEXEC _O_CLOEXEC
#endif
#endif /* old POSIX names */

/* Overriding existing `open` API signatures with aliases while keeping overridden implementations available. */

inline int fatctl_fallback_openf(const char* path, int flags) {
    return open(path, flags);
}

inline int fatctl_fallback_openfm(const char* path, int flags, mode_t mode) {
    return open(path, flags, mode);
}

#ifdef open
#undef open
#endif
#define open fatctl_allcatch_open

/* BEGIN_DECLS */
#ifdef __cplusplus
extern "C" {
#endif

int open(const char* path, int mode, ...);

/* END_DECLS */
#ifdef __cplusplus
}
#endif

#endif /* _FATCTL_OPEN_H_ */
