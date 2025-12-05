/*
 * bluealsa-autoconfig - bluealsa-hook.c
 * SPDX-FileCopyrightText: 2024-2025 @borine <https://github.com/borine>
 * SPDX-License-Identifier: MIT
 */

#include <alsa/asoundlib.h>
#include <alsa/conf.h>

#define AUTOCONFIG_DATA_FILE "/run/bluealsa-autoconfig/defaults.conf"

/**
 * Restore the bluealsa autoconfig hook.
 * libasound detaches the @hooks node from the root tree before calling this
 * function, then deletes it when we return. So we need to create a new
 * copy and add that in place of the original to ensure our hook is invoked each
 * time the application opens the autoconfig node. */
static void restore_hook_func(snd_config_t *root) {
	snd_config_t *hooks = NULL, *hook0, *node;
	int err;

	if ((err = snd_config_make_compound(&hooks, "@hooks", 0)) < 0)
		goto fail;

	if ((err = snd_config_make_compound(&hook0, "0", 0)) < 0)
		goto fail;
	if ((err = snd_config_add(hooks, hook0)) < 0) {
		snd_config_delete(hook0);
		goto fail;
	}

	if ((err = snd_config_imake_string(&node, "func", "bluealsa_autoconfig")) < 0 )
		goto fail;
	if ((err = snd_config_add(hook0, node)) < 0) {
		snd_config_delete(node);
		goto fail;
	}

	if ((err = snd_config_add(root, hooks)) < 0)
		goto fail;

	return;

fail:
	SNDERR("Cannot re-create BlueALSA autoconfig hook_func: %s", snd_strerror(err));
	if (hooks != NULL)
		snd_config_delete(hooks);
}

/**
 * @param root the configuration root node (ie bluealsa.autoconfig)
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

	snd_config_t *def;
	snd_input_t *in;
	int ret = 0;

	assert(root && dst);
	*dst = NULL;

	if ((ret = snd_config_search(root, "default", &def)) == 0) {
		if (snd_config_get_type(def) != SND_CONFIG_TYPE_COMPOUND) {
			SNDERR("Invalid BlueALSA autoconfig default node");
			return -EINVAL;
		}
		snd_config_delete_compound_members(def);
	}
	else {
		if ((ret = snd_config_make_compound(&def, "default", 0)) < 0) {
			SNDERR("Cannot create BlueALSA autoconfig default node");
			return ret;
		}
		if ((ret = snd_config_add(root, def)) < 0) {
			SNDERR("Cannot create BlueALSA autoconfig default node");
			snd_config_delete(def);
			return ret;
		}
	}

	restore_hook_func(root);

	ret = snd_input_stdio_open(&in, AUTOCONFIG_DATA_FILE, "r");
	if (ret >= 0) {
		ret = snd_config_load(def, in);
		snd_input_close(in);
	}
	if (ret < 0) {
		SNDERR("Cannot load BlueALSA autoconfig defaults file %s", AUTOCONFIG_DATA_FILE);
		return ret;
	}

	return 0;
}

SND_DLSYM_BUILD_VERSION(bluealsa_autoconfig, SND_CONFIG_DLSYM_VERSION_HOOK);
