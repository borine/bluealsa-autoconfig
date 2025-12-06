# bluealsa-autoconfig - systemd/system/meson.build
# SPDX-FileCopyrightText: 2024-2025 @borine <https://github.com/borine/>
# SPDX-License-Identifier: MIT

configure_file(
	input : autoconfig_unit_template,
	output : 'bluealsa-autoconfig.service',
	configuration : conf_data,
	install: true,
	install_dir: unitdir / 'system',
)

conf_data = configuration_data()
conf_data.set('bindir', bindir)
conf_data.set('systemd_session', 'SYSTEM')
conf_data.set('ENVIRONMENT_FILE', '')
conf_data.set('ARGS', '%S/bluealsa-agent/commands')
conf_data.set('USER', 'User=bluealsa-agent')
conf_data.set('GROUP', 'Group=audio')
conf_data.set('SYSTEM_DIRECTIVES',
'''CapabilityBoundingSet=
ProtectClock=true
ProtectHostname=true
ProtectKernelLogs=true
ProtectKernelModules=true''')
agent_system_unit = configure_file(
	input : agent_unit_template,
	output : 'bluealsa-agent.service',
	configuration : conf_data,
	install: true,
	install_dir: unitdir / 'system',
)

