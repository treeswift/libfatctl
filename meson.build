project('libfatctl', 'cpp',
    version : '0.0.1',
    default_options : ['warning_level=3'],
  )

add_global_arguments('-D_WIN32_WINNT=_WIN32_WINNT_WIN8', language: 'cpp') # Win8+
add_global_arguments('-Wno-c++17-extensions', language: 'cpp') # fs requires C++17 anyway

subdir('_user') # specify e.g. debug flags in `meson.build` there

subdir('src')

fatctl = shared_library('fatctl', lib_implementation,
    include_directories: ['include'],
    install: true,
  )

executable('dirinfo', files('samples/dirinfo.cpp'),
    include_directories: ['include', 'src'], # playground can use nonpublic APIs
    link_with: [fatctl],
    install: true,
  )

executable('test-atf', files('samples/dirtest.cpp'),
    include_directories: ['include', 'src'], # ditto
    link_with: [fatctl],
    install: true,
  )

subdir('include/fatctl')

install_headers(public_includes, subdir: 'fatctl')
