# BlueALSA ALSA Configuration Service

## Introduction

`bluealsa-autoconfig` is a simple program to add and remove ALSA configuration
nodes for BlueALSA Bluetooth PCM and CTL devices. It can also optionally
simulate udev events on Bluetooth audio device connect and disconnect.

## Usage

The program must be run as root to gain the necessary privileges for modifying
the global ALSA configuration and triggering udev events. See the
[manual page](./bluealsa-autoconfig.8.rst) for full details.
A systemd service unit file is included.

## Installation

```
meson setup builddir
cd builddir
meson compile
sudo meson install
```

To include the manual page, the application `rst2man` from the `docutils` project is required. Many distributions provide this in the package `python3-docutils`. To enable building and installing the manual page, setup the build directory with
```
meson setup -Ddoc=true builddir
```
or, if the build directory has already been set up, chnage its configuration with
```
meson configure -Ddoc=true builddir
```

## License

This project is licensed under the terms of the MIT license.

It includes copies of source code files from the bluez-alsa project, which are also licensed under the MIT license and all rights to those files remain with the original author.
