# xfce4-cava-plugin

[CAVA](https://github.com/karlstav/cava) plugin for xfce4-panel. This is a work in progress. Only [PipeWire](https://github.com/PipeWire/pipewire) and [PulseAudio](https://github.com/pulseaudio/pulseaudio) are currently supported.

![plugin](https://github.com/kreddkrikk/xfce4-cava-plugin/blob/master/cava.gif "plugin")

----

### Dependencies

Debian/Ubuntu:

    sudo apt install build-essential libfftw3-dev libpipewire-0.3-dev libpulse-dev libtool automake pkgconf

Arch Linux:

    pacman -S base-devel fftw alsa-lib pipewire-pulse xfce4-panel

### Installation

From source code repository: 

    % cd xfce4-cava-plugin
    % meson setup build
    % meson compile -C build
    % meson install -C build

### Uninstallation

    % ninja uninstall -C build

### TODO

- Improve error checking
- Add theme support
- Add profile support
