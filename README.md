# BlueALSA ALSA Configuration Service and PCM Event Handler Service

## Introduction

This project maintains two utilities (`bluealsa-autoconfig` and
`bluealsa-agent`) which are designed to help simplify the management of the
Bluetooth audio user experience when using BlueALSA. See
[The bluez-alsa project](https://github.com/arkq/bluez-alsa) for more
information on BlueALSA.

## bluealsa-autoconfig

`bluealsa-autoconfig` is a simple program to add and remove ALSA configuration
nodes for BlueALSA Bluetooth PCM and CTL devices. It can also optionally
simulate `udev` events on Bluetooth audio device connect and disconnect.

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
the configuration has changed. The `udev` option is particularly important when
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

A systemd system service unit file is included for `bluealsa-autoconfig`

## bluealsa-agent

`bluealsa-agent` is a simple program to invoke arbitrary user commands in
response to BlueALSA PCM D-Bus signals. It does not require `root` privileges
and is intended to be used as a user service.

For a trivial example, suppose it is required to display a desktop info pop-up
whenever a BlueALSA PCM is added or removed; one could install the following
shell script as `/usr/local/bin/bluealsa-notify` and then run the
`bluealsa-agent` service as
```
bluealsa-agent /usr/local/bin/bluealsa-notify
```

```
#!/bin/sh

title="BlueALSA PCM"
case "$1" in
	"add")
		title="$title added"
		;;
	"remove")
		title="$title removed"
		;;
	"update")
		title="$title updated"
		;;
	*)
		exit 0
esac

text="dbus path: $2"

[ "$BLUEALSA_PCM_PROPERTY_NAME" ] && text="${text}\nname: $BLUEALSA_PCM_PROPERTY_NAME"
[ "$BLUEALSA_PCM_PROPERTY_ADDRESS" ] && text="${text}\naddress: $BLUEALSA_PCM_PROPERTY_ADDRESS"
[ "$BLUEALSA_PCM_PROPERTY_PROFILE" ] && text="${text}\nprofile: $BLUEALSA_PCM_PROPERTY_PROFILE"
[ "$BLUEALSA_PCM_PROPERTY_MODE" ] && text="${text}\nmode: $BLUEALSA_PCM_PROPERTY_MODE"
[ "$BLUEALSA_PCM_PROPERTY_CODEC" ] && text="${text}\ncodec: $BLUEALSA_PCM_PROPERTY_CODEC"
[ "$BLUEALSA_PCM_PROPERTY_CODEC_CONFIG" ] && text="${text}\ncodec config: $BLUEALSA_PCM_PROPERTY_CODEC_CONFIG"

[ "$BLUEALSA_PCM_PROPERTY_FORMAT" ] && text="${text}\nformat: $BLUEALSA_PCM_PROPERTY_FORMAT"
[ "$BLUEALSA_PCM_PROPERTY_CHANNELS" ] && text="${text}\nchannels: $BLUEALSA_PCM_PROPERTY_CHANNELS"
[ "$BLUEALSA_PCM_PROPERTY_SAMPLING" ] && text="${text}\nsampling: $BLUEALSA_PCM_PROPERTY_SAMPLING"
[ "$BLUEALSA_PCM_PROPERTY_TRANSPORT" ] && text="${text}\ntransport: $BLUEALSA_PCM_PROPERTY_TRANSPORT"
[ "$BLUEALSA_PCM_PROPERTY_TRANSPORT_TYPE" ] && text="${text}\ntransport type: $BLUEALSA_PCM_PROPERTY_TRANSPORT_TYPE"
[ "$BLUEALSA_PCM_PROPERTY_SERVICE" ] && text="${text}\nservice: $BLUEALSA_PCM_PROPERTY_SERVICE"
[ "$BLUEALSA_PCM_PROPERTY_ALSA_ID" ] && text="${text}\nALSA id: $BLUEALSA_PCM_PROPERTY_ALSA_ID"
[ "$BLUEALSA_PCM_PROPERTY_RUNNING" ] && text="${text}\nrunning: $BLUEALSA_PCM_PROPERTY_RUNNING"
[ "$BLUEALSA_PCM_PROPERTY_SOFTVOL" ] && text="${text}\nsoftvol: $BLUEALSA_PCM_PROPERTY_SOFTVOL"
[ "$BLUEALSA_PCM_PROPERTY_CHANGES" ] && text="${text}\nchanges: $BLUEALSA_PCM_PROPERTY_CHANGES"
[ "$BLUEALSA_AGENT_SYSTEMD" ] && text="${text}\nsystemd session: $BLUEALSA_AGENT_SYSTEMD"

exec /usr/bin/zenity --info --title="$title" --text="$text" -
```
See the [manual page](./bluealsa-agent.8.rst.in) for full details.

Some more practical example scripts are included in the [examples directory](./examples/examples.md)

## Installation

```
meson setup builddir
cd builddir
meson compile
sudo meson install
```

To include the manual pages, the application `rst2man` from the `docutils` project is required. Many distributions provide this in the package `python3-docutils`. To enable building and installing the manual page, setup the build directory with
```
meson setup -Ddoc=true builddir
```
or, if the build directory has already been set up, change its configuration with
```
meson configure -Ddoc=true builddir
```

## Usage

The two services are documented in their respective manual pages: [bluealsa-autoconfig](./bluealsa-autoconfig.8.rst.in) and [bluealsa-agent](./bluealsa-agent.8.rst.in). `systemd` configuration is documented in [systemd integration](./systemd-integration.md).

## License

This project is licensed under the terms of the MIT license.

It includes copies of source code files from the bluez-alsa project, which are also licensed under the MIT license and all rights to those files remain with their original author.
