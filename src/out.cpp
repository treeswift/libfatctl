#include "fatctl/fdio.h"

#include <stdio.h>
#include <unistd.h>
#include <errno.h>

extern "C" {

extern int vasprintf(char** alloc, const char* format, va_list args) __attribute__((weak));
extern int __mingw_vasprintf(char** alloc, const char* format, va_list args) __attribute__((weak));

int dprintf(int fd, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int retval = vdprintf(fd, format, args);
    va_end(args);
    return retval;
}

int vdprintf(int fd, const char *format, va_list args) {
    char* dyn;
    auto func = &__mingw_vasprintf ? &__mingw_vasprintf : &vasprintf;
    const int length = (*func)(&dyn, format, args);
    return length >= 0 ? write(fd, dyn, length) : (errno = EINVAL, -1);
}

} // extern "C"

