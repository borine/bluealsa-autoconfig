# Example Agent Scripts

## Single Command

### 51 Notify

An agent script must be a "one-shot" program that does not block or linger. Each invocation is a new process. The simplest ones merely update a file or send some message in response to one BlueALSA PCM event. The first example is a very simple program to generate a desktop notification whenever a BlueALSA PCM connects or disconnects. It uses the GTK+ dialog
tool `zenity` to create the notification. To try this example, either copy (or symlink) the file [51-notify.sh](./51-notify.sh) to your `bluealsa-agent` scripts directory, ensure it has execute permission (`chmod a+x 51-notify.sh`), then re-start the `bluealsa-agent` service.

## Client-Server

### 52 MPD

This example demonstrates the use of `bluealsa-agent` to invoke the client of a client-server application, in this case [MPD](https://www.musicpd.org) and its command-line client `mpc`. As for the previous example, it consists of a single file (this time a `bash` script [52-mpd.bash](./52-mpd.bash)) which must be made executable and be added to the `bluealsa-agent` commands. The script operates only for A2DP streams and ignores HFP and HSP devices.

For use with Bluetooth headphones or speakers, it is necessary to have at least 2 ALSA output devices defined in the `mpd.conf` configuration file, one for local soundcard playback and the other for Bluetooth playback. The script assumes that MPD Output 1 is the soundcard, and MPD Output 2 is the Bluetooth output. Edit the values of `CARD_OUTPUT` and `BLUETOOTH_OUTPUT` at the top of the script if you require to use different output numbers. Example `mpd.conf` entries:

```mpd
audio_ouput {
	name "Default"
	type "alsa"
	device "default"
	enabled "yes"
}

audio_output {
	name "Bluetooth"
	type "alsa"
	device "bluealsa:PROFILE=a2dp"
	mixer_type "software"
	enabled "no"
}
```

The `bluealsa` output mixer_type is set to "software" because MPD is not fully compatible with removable ALSA devices and in particular the use of "hardware" mixer_type here can cause MPD to trigger an abort in libasound. This output is also initially "disabled" so that it is only used when a bluetooth device is actually connected.

When a bluetooth speaker or headphones connects the A2DP profile, the script pauses playback,enables the "Bluetooth" output and disables the "Default" output, then resumes playback if MPD was already playing when the connection occurred.

When a Bluetooth output device disconnects then MPD pauses playback immediately, before the agent script is invoked. So the script cannot tell if playback was already paused before the disconnection and therefore does not automatically resume playback after enabling the "Default" output and disabling the "Bluetooth" device. The user must manually resume using any MPD client (e.g. `mpc play`).

This example agent script also enables the use of `MPD` as a player for A2DP streams from a connected mobile phone. This feature requires MPD built from the latest sources (https://github.com/MusicPlayerDaemon/MPD) as the ALSA input plugin in MPD releases v0.23.15 and earlier does not work well with non-hardware devices. [The important commit for BlueALSA capture support is [4947bb1](https://github.com/MusicPlayerDaemon/MPD/commit/4947bb113d26045f5fa0e5800db73acd75500109)]. To use this feature it is necessary to run `bluealsa-agent` with status change events enabled (`bluealsa-agent --status`).

When an A2DP input stream is **started** (i.e., not when the 'phone connects, only when it begins audio playback) the agent script pauses `mpd`, inserts the A2DP stream into the playqueue, and starts playback of the stream.

When the A2DP stream is stopped the agent script removes the A2DP stream from the queue and resumes playback of the queue from where it was paused.

## Long-Running Service

In the next two examples we have a service to transfer HFP or HSP audio streams from a BlueALSA PCM to a locally attached speaker, and from a locally attached microphone to a BlueALSA PCM. This is a basic requirement of a HFP Handsfree device or a HSP Headset. The example service is in fact a simple `bash` script wrapping `alsaloop`. To use this service, the script [handsfreee.bash](./handsfree/handsfree.bash) must be run by a normal user who is a member of the `audio` group. The script uses `hw:0,0` as the speaker and `plughw:0,0` as the microphone. Change the values of the variables at the top of the file if you wish to use some other device(s). Note that the speaker audio parameters must be defined: choose values that are natively supported by the hardware (the `bluealsa` PCM plugin will convert the stream to the given values). The microphone device should support the Bluetooth stream parameters, so use `plug` in this device if necessary. Avoid the use of `dmix` and `dsnoop` in both devices as these plugins can cause problems for `alsaloop`. As `default` often includes `dmix` and/or `dsnoop` it too should be avoided.

Both the agent script examples assume that the service script is installed as `/usr/local/bin/handsfree.bash`. The two examples are exclusive alternatives, and as such only one at a time may be installed. The BlueALSA daemon command line must include `-p hfp-hf -p hsp-hs` and the `bluealsa-agent` command line must include `--status`.

### 53 Handsfree (Direct Start)

It is possible to launch the service executable directly from an agent script so long as the service is moved to a different process session so that the agent is not blocked while the service is running, for example by using `setsid(1)`. In order to be able to later stop the service it is necessary to write its process ID (pid) to a file so that a later invocation of the agent knows which process to kill.

To try this example, add the agent script [53-handsfree.bash](./handsfree/53-handsfree.bash) to the `bluealsa-agent` commands.

### 54 Handsfree (Via Systemd)

This example uses `systemd` to manage the `handfree.bash` service. This is preferable to the direct control method as it is more robust and is the recommended approach for all long-running service management. To use this example, first remove the above example `53-handsfree.bash` from the `bluealsa-agent` commands if you have installed it. Copy the systemd service template file [hf-hs@.service](./handsfree/hf-hs@.service) to the directory /usr/local/lib/systemd/user (create that path if necessary) then inform the user `systemd` instance of the change `systemctl --user daemon-reload`. Do not `enable` the service, it is to be started only explicitly by the agent script. Now add the agent script [54-handsfree.bash](./handsfree/54-handsfree.bash) to the `bluealsa-agent` commands.

