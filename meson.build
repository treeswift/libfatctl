project('libfatctl', 'cpp',
    version : '0.0.1',
    default_options : ['warning_level=3'],
  )

add_global_arguments('-D_WIN32_WINNT=_WIN32_WINNT_WIN8', language: 'cpp') # Win8+

fatctl = shared_library('fatctl', files('src/dir.cpp'),
    include_directories: ['include'],
    install: true,
  )

executable('dirinfo', files('samples/dirinfo.cpp'),
    include_directories: ['include', 'src'], # playground can use nonpublic APIs
    link_with: [fatctl],
    install: true,
  )