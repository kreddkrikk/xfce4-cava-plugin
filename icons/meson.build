sizes = [16, 24, 32, 48, 128]

foreach size : sizes
  install_data(
    '@0@x@0@'.format(size) / 'xfce4-cava-plugin.png',
    install_dir: get_option('prefix') / get_option('datadir') / 'icons' / 'hicolor' / '@0@x@0@'.format(size) / 'apps',
    )
endforeach

install_data(
  'scalable' / 'xfce4-cava-plugin.svg',
  install_dir: get_option('prefix') / get_option('datadir') / 'icons' / 'hicolor' / 'scalable' / 'apps',
)
