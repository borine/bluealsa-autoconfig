project('bluealsa-autoconfig', 'c', version: '0.0.1', license: 'MIT')

prefix = get_option('prefix')
assert(prefix.startswith('/'), 'Prefix is not absolute: "@0@"'.format(prefix))

systemd = dependency('systemd', required: false)
bashcompletion = dependency('bash-completion', required : false)

bindir = join_paths(prefix, get_option('bindir'))
mandir = join_paths(prefix, get_option('mandir'))
alsaconfdir = '/etc/alsa/conf.d'

if systemd.found()
	unitdir = join_paths(prefix, 'lib', 'systemd', 'system')
endif

conf_data = configuration_data()
conf_data.set('prefix', prefix)
conf_data.set('bindir', bindir)

version_conf = configuration_data()
version_conf.set_quoted('PACKAGE_VERSION', meson.project_version())
configure_file(output: 'version.h', configuration: version_conf)

compiler = meson.get_compiler('c')

#alsa_dep = dependency('alsa', version: '>= 1.2.5')
alsa_dep = dependency('alsa')
bluez_dep = dependency('bluez')
dbus_dep = dependency('dbus-1')

subdir('bluez-alsa')

autoconfig_sources = [
	'autoconfig.c',
	'bluealsa-client.c',
	'namehint.c',
]

autoconfig = build_target(
	'bluealsa-autoconfig',
	autoconfig_sources,
	target_type: 'executable',
	dependencies: bluez_alsa_dep,
	install: true,
	install_dir: bindir,
)

install_data(
	'21-bluealsa-autoconfig.conf',
	install_dir: alsaconfdir,
	install_mode: ['rw-r--r--', 'root', 'root']
)

if systemd.found()
	unitdir = join_paths(prefix, 'lib', 'systemd', 'system')
	configure_file(
		input : 'bluealsa-autoconfig.service.in',
		output : 'bluealsa-autoconfig.service',
		configuration : conf_data,
		install: true,
		install_dir: unitdir
	)
endif

if get_option('doc')
  custom_target(
    'documentation',
    output: 'bluealsa-autoconfig.8',
    input: 'bluealsa-autoconfig.8.rst',
	command: ['rst2man', '@INPUT@', '@OUTPUT@'],
    build_by_default: true,
    install: true,
    install_dir: join_paths(mandir, 'man8'),
  )
endif

if bashcompletion.found()
    completionsdir = bashcompletion.get_pkgconfig_variable('completionsdir')
	install_data(
		'completion',
		rename: 'bluealsa-autoconfig',
		install_dir: completionsdir,
	)
endif
