# Systemd Integration

## bluealsa-autoconfig

`bluealsa-autoconfig` must be run as `root` in order to be able to update the global ALSA configuration, and also to be able to generate `udev` events. The included service unit file achieves this with a comprehensive security sandbox, such that the security exposure risk of running as root is minimized. To use this service, simply enable it to start at boot:

```
sudo systemctl daemon-reload
sudo systemctl enable --now bluealsa-autoconfig.service
```

## bluealsa-agent

This service is designed to be run by an unprivileged user account so that the risk incurred by any command script is minimized. However, if you need to run a script as `root` for some reason, then that can be done; see below for more details.

This project includes `systemd` service unit files for both a `system` service and a `user` service. The files include security lockdown directives; however as the functionality of the agent scripts is unknown, the security sandbox is a compromise permitting the "most likely" functions. It may be necessary to relax some of the directives for certain use cases; in which case please consult the [systemd documentation](https://www.freedesktop.org/software/systemd/man/latest/systemd.service.html).

### Running in the session of a logged-in user

This is the simplest case. To use this method:

1. create the directory `~/.config/bluealsa-agent/commands/` in which to place command scripts and then copy or symlink the required scripts into that directory.

1. create the file `~/.config/bluealsa-agent/options.conf` defining the environment variable `OPTIONS`, which sets the command-line options for `bluealsa-agent`, for example:

    ```
    OPTIONS="--status --profile=A2DP"
    ```

   _[Note] Alternatively, change the service settings using `systemctl --user edit bluealsa-agent.service`. That way you can also change the commands directory path._

1. enable the service to start at login:

    ```
    systemctl --user daemon-reload
    systemctl --user enable bluealsa-agent.service
    ```

1. start the service:

    ```
    systemctl --user start bluealsa-agent.service
    ```

### Running as an unprivileged system service

BlueALSA is often used on headless, small, servers with no login (other than for maintenance). For these systems we can use `bluealsa-agent` as a system service which starts at boot.

1. The installed systemd system service unit assumes a user account called `bluealsa-agent`, in group `audio`, which you will need to create. The account should be a "service" account with login disabled and password disabled. Each Linux distribution has its own method for account management, so if needed you should seek advice from your distribution. For example, on Debian and derivatives:

    ```
    sudo adduser --system --group --home /var/lib/bluealsa-agent bluealsa-agent
    sudo gpasswd --add bluealsa-agent audio
    ```

1. Create the directory in which to place the agent command scripts:

    ```
    sudo mkdir -p /var/lib/bluealsa-agent/commands/
	sudo chown bluealsa-agent:bluealsa-agent /var/lib/bluealsa-agent/commands/
    ```

1. Copy (or symlink) the required command scripts to that directory.

1. The provided system service unit file does not apply any command-line options. To change the `bluealsa-agent` command line, create a unit override to clear the default command line and create a new command line:

    ```
    sudo systemctl edit bluealsa-agent.service
    ```

    For example, to add `--status` to the command line options:

	```
    [Service]
	ExecStart=
	ExecStart=/usr/local/bin/bluealsa-agent --status %S/bluealsa-agent/commands
    ```

1. Enable the service to start at boot:

    ```
	sudo systemctl daemon-reload
	sudo systemctl enable bluealsa-agent.service
    ```

1. Start the service:

    ```
    sudo systemctl start bluealsa-agent.service
    ```

With this method the scripts run as user `bluealsa-agent`, but with no associated user instance of `systemd` nor any user session bus for D-Bus. If either of those are required, please consider the next method:

### Running as a user service without login

Occasionally, it is required that an agent command script must start and stop a service managed by `systemd` (see for example [54 Handsfree (via systemd)](https://github.com/borine/bluealsa-autoconfig/blob/main/examples/examples.md#54-handsfree-via-systemd). However, `systemd` does not permit an unprivileged user to start or stop system services. So we have two options, either run the agent command script as root (and therefore run `bluealsa-agent` as root), or run the target service as a `systemd` user service. The second option is the most secure. It is possible to run `bluealsa-agent` as a `systemd` user service without needing to login, by using `loginctl`; and in this way a user `systemd` service and D-Bus session bus are also automatically available.

For `bluealsa-agent` we create a "system" user, just as described in [Running as an unprivileged system service](#Running-as-an-unprivileged-system-service) above; but here we use the installed **user** service unit described in [Running in the session of a-logged-in user](#Running-in-the-session-of-a-logged-in-user) above. The user service unit file uses `%E/bluealsa-agent` as the basedir for the `commands` directory and `args.conf` file, which here expand to `/var/lib/bluealsa-agent/.config/bluealsa-agent/commands` and `/var/lib/bluealsa-agent/.config/bluealsa-agent/args.conf`. You may prefer to create an override to simplify those paths. `systemctl edit` does not allow root to edit a user's unit files, so we must construct the override file manually, for example:

```
sudo mkdir /var/lib/bluealsa-agent/commands
sudo mkdir -p /var/lib/bluealsa-agent/.config/systemd/user/default.target.wants/
sudo mkdir -p /var/lib/bluealsa-agent/.config/systemd/user/bluealsa-agent.service.d/
sudo tee /var/lib/bluealsa-agent/.config/systemd/user/bluealsa-agent.service.d/override.conf >/dev/null <<EOF
[Service]
ExecStart=
ExecStart=/usr/local/bin/bluealsa-agent --status /var/lib/bluealsa-agent/commands
EOF
sudo chown -R bluealsa-agent:bluealsa-agent /var/lib/bluealsa-agent
```

Copy or symlink the required agent command scripts to the directory `/var/lib/bluealsa-agent/commands` and make sure they are readable and executable by the user `bluealsa-agent`. The simplest way is just to make them readable and executable by all users, for example

```
sudo chmod a+rx /var/lib/bluealsa-agent/commands/*
```

Now enable this user service to start at boot (this command will also start the service immediately)

```
sudo systemctl --user -M bluealsa-agent@ daemon-reload
sudo systemctl --user -M bluealsa-agent@ enable bluealsa-agent.service
sudo loginctl enable-linger bluealsa-agent
```

We can check that the service has started correctly with:

```
sudo loginctl user-status bluealsa-agent
bluealsa-agent (106)
           Since: Thu 2024-06-27 08:02:25 BST; 2min ago
           State: lingering
          Linger: yes
            Unit: user-106.slice
                  └─user@106.service
                    ├─app.slice
                    │ └─bluealsa-agent.service
                    │   └─1113 /usr/local/bin/bluealsa-agent --status /var/lib/bluealsa-agent/commands
                    ├─init.scope
                    │ ├─1097 /lib/systemd/systemd --user
                    │ └─1098 "(sd-pam)"
                    └─session.slice
                      └─dbus.service
                        └─1123 /usr/bin/dbus-daemon --session --address=systemd: --nofork --nopidfile --systemd-activation --syslog-only
```

> [!Note]
> If your distribution is intended as a desktop system, then you may find that the bluealsa-agent session fails because some of the autostart user units fail when started outside a login session. In that case it is necessary to mask each failed unit. For example:
>
> ```
> sudo systemctl --user -M bluealsa-agent@ mask speech-dispatcher.socket pipewire.socket pipewire.service pulseaudio.socket pulseaudio.service
> ```
>
> Some units may be started by `systemd` "user-generators", such as `systemd-xdg-autostart-generator`. These cannot be masked by `systemctl`, but can be masked manually. If needed, create a directory:
>
> ```
> sudo mkdir -p /var/lib/bluealsa-agent/.config/systemd/user-generators
> ```
>
> Then symlink each failing user-generator to `/dev/null`; for example:
>
> ```
> sudo ln -s /dev/null /var/lib/bluealsa-agent/.config/systemd/user-generators/systemd-xdg-autostart-generator
> ```
>
> Always reload the systemd daemon config after making manual changes:
>
> ```
> sudo systemctl --user -M bluealsa-agent@ daemon-reload
> ```

If you need to stop or re-start the `bluealsa-agent` service manually, use:

```
# stop the service manually (also disables start at boot)
sudo loginctl disable-linger bluealsa-agent
# start the service manually (also enables start at boot)
sudo loginctl enable-linger bluealsa-agent
```

So now a `bluealsa-agent` command script can start and stop any **user** service which will then run as user `bluealsa-agent`, it can also interact with user D-Bus services that are accessible via the D-Bus session bus.

### Running as a root system service

If it is necessary for an agent command script to be run as root (for example to start or stop a systemd **system** service), then `bluealsa-agent` must be run as `root`. This not recommended, and the user should first consider whether there is some other way of achieving the desired objective.

To modify the `bluealsa-agent` system service unit to run as `root`, create an override to cancel the `User=` directive, for example by using

```
sudo systemctl edit bluealsa-agent

[Service]
User=

```

The security sandbox directives included in the provided `bluealsa-agent` system service unit file are designed for use with a non-privileged user, so it is strongly advised to review them and restrict any function or capability that is not required by the agent script(s).
