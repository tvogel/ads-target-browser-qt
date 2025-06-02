project('QtAdsTargetBrowser', 'cpp',
  version : '0.1',
  default_options : [
    'buildtype=debug',
    'warning_level=3',
    'werror=true',
    'b_pie=true',
  ]
)

# some hardening options
# add_project_arguments('-D_FORTIFY_SOURCE=2', language: 'cpp')

cxx = meson.get_compiler('cpp')
libs = [
    cxx.find_library('ws2_32', required: false),
    dependency('threads'),
]

qt6 = import('qt6')
qt6_dep = dependency('qt6', modules: ['Core', 'Gui', 'Widgets', 'Network', 'Core5Compat'])
inc = include_directories('../AdsLib')

sources = files(
  'TargetBrowser.cpp',
  'ConnectDialog.cpp',
  'AdsDatatypeEntry.cpp',
  'AdsSymbolUploadInfo2.cpp',
  'AdsSymbolModel.cpp',
  'AdsDatatypeIndex.cpp',
  'AdsSymbolIndex.cpp',
  'AdsCodec.cpp',
  'RouteCreationDialog.cpp',
  'RemoteRouteCreation.cpp',
)

qobject_headers = files(
  'TargetBrowser.h',
  'ConnectDialog.h',
  'AdsSymbolModel.h',
  'RouteCreationDialog.h',
)

ui_files = files(
  'TargetBrowser.ui',
  'ConnectDialog.ui',
  'RouteCreationDialog.ui',
)

moc_files = qt6.compile_moc(
  headers: qobject_headers,
  include_directories: inc,
  dependencies: qt6_dep
)

uic_files = qt6.compile_ui(sources : ui_files, preserve_paths: true)

sample16 = executable('targetbrowser',
  [sources, uic_files, moc_files],
  include_directories: inc,
  dependencies: [
    cxx.find_library('AdsLib', dirs: meson.project_source_root() + '/../build/'),
    libs,
    qt6_dep,
  ]
)

if get_option('tcadsdll_lib') != ''
  libs += cxx.find_library('TcAdsLib', dirs: meson.project_source_root() + '/../build/')
  libs += cxx.find_library('TcAdsDll', dirs: get_option('tcadsdll_lib'))
  inc += get_option('tcadsdll_include')
else
  libs = disabler()
endif

tcsample16 = executable('tcsample16',
  [sources, uic_files, moc_files],
  cpp_args: '-DUSE_TWINCAT_ROUTER',
  include_directories: inc,
  dependencies: [
    libs,
    qt6_dep,
  ]
)

