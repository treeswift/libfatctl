#ifndef _FATCTL_SRC_DBG_H_
#define _FATCTL_SRC_DBG_H_

#ifdef _FATCTL_DEBUG
#include <stdio.h>

#define _FATCTL_LOG(format, ...) { fprintf(_FATCTL_DEBUG, format "\n", ##__VA_ARGS__); fflush(_FATCTL_DEBUG); }
#else
#define _FATCTL_LOG(...)
#endif

#endif /* _FATCTL_SRC_DBG_H_ */
