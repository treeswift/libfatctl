#include "fatctl/fdio.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
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
    char* dyn = nullptr;
    auto func = &__mingw_vasprintf ? &__mingw_vasprintf : &vasprintf;
    const int length = (*func)(&dyn, format, args);
    int retval = length >= 0 ? write(fd, dyn, length) : (errno = EINVAL, -1);
    if(dyn) free(dyn); // yes, I know, unique_ptr blah blah blah. just keep it readable.
    return retval;
}

} // extern "C"

