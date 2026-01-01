/*
 * bluealsa-autoconfig - bluealsa-hook.c
 * SPDX-FileCopyrightText: 2024-2026 @borine <https://github.com/borine>
 * SPDX-License-Identifier: MIT
 */

#include <alsa/asoundlib.h>
#include <alsa/conf.h>
#include <sys/stat.h>

#include "autoconfig-filepaths.h"

/* libasound locks its config mutex before calling hook functions, so it is
 * safe to store the last modification time of the defaults file in a global
 * static variable. */
static struct timespec last_change = { 0 };

/**
 * @param root the configuration root node (ie bluealsa.autoconfig.dynamic)
 * @param config the config node of the hook (ie bluealsa.autoconfig.@hooks.0)
 * @param dst    address to place the result node (must set this to NULL)
 * @param private_data not used
 */
int bluealsa_autoconfig (
			snd_config_t *root,
			snd_config_t *config,
			snd_config_t **dst,
			snd_config_t *private_data) {
	(void) config;
	(void) private_data;

	snd_config_t *hook_func, *hooks;
	snd_input_t *in;
	struct stat statbuf;

	int ret = 0;

	assert(root && dst);
	*dst = NULL;

	if ((ret = stat(BLUEALSA_AUTOCONFIG_DEFAULTS_FILE, &statbuf)) < 0) {
		SNDERR("Cannot access file %s", BLUEALSA_AUTOCONFIG_DEFAULTS_FILE);
		return ret;
	}

	 if (statbuf.st_mtim.tv_sec == last_change.tv_sec &&
				statbuf.st_mtim.tv_nsec == last_change.tv_nsec)
		goto restore_hook;

	/* delete all child nodes except the hook_func definition */
	if ((ret = snd_config_search(root, "hook_func", &hook_func)) < 0) {
		SNDERR("Invalid BlueALSA autoconfig dynamic hook func");
		return ret;
	}
	snd_config_remove(hook_func);
	snd_config_delete_compound_members(root);
	snd_config_add(root, hook_func);

	/* load the updated default device config */
	ret = snd_input_stdio_open(&in, BLUEALSA_AUTOCONFIG_DEFAULTS_FILE, "r");
	if (ret >= 0) {
		ret = snd_config_load(root, in);
		snd_input_close(in);
	}
	if (ret < 0) {
		SNDERR("Cannot load BlueALSA autoconfig defaults file %s", BLUEALSA_AUTOCONFIG_DEFAULTS_FILE);
		return ret;
	}

	last_change.tv_sec = statbuf.st_mtim.tv_sec;
	last_change.tv_nsec = statbuf.st_mtim.tv_nsec;

restore_hook:

	/* libasound removes the @hooks node from the tree before calling this
	 * function, then deletes it when this function returns. So we must create
	 * a new @hooks node and move the hook config into it so that this
	 * function is called each time the application accesses the autoconfig
	 * dynamic node. */
	if ((ret = snd_config_make_compound(&hooks, "@hooks", 0)) < 0) {
		SNDERR("Cannot create BlueALSA autoconfig @hooks node");
		return ret;
	}
	snd_config_remove(config);
	snd_config_add(hooks, config);
	snd_config_add(root, hooks);

	return 0;
}

SND_DLSYM_BUILD_VERSION(bluealsa_autoconfig, SND_CONFIG_DLSYM_VERSION_HOOK);
