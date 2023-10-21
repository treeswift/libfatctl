# What is this?

Fat Control (`libfatctl`) is a Windows library providing extended POSIX file APIs such as `fstatat`, `fchmod`, `fcntl`
and the rest of the fanciful API calls designed for constrained-resource systems running on kilobytes of memory that
the application or library of your choice is badly missing on this high-end Windows machine sporting M gigabytes and
N gigahertz. Plug it in, include what you need (see the header file breakdown below) and let us do the rest.

The library is primarily intended for the MinGW environment or other POSIX-on-Windows environments.

# Status

Sizzling WIP. The development is its day three — file trees are being planted, but the volume astronomy is still a mess. Don't look up.

# Technical notes

Build the library with [Meson](https://mesonbuild.com)/[Ninja](https://ninja-build.org)
or use Meson to generate project files for your favorite build system.

## Headers

* `fatctl/mode.h` defines `mode_t` (unless already defined) and a few extra file types (ditto).
* `fatctl/path.h` defines the `AT_*` family of constants that command special modes of relative path resolution.
* `fatctl/open.h` redefines `open()`, exposing old definition as `fatctl_open_fallback()`. Include after `<io.h>`/`<fcntl.h>` and such.
* `fatctl/fdio.h` declares `[v]dprintf()`, which is for some odd reason missing in MinGW.
* `fatctl/cntl.h` declares `fcntl()`;
* `fatctl/info.h` declares library-specific filesystem property query functions, such as `fatctl_fsinfo_query()` to get the block size.
* `fatctl/link.h` declares hardlink/symlink manipulation APIs, such as `[sym]link[at]()` and `readlink[at]()`;
* `fatctl/dirs.h` declares `int dirfd`-based (`openat`/`mkdirat`/`unlinkat`) and `struct dirent*`-based APIs (`fdopendir()`);
* `fatctl/stat.h` declares extra `stat()` routines (e.g. `fstatat()`);
* `fatctl/lock.h` declares `flock()`. It may declare other (alternative and more fine-grained) file locking APIs in the future.
* `fatctl/poll.h` (stub) declares change notification APIs.
* `fatctl/perm.h` (stub) declares permission-setting APIs.
* `fatctl/wrap.h` declares fd semantic customization APIs to use with custom POSIX compatibility layers (see below).

MOREINFO: greater selectiveness MAY be implemented with per-routine `*_USESYSЕEM_*` and `*_PRIVATIZE_*` macros in case anyone
thinks that it might be useful (we aren't currently sure ourselves). Link: https://github.com/treeswift/libfatctl/issues/2

## Custom fd semantic

If you are using a custom compatibility layer translating `int fd` into `HANDLE` via its own look-up table,
rather than the one provided by the CRT and used by `_get_osfhandle(int)`, give us a call: `SetFd2Handle()`
in C++, `set_handle_from_posix_fd_func()` (w/o data) or `set_handle_from_posix_fd_hook()` (with data) in C.
Client code can itself refer to the assigned mapping via `get_handle_from_posix_fd(int fd)`.

_TODO: custom handle registration as fd._

# Terms and conditions

## License

`libfatctl` is free as in freedom and released into the public domain (where it rightfully belongs)
with a no-strings-attached [Unlicense license](LICENSE).

## Support

Contact us, and we will do what we can. That's all we promise.

## Code of conduct

Put the good of the craft first and your sensitivities next. That's all the law and the prophets.
