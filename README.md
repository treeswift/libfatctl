# What is this?

Fat Control (`libfatctl`) is a Windows library providing extended POSIX file APIs such as `fstatat`, `fchmod`, `fcntl`
and the rest of the fanciful API calls designed for constrained-resource systems running on kilobytes of memory that
the application or library of your choice is badly missing on this high-end Windows machine sporting M gigabytes and
N gigahertz. Plug it in, include what you need (see the header file breakdown below) and let us do the rest.

The library is primarily intended for the MinGW environment or other POSIX-on-Windows environments.

# Status

Sizzling WIP. The development is in its day four. Outstanding API families to implement are `fcntl` and `flock`. Also,
`what.h` and `wrap.h` (supplementary APIs not present in *nix/POSIX but aiding Windows integration) are not yet complete.

# Technical notes

Build the library with [Meson](https://mesonbuild.com)/[Ninja](https://ninja-build.org)
or use Meson to generate project files for your favorite build system.

Use the `filesystem` option to provide the namespace of `<filesystem>`. Default is `std::filesystem`.
(This choice does not affect the public API of the library.)

## Headers

* `fatctl/full.h` is the "everything bagel" to include the entirety of `libfatctl` public API.

Otherwise, build your own plate:

* `fatctl/mode.h` defines `mode_t` (unless already defined) and a few extra file types (ditto).
* `fatctl/path.h` defines the `AT_*` family of constants that command special modes of relative path resolution.
* `fatctl/open.h` redefines `open()`, exposing old definitions as `fatctl_fallback_openf()` and `fatctl_fallback_openfm()`.
 Include after `<io.h>`/`<fcntl.h>` and such.
* `fatctl/fdio.h` declares `[v]dprintf()`, which is for some odd reason missing in MinGW.
* `fatctl/cntl.h` declares `fcntl()`.
* `fatctl/info.h` declares library-specific filesystem property query functions, such as `fatctl_fsinfo_query()` to get the block size.
* `fatctl/link.h` declares hardlink/symlink manipulation APIs, such as `[sym]link[at]()` and `readlink[at]()` (note that creating symlinks
 requires elevation before a certain Windows 10 build — our test application is aware of that and respects EPERM);
* `fatctl/dirs.h` declares `int dirfd`-based (`openat`/`mkdirat`/`unlinkat`) and `struct dirent*`-based APIs (`fdopendir()`).
* `fatctl/stat.h` declares extra `stat()` routines (e.g. `fstatat()`);
* `fatctl/lock.h` declares `struct flock` (for use with `fcntl()`) and optionally `flock()` (emulation with full-range locks).
* `fatctl/perm.h` declares permission-setting and permission-querying APIs (`*access*()`, `*chmod*()`).
* `fatctl/what.h` declares (non-POSIX) API to restore file system paths from `int` file descriptors. (When using the pure C API,
 it is the caller's responsibility to provide appropriately sized buffers (typically `char[MAX_PATH]`) or `free` returned C-strings.)
* `fatctl/wrap.h` declares fd semantic customization APIs to use with custom POSIX compatibility layers (see below).

MOREINFO: greater selectiveness MAY be implemented with per-routine `*_USESYSTEM_*` and `*_PRIVATIZE_*` macros in case anyone
thinks that it might be useful (we aren't currently sure ourselves). Link: https://github.com/treeswift/libfatctl/issues/2

## Build options

The project exposes the following "production" build options:

Meson option|Type|Compiler macro|Effect|Default
---|---|---|---|---
`filesystem`|string|`_FATCTL_FSNS`|Namespace of `<filesystem>`|`std::filesystem`
`usentquery`|flag|`_FATCTL_NTQUERY`|Use `NtQuery*` functions to provide `FD_GETFL`^|true/1|
`fcntlflock`|flag|`_FATCTL_EMUFLOCK`|`flock()` emulation with `fcntl()` locks^^|false/undefined|

 ^ The only viable alternative would be to wrap `open*()` and `dup*()` requests, redefining or macroizing all API flavors
and aliases that allocate an `fd`. (`libfatctl` already redefines `open`, but not `dup*`; it also wraps `_open_osfhandle()`
but does not try to intercept direct invocations of it.) Such a solution would be runtime-kosher but extremely fragile at
compile time; as of now, we don't even try, and `fcntl(fd, F_GETFL)` simply returns `errno=EINVAL, -1`. Please file a bug /
feature request if the Windows version you are targeting misses those functions or if your code cannot link to `ntdll.dll`.

 ^^ WinAPI `LockFile*` functionality is range-based, as are `fcntl()` locks. There is no file-based API similar to `flock()`
other than `dwShareMode` flags specified when the file is opened. Emulating `flock()` with `fcntl()` breaks both the strict
API contract and the common Linux assumption the the two locking APIs are independent; however, POSIX requirements are more
relaxed in this respect. (Toybox, our reference client component, does not use `F_GETFL`.)

Development-time options, such as debug info generation flags, can be added to (`.gitignore`d) `_user/meson.build` file.
Particularly, `-D_FATCTL_DEBUG=stdout` or `-D_FATCTL_DEBUG=stderr` enables the `_FATCTL_LOG` macro and routes its output
to `stdout` or `stderr`, respectively.

## Custom fd semantic

If you are using a custom compatibility layer translating `int fd` into `HANDLE` via its own look-up table,
rather than the one provided by the CRT and used by `_get_osfhandle(int)`, give us a call: `SetFd2Handle()`
in C++, `set_handle_from_posix_fd_func()` (w/o data) or `set_handle_from_posix_fd_hook()` (with data) in C.
Client code can itself refer to the current mapping via `get_handle_from_posix_fd(int fd)`.

Customization of `_get_osfhandle(int)` requires a converse customization of `_open_osfhandle(HANDLE,int)`.
The corresponding methods are `SetHandle2Fd()` in C++ and `set_posix_fd_from_handle_func()` (without data)
or `set_posix_fd_from_handle_hook()` (with data) in C; the `HANDLE` registration itself is accomplished by
calling `wrap_handle_as_posix_fd()`.

No locks guard the assignments, and no atomicity is guaranteed in general. It is therefore (as well as for
other reasons) best to make both assignments only once, early in application initialization code.

## Known limitations

* Long paths are not supported. The current expectation is that all paths fit in `MAX_PATH`.
 (Our primary target is [Windows RT](https://github.com/armdevvel/mxe-SHARED).)
* As of now, there is no wide-character version of the API. Our current goal is to glue the gaps between MinGW (which
obeys the Windows A/W convention) and [Toybox](https://landley.net/toybox) (which is, to the best of our current
understanding, 8-bit locale-agnostic).
At present, it means that paths with non-ASCII characters will not be handled correctly. One possible remedy may be using
the 8.3 notation which is guranteed to be lower 7-bit ASCII; however, no solution confined within `libfatctl` will affect
Windows C runtime APIs called by Toybox directly. Some inlined or statically linked translation layer may be necessary.

# Terms and conditions

## License

`libfatctl` is free as in freedom and released into the public domain (where it rightfully belongs)
with a no-strings-attached [Unlicense license](LICENSE).

## Support

Contact us, and we will do what we can. That's all we promise.

## Code of conduct

Put the good of the craft first and your sensitivities next. That's all the law and the prophets.
