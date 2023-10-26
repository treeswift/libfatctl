project('libfatctl', 'cpp',
    version : '0.0.1',
    default_options : ['warning_level=3'],
  )

add_global_arguments('-D_FATCTL_FSNS=' + get_option('filesystem'), language: 'cpp')
if get_option('usentquery')
  add_global_arguments('-D_FATCTL_NTQUERY=1', language: 'cpp') # NtQuery*
  # ... and link to -lntdll (we do so implicitly)
endif
if get_option('fcntlflock')
  add_global_arguments('-D_FATCTL_EMUFLOCK=1', language: 'cpp') # flock()
endif
add_global_arguments('-D_WIN32_WINNT=_WIN32_WINNT_WIN8', language: 'cpp') # Win8+
add_global_arguments('-Wno-c++17-extensions', language: 'cpp') # fs requires C++17 anyway
add_global_arguments('-Wno-gnu-zero-variadic-macro-arguments', language: 'cpp') ##__VA_ARGS__

subdir('_user') # specify e.g. debug flags in `meson.build` there

subdir('src')

fatctl = shared_library('fatctl', lib_implementation,
    include_directories: ['include'],
    install: true,
  )

executable('dirinfo', files('samples/dirinfo.cpp', 'samples/sysroot.cpp'),
    include_directories: ['include', 'src'], # playground can use nonpublic APIs
    link_with: [fatctl],
    install: true,
  )

executable('test-libfatctl', files('samples/dirtest.cpp', 'samples/sysroot.cpp'),
    include_directories: ['include', 'src'], # ditto
    link_with: [fatctl],
    install: true,
  )

subdir('include/fatctl')

install_headers(public_includes, subdir: 'fatctl')
