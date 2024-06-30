
conf_data = configuration_data()
conf_data.set('bindir', bindir)
conf_data.set('ENVIRONMENT_FILE', 'EnvironmentFile=%E/bluealsa-agent/args.conf')
conf_data.set('USER', '')
conf_data.set('GROUP', '')
conf_data.set('ARGS', '$OPTIONS $COMMAND')

agent_user_unit = configure_file(
	input : agent_unit_template,
	output : 'bluealsa-agent.service',
	configuration : conf_data,
	install: true,
	install_dir: join_paths(unitdir, 'user'),
)

