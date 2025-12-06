# bluealsa-autoconfig - systemd/meson.build
# SPDX-FileCopyrightText: 2024-2025 @borine <https://github.com/borine/>
# SPDX-License-Identifier: MIT

unitdir = prefix / 'lib' / 'systemd'

autoconfig_unit_template = files('bluealsa-autoconfig.service.in')
agent_unit_template = files('bluealsa-agent.service.in')

subdir('system')
subdir('user')

