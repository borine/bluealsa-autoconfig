bluez_alsa_sources = [
	'dbus.c',
	'shared/a2dp-codecs.c',
	'shared/dbus-client.c',
	'shared/dbus-client-pcm.c',
	'shared/log.c',
	'shared/rt.c',
]

bluez_alsa_deps = [
	alsa_dep,
	bluez_dep,
	dbus_dep,
	gio_dep,
]

bluez_alsa_includes = [
	'shared',
	'.',
]

bluez_alsa = static_library(
	'bluez-alsa',
	bluez_alsa_sources,
	dependencies: bluez_alsa_deps,
	install: false,
)

bluez_alsa_dep = declare_dependency(
	link_with: bluez_alsa,
	include_directories: bluez_alsa_includes,
	dependencies: bluez_alsa_deps,
)

