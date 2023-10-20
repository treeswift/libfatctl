# What is this?

Fat Control (`libfatctl`) is a Windows library providing extended POSIX file APIs such as `fstatat`, `fchmod`, `fcntl`
and the rest of the fanciful API calls designed for constrained-resource systems running on kilobytes of memory that
the application or library of your choice is badly missing on this high-end Windows machine sporting M gigabytes and
N gigahertz. Plug it in, include `fatctl/ctrl.h` or/or `fatctl/stat.h` and let us do the rest.

The library is primarily intended for the MinGW environment or other POSIX-on-Windows environments.

# Status

Sizzling WIP. The development is its day three.

# Technical notes

Build the library with [Meson](https://mesonbuild.com)/[Ninja](https://ninja-build.org)
or use Meson to generate project files for your favorite build system.

## Headers

[not final, tbd]

* `fatctl/ctrl.h` for `fcntl()`;
* `fatctl/stat.h` for extra `stat()` routines (e.g. `lstat()`);
* `fatctl/dirs.h` for `int dirfd`- and `struct dirent*`-based APIs (e.g. `fstatat()`);
* `fatctl/conf.h` for tuning and custom logic.

Greater customization is possible by defining the following macros (per routine name):
* `_FATCTL_USESYSTEM_*` — use system definition. Applied at build time.
    If importing from a dynamic library, set to `__declspec(dllimport)` or an equivalent attribute macro.
* `_FATCTL_PRIVATIZE_*` — hide declaration. Applied at consumption time
    (e.g. when a dependent component is running its `configure` script).

## Custom fd semantic

If you are using a custom compatibility layer translating `int fd` into `HANDLE` via its own look-up table,
rather than the one provided by the CRT and used by `_get_osfhandle(int)`, give us a call: `SetFd2Handle()`
in C++, `set_handle_from_posix_fd_func()` (w/o data) or `set_handle_from_posix_fd_hook()` (with data) in C.

# Terms and conditions

## License

`libfatctl` is free as in freedom and released into the public domain (where it rightfully belongs)
with a no-strings-attached [Unlicense license](LICENSE).

## Support

Contact us, and we will do what we can. That's all we promise.

## Code of conduct

Put the good of the craft first and your sensitivities next. That's all the law and the prophets.
