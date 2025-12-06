# bluealsa-autoconfig - meson.build
# SPDX-FileCopyrightText: 2024-2025 @borine <https://github.com/borine/>
# SPDX-License-Identifier: MIT

project('bluealsa-autoconfig', 'c', version: '0.5.1', license: 'MIT')

prefix = get_option('prefix')
assert(prefix.startswith('/'), 'Prefix is not absolute: "@0@"'.format(prefix))

bashcompletion = dependency('bash-completion', required: false)

bindir = prefix /  get_option('bindir')
mandir = prefix / get_option('mandir')
docdir = prefix / get_option('datadir') / 'doc'
alsaconfdir = '/etc/alsa/conf.d'
alsadatadir = '/usr/share/alsa/alsa.conf.d'

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

alsa_plugin_dir = alsa_dep.get_variable(pkgconfig : 'libdir') / 'alsa-lib'

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
	install_dir: alsadatadir,
	install_mode: ['rw-r--r--', 'root', 'root']
)

install_symlink(
	'21-bluealsa-autoconfig.conf',
	install_dir : alsaconfdir,
	pointing_to : alsadatadir / '21-bluealsa-autoconfig.conf',
)

subdir('systemd')

completionsdir = bashcompletion.get_variable(pkgconfig : 'completionsdir')

install_data(
	'completion',
	rename: 'bluealsa-autoconfig',
	install_dir: completionsdir,
)

install_symlink(
	'bluealsa-agent',
	install_dir : completionsdir,
	pointing_to : 'bluealsa-autoconfig',
)

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
        install_dir: mandir / 'man8',
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
        install_dir: mandir / 'man8',
    )
endif
install_subdir(
	'examples',
	install_dir: docdir / 'bluealsa-agent'
)
