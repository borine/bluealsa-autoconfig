
configure_file(
	input : autoconfig_unit_template,
	output : 'bluealsa-autoconfig.service',
	configuration : conf_data,
	install: true,
	install_dir: join_paths(unitdir, 'system'),
)

conf_data = configuration_data()
conf_data.set('bindir', bindir)
conf_data.set('ENVIRONMENT_FILE', '')
conf_data.set('USER', 'User=bluealsa-agent')
conf_data.set('GROUP', 'Group=audio')
conf_data.set('ARGS', '%S/bluealsa-agent/commands')

agent_system_unit = configure_file(
	input : agent_unit_template,
	output : 'bluealsa-agent.service',
	configuration : conf_data,
	install: true,
	install_dir: join_paths(unitdir, 'system'),
)

