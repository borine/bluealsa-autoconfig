project('bluealsa-autoconfig', 'c', version: '0.4.5', license: 'MIT')

prefix = get_option('prefix')
assert(prefix.startswith('/'), 'Prefix is not absolute: "@0@"'.format(prefix))

bashcompletion = dependency('bash-completion', required: false)

bindir = join_paths(prefix, get_option('bindir'))
mandir = join_paths(prefix, get_option('mandir'))
docdir = join_paths(prefix, get_option('datadir'), 'doc')
alsaconfdir = '/etc/alsa/conf.d'

conf_data = configuration_data()
conf_data.set('prefix', prefix)
conf_data.set('bindir', bindir)

version_conf = configuration_data()
version_conf.set_quoted('PACKAGE_VERSION', meson.project_version())
version_h = configure_file(output: 'version.h', configuration: version_conf)

compiler = meson.get_compiler('c')

alsa_dep = dependency('alsa')
bluez_dep = dependency('bluez')
dbus_dep = dependency('dbus-1')
gio_dep = dependency('gio-unix-2.0')

subdir('bluez-alsa')

autoconfig_sources = [
	version_h,
	'alsa.c',
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

agent_sources = [
	version_h,
	'agent.c',
	'bluealsa-client.c',
]

agent = build_target(
	'bluealsa-agent',
	agent_sources,
	target_type: 'executable',
	dependencies: bluez_alsa_dep,
	install: true,
	install_dir: bindir,
)

alsa_plugin_dir = join_paths(
	alsa_dep.get_pkgconfig_variable('libdir'),
	'alsa-lib'
)

asound_module_sources = [
	'bluealsa-hook.c',
]

asound_module = shared_library(
	'asound_module_hook_bluealsa_autoconfig',
	asound_module_sources,
	dependencies: [ alsa_dep ],
	c_args: '-DPIC',
	install: true,
	install_dir: alsa_plugin_dir,
)

install_data(
	'21-bluealsa-autoconfig.conf',
	install_dir: alsaconfdir,
	install_mode: ['rw-r--r--', 'root', 'root']
)

subdir('systemd')

completionsdir = bashcompletion.get_pkgconfig_variable('completionsdir')
install_data(
	'completion',
	rename: 'bluealsa-autoconfig',
	install_dir: completionsdir,
)
dst = join_paths(completionsdir, 'bluealsa-agent')
cmd = 'ln -fs @0@ $DESTDIR@1@'.format('bluealsa-autoconfig', dst)
meson.add_install_script('sh', '-c', cmd)

if get_option('doc')
    autoconf_manual_source = configure_file(
        input: 'bluealsa-autoconfig.8.rst.in',
        output: 'bluealsa-autoconfig.8.rst',
        configuration: configuration_data({'VERSION' : meson.project_name() + ' ' + meson.project_version()})
    )
    custom_target(
        'autoconfig_documentation',
        output: 'bluealsa-autoconfig.8',
	    input: autoconf_manual_source,
	    command: ['rst2man', '@INPUT@', '@OUTPUT@'],
        build_by_default: true,
        install: true,
        install_dir: join_paths(mandir, 'man8'),
    )
    agent_manual_source = configure_file(
        input: 'bluealsa-agent.8.rst.in',
        output: 'bluealsa-agent.8.rst',
        configuration: configuration_data({'VERSION' : meson.project_name() + ' ' + meson.project_version()})
    )
    custom_target(
        'agent_documentation',
        output: 'bluealsa-agent.8',
	    input: agent_manual_source,
	    command: ['rst2man', '@INPUT@', '@OUTPUT@'],
        build_by_default: true,
        install: true,
        install_dir: join_paths(mandir, 'man8'),
    )
endif
install_subdir(
	'examples',
	install_dir: join_paths(docdir, 'bluealsa-agent')
)
