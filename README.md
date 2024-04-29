# BlueALSA ALSA Configuration Service

## Introduction

`bluealsa-autoconfig` is a simple program to add and remove ALSA configuration
nodes for BlueALSA Bluetooth PCM and CTL devices. It can also optionally
simulate `udev` events on Bluetooth audio device connect and disconnect.
See [The bluez-alsa project](https://github.com/arkq/bluez-alsa) for more
information on BlueALSA.

ALSA has built-in functions to add and remove PCM configuration nodes when
sound cards are added or removed. However, it provides no equivalent
functionality for software PCM types such as BlueALSA. This is unfortunate
because Bluetooth audio devices are more likely to be disconnected during a
session, so dynamic management of the ALSA configuration is even more important
for Bluetooth than it is for sound cards.

`bluealsa-autoconfig` aims to rectify that situation by updating the ALSA
configuration dynamically whenever a Bluetooth audio device connects or
disconnects. So, for example, when no Bluetooth audio devices are connected,
`aplay -L` may show:
```
$ aplay -L
default
    Default Audio Device
sysdefault:CARD=PCH
    HDA Intel PCH, ALC236 Analog
    Default Audio Device
surround21:CARD=PCH,DEV=0
    HDA Intel PCH, ALC236 Analog
    2.1 Surround output to Front and Subwoofer speakers
surround40:CARD=PCH,DEV=0
    HDA Intel PCH, ALC236 Analog
    4.0 Surround output to Front and Rear speakers
surround41:CARD=PCH,DEV=0
    HDA Intel PCH, ALC236 Analog
    4.1 Surround output to Front, Rear and Subwoofer speakers
surround50:CARD=PCH,DEV=0
    HDA Intel PCH, ALC236 Analog
    5.0 Surround output to Front, Center and Rear speakers
surround51:CARD=PCH,DEV=0
    HDA Intel PCH, ALC236 Analog
    5.1 Surround output to Front, Center, Rear and Subwoofer speakers
surround71:CARD=PCH,DEV=0
    HDA Intel PCH, ALC236 Analog
    7.1 Surround output to Front, Center, Side, Rear and Woofer speakers
```
And then when a Bluetooth device is connected:
```
$ aplay -L
default
    Default Audio Device
sysdefault:CARD=PCH
    HDA Intel PCH, ALC236 Analog
    Default Audio Device
surround21:CARD=PCH,DEV=0
    HDA Intel PCH, ALC236 Analog
    2.1 Surround output to Front and Subwoofer speakers
surround40:CARD=PCH,DEV=0
    HDA Intel PCH, ALC236 Analog
    4.0 Surround output to Front and Rear speakers
surround41:CARD=PCH,DEV=0
    HDA Intel PCH, ALC236 Analog
    4.1 Surround output to Front, Rear and Subwoofer speakers
surround50:CARD=PCH,DEV=0
    HDA Intel PCH, ALC236 Analog
    5.0 Surround output to Front, Center and Rear speakers
surround51:CARD=PCH,DEV=0
    HDA Intel PCH, ALC236 Analog
    5.1 Surround output to Front, Center, Rear and Subwoofer speakers
surround71:CARD=PCH,DEV=0
    HDA Intel PCH, ALC236 Analog
    7.1 Surround output to Front, Center, Side, Rear and Woofer speakers
bluealsa:DEV=C4:67:B5:37:26:12,PROFILE=sco
    Libratone Zipp Mini HFP (CVSD)
    Bluetooth Audio Input/Output
bluealsa:DEV=C4:67:B5:37:26:12,PROFILE=a2dp
    Libratone Zipp Mini A2DP (aptX)
    Bluetooth Audio Output
```

The last two entries in the above listing have been created by
`bluealsa-autoconfig`.

As an example GUI application, here's a screenshot of the `kodi` audio device
selection dialog with the same Bluetooth speaker connected:

![kodi](https://github.com/borine/bluealsa-autoconfig/assets/32966433/83dda446-6c56-4e01-b61a-d158f386a645)

`bluealsa-autoconfig` can also optionally create a new `default` device
definition that uses a connected BlueALSA device; and it can also optionally
trigger a `udev` audio device change event to notify ALSA applications when
the configuration has changed.The `udev` option is particularly important when
using applications such as `kodi` because otherwise they will not see the
changes to the ALSA configuration and therefore will not display devices
connected after the application started.

In the above `kodi` example, `bluealsa-autoconfig` is running as
```
bluealsa-autoconfig --udev
```
and `/etc/asound.conf` contains:
```
defaults.bluealsa.namehint "%n %p (%c) Bluetooth %s"
```

`bluealsa-autoconfig` must be run as root to gain the necessary privileges
for modifying the global ALSA configuration and triggering udev events. See
the [manual page](./bluealsa-autoconfig.8.rst.in) for full details.

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
or, if the build directory has already been set up, change its configuration with
```
meson configure -Ddoc=true builddir
```

## License

This project is licensed under the terms of the MIT license.

It includes copies of source code files from the bluez-alsa project, which are also licensed under the MIT license and all rights to those files remain with their original author.
