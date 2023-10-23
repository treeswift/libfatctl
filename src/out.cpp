#include "fatctl/fdio.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

extern "C" {

#ifndef vasprintf
#define vasprintf __mingw_vasprintf
#endif

/**
 * A good way to introduce a responsibility chain could be adding a few weak declarations, e.g.:
 * 
 * extern int vasprintf(char** alloc, const char* format, va_list args) __attribute__((weak));
 * extern int __mingw_vasprintf(char** alloc, const char* format, va_list args) __attribute__((weak));
 * 
 * -- and then testing the function pointers for equality to null. In practice, however, &vasprintf
 * would produce something like 0x1. We could test whether the address is below the image base, but
 * that's an overcomplication. Just pass -Dvasprintf=<some-implementation-available-on-your-system>.
 * Don't allocate a fixed-size buffer. Or, if you do, use a safe snprintf impl and test for overruns.
 */

int dprintf(int fd, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int retval = vdprintf(fd, format, args);
    va_end(args);
    return retval;
}

int vdprintf(int fd, const char *format, va_list args) {
    char* dyn = nullptr;
    const int length = vasprintf(&dyn, format, args);
    int retval = length >= 0 ? write(fd, dyn, length) : (errno = EINVAL, -1);
    if(dyn) free(dyn); // yes, I know, unique_ptr blah blah blah. just keep it readable.
    return retval;
}

} // extern "C"

