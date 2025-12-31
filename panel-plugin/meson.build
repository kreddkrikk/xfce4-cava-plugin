plugin_sources = [
  'dialogs.c',
  'dialogs.h',
  'plugin.c',
  'plugin.h',
  'cava.c',
  'cava/cavacore.c',
  'cava/cavacore.h',
  'cava/input/pulse.c',
  'cava/input/pulse.h',
  'cava/input/pipewire.c',
  'cava/input/pipewire.h',
  'cava/input/common.c',
  'cava/input/common.h',
  xfce_revision_h,
]

cc = meson.get_compiler('c')

plugin_install_subdir = 'xfce4' / 'panel' / 'plugins'

plugin_lib = shared_module(
  'cava',
  plugin_sources,
  gnu_symbol_visibility: 'hidden',
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('xfce4-cava-plugin'),
  ],
  include_directories: [
    include_directories('..'),
    include_directories('cava'),
  ],
  dependencies: [
    glib,
    gtk,
    libxfce4panel,
    libxfce4ui,
    libxfce4util,
    libm,
    libfftw3,
    libpulse,
    libpulse_simple,
    libpipewire,
  ],
  install: true,
  install_dir: get_option('prefix') / get_option('libdir') / plugin_install_subdir,
)

i18n.merge_file(
  input: 'cava.desktop.in',
  output: 'cava.desktop',
  po_dir: '..' / 'po',
  type: 'desktop',
  install: true,
  install_dir: get_option('prefix') / get_option('datadir') / plugin_install_subdir,
)
