#ifndef _FATCTL_FDIO_H_
#define _FATCTL_FDIO_H_

#include <stdarg.h>

/* BEGIN_DECLS */   
#ifdef __cplusplus
extern "C" {
#endif

int dprintf(int fd, const char *format, ...);
int vdprintf(int fd, const char *format, va_list args);

#define fsync _commit

/* END_DECLS */
#ifdef __cplusplus
}
#endif

#endif /* _FATCTL_FDIO_H_ */