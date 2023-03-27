===================
bluealsa-autoconfig
===================

------------------------------------------
Bluetooth Audio ALSA Configuration Manager
------------------------------------------

:Date: March 2023
:Manual section: 8
:Manual group: System Manager's Manual
:Version: $VERSION$

SYNOPSIS
========

**bluealsa-autoconfig** [*OPTION*] ...

DESCRIPTION
===========

**bluealsa-autoconfig** is a simple program to add and remove ALSA
configuration nodes for **BlueALSA** Bluetooth PCM and CTL devices. It can also
optionally simulate **udev** events on Bluetooth audio device connect and
disconnect.

OPTIONS
=======

-h, --help
    Output a usage message and exit.

-V, --version
    Output the version number and exit.

-B NAME, --dbus=NAME
    BlueALSA service name suffix. This option can be given more than once to
    add support for multiple ``bluealsa(8)`` service instances. The default
    service **org.bluealsa** is always included. For example, the following
    will include into the ALSA configuration PCMs from **org.bluealsa**,
    **org.bluealsa.sink** and **org.bluealsa.source**:
    ::

        bluealsa-autoconfig -B sink -B source

    For more information see the ``--dbus`` option of the ``bluealsa(8)``
    service daemon.

-u, --udev
    Emit a synthesized ``udev`` event on BlueALSA device connect and
    disconnect. The event is signalled *after* all the associated ALSA
    configuration changes have been committed.

    This option may be useful for some applications (e.g. ``kodi(1)``) that do
    not refresh their audio device list unless a soundcard change is signalled
    via ``udev``.

-d, --default
    Include a definition of a PCM that can be used as the ALSA **default** PCM
    and similarly a CTL. These definitions use a BlueALSA bluetooth device when
    connected or a fallback to a soundcard device otherwise. See
    `AUTOMATIC DEFAULT`_ below.

OPERATION
=========

The program must be run as ``root`` to gain the necessary privileges for
modifying the global ALSA configuration and triggering udev events.

For example, ``aplay -L`` might produce output such as:
::

    bluealsa:DEV=XX:XX:XX:XX:XX:XX,PROFILE=a2dp,SRV=org.bluealsa
        Jabra MOVE v2.3.0 A2DP (aptX)
        Bluetooth Audio Output

The generated configuration consists of ``namehint`` nodes which associate a
description with each connected BlueALSA PCM. The content of the description
is configurable - see `DESCRIPTION TEMPLATE`_ below.

DESCRIPTION TEMPLATE
====================

The text used in a PCM description is defined by a key in the global ALSA
configuration: ``defaults.bluealsa.namehint``. The value is a string which can
contain substitutions that are expanded for each PCM. The available
substitutions are:
::

    %a	bluetooth address in format "XX:XX:XX:XX:XX:XX"
    %c	codec
    %l	newline
    %n	device name (alias)
    %p	profile
    %s	stream direction ("Input" | "Output")
    %%	literal '%'

The text can contain at most one newline which applications such as `aplay`
will use to break the displayed description over two lines. However, be aware
that some other applications, particularly GUI applications, do not always
render the newline very well.

The default description definition is:
::

    defaults.bluealsa.namehint "%n %p (%c)%lBluetooth Audio %s"

Note the newline in this default.

``bluealsa-autoconfig`` reads the definition of this key only once, on startup.
So the program will have to be re-started after editing this key before any
change takes effect. The key should be defined in a system-wide configuration
file, it will not be read from a user's ``~/.asoundrc`` file. It is recommended
to use ``/etc/asound.conf`` for this purpose.

AUTOMATIC DEFAULT
=================

``bluealsa-autoconfig`` can optionally create an ALSA PCM definition suitable
for use as the user's ``default`` PCM. When enabled with the ``--default``
option, a user can choose to have the most recently connected BlueALSA device
as the ``default`` PCM by overriding the default in the ``~/.asoundrc`` file
as:
::

    pcm.!default bluealsa.pcm.default

The definition of this PCM varies according to whether any BlueALSA devices
are connected and which profiles and stream directions they provide. It will
"fallback" to the system default, or a PCM of the user's choosing, if no
matching BlueALSA PCM is currently connected. If more than one BlueALSA device
is available, then the most recently connected is chosen.

Note that this PCM does not re-direct a stream while in use - an application
will use the definition in place at the time it opens the PCM, and must close
that PCM then re-open the ``default`` to see any changes.

Three user-definable keys are provided to permit per-user customization of the
default behaviour. A user can define these in the ``~/.asoundrc`` file.

defaults.bluealsa.default.profile
    This key determines the profile that will be used for the ALSA ``default``
    PCM. It defaults to ``defaults.bluealsa.profile`` - i.e. it uses the same
    value as the ``bluealsa`` PCM default. If, for some reason, the user
    requires the profile for ``pcm.default`` to be different from the one for
    ``pcm.bluealsa``, then the above key can be used. Permitted values
    are ``a2dp`` or ``sco``.

defaults.bluealsa.default.stream
    This key determines which stream direction(s) are assigned to BlueALSA PCMs
    for the ``default`` PCM. Permitted values are ``capture``, ``playback`` or
    ``duplex``. The default is ``duplex``, which will use BlueALSA for both
    directions if both are available, or just one if only one is available.
    Note that A2DP streams (except SBC ``faststream``) generally have only one
    direction.

defaults.bluealsa.default.fallback
    This key determines which PCM will be used as a fallback for whichever
    stream direction(s) is/are not available as connected BlueALSA PCMs. For
    example to use the second soundcard with its default setup:
    ::

        defaults.bluealsa.default.fallback "sysdefault:CARD=1"

    The default value is ``sysdefault`` which is normally the system-defined
    default.

When used with ``alsa-lib`` release 1.2.5 or later, ``bluealsa-autoconfig``
also creates a ALSA CTL suitable for use as a user's default mixer. This CTL is
a ``single-device`` mixer (except in one case, see below), and contains
controls for the ``bluealsa.pcm.default`` PCM also created by this option. To
use this CTL, a user must override the default in the ``~/.asoundrc`` file as:
::

    ctl.!default bluealsa.ctl.default

``alsa-lib`` does not support ``asym`` type CTLs, so it is not possible to have
a single mixer that has controls from different devices for capture and
playback. Therefore if this default CTL is selected it is not possible to have
the system default controls for capture combined with the BlueALSA controls for
playback.

In the case that both playback and capture BlueALSA PCMs are connected, but
from different devices, then the mixer ``bluealsa.ctl.default`` will be in its
default mode, and will show controls for **all** connected BlueALSA devices.
See ``bluealsa-plugins(7)`` for more information on BlueALSA CTL default and
single-device modes.

LIBASOUND VERSION DEPENDENCY
============================

ALSA ``alsa-lib`` introduced the use of ``/var/lib/alsa/conf.d/`` as a standard
directory for dynamically added configuration files in release ``1.2.5``.
From that release onward, the ALSA configuration will automatically include
definitions from all ``.conf`` files in that directory.

**bluealsa-autoconfig** uses that directory to store its dynamic configuration
definitions, so that **bluealsa** Bluetooth devices automatically appear to
ALSA applications.

For earlier ALSA libasound releases, that directory is not automatically read
by default, so it is necessary to explicitly inform libasound to read the
**bluealsa-autoconfig** configuration file. This can be achieved by adding an
``include`` directive to ``/etc/asound.conf``:
::

    </var/lib/alsa/conf.d/bluealsa-autoconfig.conf>

Note that it is an error if an included file does not exist, so it is necessary
to ensure that **bluealsa-autoconfig** is running before using any ALSA
applications when using the include method.

SEE ALSO
========

``bluealsa(8)``, ``bluealsa-plugins(7)``, ``udev(7)``

Project web site
  https://github.com/borine/bluealsa-autoconfig
