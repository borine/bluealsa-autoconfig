/*
 * bluealsa-autoconfig - namehint.c
 * Copyright (c) 2023 @borine (https://github.com/borine/)
 *
 * This project is licensed under the terms of the MIT license.
 *
 */

#include <alsa/asoundlib.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "alsa.h"
#include "namehint.h"
#include "bluealsa-client.h"

typedef enum {
	STREAM_CAPTURE = BA_PCM_MODE_SOURCE,
	STREAM_PLAYBACK = BA_PCM_MODE_SINK,
	STREAM_DUPLEX = BA_PCM_MODE_SOURCE | BA_PCM_MODE_SINK,
} stream_t;

typedef enum {
	PROFILE_A2DP,
	PROFILE_HFP,
	PROFILE_HSP,
} profile_t;

struct bluealsa_namehint_device {
	unsigned int id;
	char path[128];
	char hex_addr[18];
	char *alias;
	char service[32];
	int ref;
	struct bluealsa_namehint_device *next;
};

struct bluealsa_namehint_hint {
	unsigned int id;
	struct bluealsa_namehint_device *device;
	unsigned int transport;
	profile_t profile;
	stream_t stream;
	char codec[16];
	int ref;
	struct bluealsa_namehint_hint *next;
};

struct bluealsa_namehint_pcm {
	char path[128];
	struct bluealsa_namehint_device *device;
	struct bluealsa_namehint_hint *hint;
	struct bluealsa_namehint_pcm *next;
	stream_t stream;
};

struct bluealsa_namehint {
	struct bluealsa_namehint_hint *hints;
	struct bluealsa_namehint_pcm *pcms;
	struct bluealsa_namehint_device *devices;
	size_t next_id;
};

static const struct {
	profile_t profile;
	const char *name;
	const char *type;
} profiles[] = {
	{ PROFILE_A2DP, "A2DP", "a2dp" },
	{ PROFILE_HFP,  "HFP",  "sco" },
	{ PROFILE_HSP,  "HSP",  "sco" }
};

static struct {
	bool has_ctl_bat_arg;
	bool has_ctl_btt_arg;
	bool has_ctl_dyn_arg;
	bool has_ctl_ext_arg;
} bluealsa_config = { 0 };

static struct bluealsa_namehint_device *bluealsa_namehint_device_get(struct bluealsa_namehint *hint, const char *path, struct bluealsa_client *client, const char *service) {
	struct bluealsa_namehint_device *node = hint->devices, *prev = NULL;
	while (node != NULL) {
		if (strcmp(path, node->path) == 0)
			break;
		prev = node;
		node = node->next;
	}
	if (node != NULL)
		return node;

	node = calloc(sizeof(struct bluealsa_namehint_device), 1);
	if (node == NULL)
		return NULL;

	struct bluealsa_client_device device = {
		.path = path,
	};
	if (bluealsa_client_get_device(client, &device) < 0)
		goto fail;

	strncpy(node->path, path, sizeof(node->path));
	node->path[sizeof(node->path) - 1] = '\0';
	strncpy(node->hex_addr, device.hex_addr, sizeof(node->hex_addr));
	node->hex_addr[sizeof(node->hex_addr) - 1] = '\0';
	strncpy(node->service, service, sizeof(node->service));
	node->service[sizeof(node->service) - 1] = '\0';
	node->alias = strdup(device.alias);
	node->id = hint->next_id;

	if (prev == NULL)
		hint->devices = node;
	else
		prev->next = node;

	return node;

fail:
	if (node != NULL) {
		free(node->alias);
		free(node);
	}
	return NULL;
}

static void bluealsa_namehint_device_remove(struct bluealsa_namehint *hint, struct bluealsa_namehint_device *device) {
	struct bluealsa_namehint_device *node = hint->devices, *prev = NULL;
	while (node != NULL) {
		if (device == node) {
			if (prev == NULL)
				hint->devices = node->next;
			else
				prev->next = node->next;
			free(node->alias);
			free(node);
			return;
		}
		prev = node;
		node = node->next;
	}
}

static void bluealsa_namehint_device_ref(struct bluealsa_namehint *hint, struct bluealsa_namehint_device *device) {
	(void) hint;
	device->ref++;
}

static void bluealsa_namehint_device_unref(struct bluealsa_namehint *hint, struct bluealsa_namehint_device *device) {
	device->ref--;
	if (device->ref <= 0) {
		bluealsa_namehint_device_remove(hint, device);
	}
}

static struct bluealsa_namehint_hint *bluealsa_namehint_hint_get(struct bluealsa_namehint *hint, struct bluealsa_namehint_device* device, const struct ba_pcm *pcm) {
	struct bluealsa_namehint_hint *node = hint->hints, *prev = NULL;
	while (node != NULL) {
		if (device == node->device && pcm->transport == node->transport)
			break;
		prev = node;
		node = node->next;
	}
	if (node == NULL) {
		node = calloc(sizeof(struct bluealsa_namehint_hint), 1);
		if (node == NULL)
			return NULL;

		node->id = hint->next_id++;
		node->device = device;
		bluealsa_namehint_device_ref(hint, device);
		node->transport = pcm->transport;
		if (pcm->transport & BA_PCM_TRANSPORT_MASK_HFP)
			node->profile = PROFILE_HFP;
		else if (pcm->transport & BA_PCM_TRANSPORT_MASK_HSP)
			node->profile = PROFILE_HSP;
		else
			node->profile = PROFILE_A2DP;
		node->stream |= pcm->mode & BA_PCM_MODE_SOURCE ? STREAM_CAPTURE : STREAM_PLAYBACK;
		strcpy(node->codec, pcm->codec.name);
		if (prev == NULL)
			hint->hints = node;
		else
			prev->next = node;
	}
	else {
		node->stream |= (pcm->mode & BA_PCM_MODE_SOURCE ? STREAM_CAPTURE : STREAM_PLAYBACK);
		strcpy(node->codec, pcm->codec.name);
	}

	return node;
}

static void bluealsa_namehint_hint_remove(struct bluealsa_namehint *hint, struct bluealsa_namehint_hint *h) {
	struct bluealsa_namehint_hint *node = hint->hints, *prev = NULL;
	while (node != NULL) {
		if (h == node) {
			if (prev == NULL)
				hint->hints = node->next;
			else
				prev->next = node->next;
			bluealsa_namehint_device_unref(hint, node->device);
			free(node);
			return;
		}
		prev = node;
		node = node->next;
	}
}

static void bluealsa_namehint_hint_ref(struct bluealsa_namehint *hint, struct bluealsa_namehint_hint *h) {
	(void) hint;
	h->ref++;
}

static bool bluealsa_namehint_hint_unref(struct bluealsa_namehint *hint, struct bluealsa_namehint_hint *h) {
	h->ref--;
	if (h->ref <= 0) {
		bluealsa_namehint_hint_remove(hint, h);
		return true;
	}
	return false;
}

static void suppress_alsa_errors(const char *, int, const char *, int, const char *, ...) {
}

static bool bluealsa_has_ctl_arg(snd_config_t *root, snd_config_t *ctl, const char *arg_equals_value) {
	int result;
	snd_config_t *temp = NULL;

	result = snd_config_expand(ctl, root, arg_equals_value, NULL, &temp);
	if (temp != NULL)
		snd_config_delete(temp);

	return result >= 0;
}

/**
 * Allocate and initialize a new bluealsa_namehint structure.
 */
int bluealsa_namehint_init(struct bluealsa_namehint **hint) {
	*hint = calloc(sizeof(struct bluealsa_namehint), 1);
	if (*hint == NULL)
		return -1;

	snd_config_t *ctl_node = NULL;
	if (snd_config_search(snd_config, "ctl.bluealsa", &ctl_node) < 0)
		return 0;

	snd_lib_error_set_handler(suppress_alsa_errors);

	bluealsa_config.has_ctl_bat_arg = bluealsa_has_ctl_arg(snd_config, ctl_node, "BAT=no");
	bluealsa_config.has_ctl_btt_arg = bluealsa_has_ctl_arg(snd_config, ctl_node, "BTT=no");
	bluealsa_config.has_ctl_dyn_arg = bluealsa_has_ctl_arg(snd_config, ctl_node, "DYN=no");
	bluealsa_config.has_ctl_ext_arg = bluealsa_has_ctl_arg(snd_config, ctl_node, "EXT=no");

	snd_lib_error_set_handler(NULL);

	return 0;
}

/**
 * Add a new pcm to a namehint container, if it does not already exist in the
 * container (a duplex pcm device is reported as 2 distinct pcms bt bluealsa)
 * @param hint the namehint container.
 * @param pcm properties of added pcm.
 * @param client interface to bluealsa
 * @param service name of bluealsa service hosting the pcm
 * @return true if new namehint entry created, false otherwise.
 */
bool bluealsa_namehint_pcm_add(struct bluealsa_namehint *hint, const struct ba_pcm *pcm, struct bluealsa_client *client, const char *service) {
	struct bluealsa_namehint_pcm *node = hint->pcms, *prev = NULL;
	while (node != NULL) {
		if (strcmp(pcm->pcm_path, node->path) == 0)
			break;
		prev = node;
		node = node->next;
	}
	if (node != NULL)
		return false;

	node = calloc(sizeof(struct bluealsa_namehint_pcm), 1);
	if (node == NULL)
		return false;

	strcpy(node->path, pcm->pcm_path);

	node->device = bluealsa_namehint_device_get(hint, pcm->device_path, client, service);
	if (node->device == NULL)
		goto fail;
	bluealsa_namehint_device_ref(hint, node->device);

	node->hint = bluealsa_namehint_hint_get(hint, node->device, pcm);
	if (node->hint == NULL)
		goto fail;
	bluealsa_namehint_hint_ref(hint, node->hint);

	node->stream = pcm->mode & BA_PCM_MODE_SOURCE ? STREAM_CAPTURE : STREAM_PLAYBACK;

	if (prev == NULL)
		hint->pcms = node;
	else
		prev->next = node;

	return true;

fail:
	free(node);
	return false;
}

/**
 * Remove a pcm from a namehint container.
 * @param hint the namehint container.
 * @param path d-bus bluealsa path of pcm.
 * @return true if namehint entry removed, false otherwise.
 */
bool bluealsa_namehint_pcm_remove(struct bluealsa_namehint *hint, const char *path) {
	struct bluealsa_namehint_pcm *node = hint->pcms, *prev = NULL;
	while (node != NULL) {
		if (strcmp(path, node->path) == 0) {
			bool hint_removed;
			if (prev != NULL)
				prev->next = node->next;
			else
				hint->pcms = node->next;
			hint_removed = bluealsa_namehint_hint_unref(hint, node->hint);
			bluealsa_namehint_device_unref(hint, node->device);
			free(node);
			return hint_removed;
		}
		prev = node;
		node = node->next;
	}
	return false;
}

/**
 * Remove all hints relating to a specific service from a namehint container.
 * @param hint the namehint container.
 * @param service d-bus name of bluealsa service.
 * @return true if namehint entry removed, false otherwise.
 */
bool bluealsa_namehint_service_remove(struct bluealsa_namehint *hint, const char *service) {
	struct bluealsa_namehint_pcm *pcm = hint->pcms, *pcm_del = NULL, *prev = NULL;
	bool hint_removed = false;
	while (pcm != NULL) {
		if (strcmp(pcm->device->service, service) == 0) {
			if (prev != NULL)
				prev->next = pcm->next;
			else
				hint->pcms = pcm->next;
			bluealsa_namehint_hint_unref(hint, pcm->hint);
			bluealsa_namehint_device_unref(hint, pcm->device);
			pcm_del = pcm;
			pcm = pcm->next;
			free(pcm_del);
			hint_removed = true;
		}
		else {
			prev = pcm;
			pcm = pcm->next;
		}
	}
	return hint_removed;
}

/**
 * Remove all hints from a namehint container.
 * @param hint the namehint container.
 */
void bluealsa_namehint_remove_all(struct bluealsa_namehint *hint) {
	struct bluealsa_namehint_pcm *pcm = hint->pcms, *pcm_del = NULL;
	struct bluealsa_namehint_hint *h = hint->hints, *hint_del = NULL;
	struct bluealsa_namehint_device *device = hint->devices, *device_del = NULL;
	while (pcm != NULL) {
		pcm_del = pcm;
		pcm = pcm->next;
		free(pcm_del);
	}
	while (h != NULL) {
		hint_del = h;
		h = h->next;
		free(hint_del);
	}
	while (device != NULL) {
		device_del = device;
		device = device->next;
		free(device_del->alias);
		free(device_del);
	}
	hint->hints = NULL;
	hint->devices = NULL;
	hint->pcms = NULL;
}

bool bluealsa_namehint_pcm_update(struct bluealsa_namehint *hint, const char *path, const char *codec) {
	struct bluealsa_namehint_pcm *pcm_node = hint->pcms;

	while (pcm_node != NULL) {
		if (strcmp(path, pcm_node->path) == 0)
			break;
		pcm_node = pcm_node->next;
	}
	if (pcm_node == NULL)
		return false;

	strncpy(pcm_node->hint->codec, codec, sizeof(pcm_node->hint->codec) - 1);
	return true;
}

void bluealsa_namehint_reset(struct bluealsa_namehint *hint) {
	if (hint->pcms == NULL)
		hint->next_id = 0;
}

static int bluealsa_namehint_hint_expand_description(const struct bluealsa_namehint_hint *h, const char *pattern, char *buffer, size_t len) {
	const char *end = buffer + len;
	char *pos = buffer;
	const char *p;
	for (p = pattern; *p != 0 && pos < end; p++) {
		if (*p == '%') {
			switch (*(++p)) {
			case 'n': /* name (alias) */ {
				len = strlen(h->device->alias);
				if ((pos + len) >= end)
					return -ENOMEM;
				strcpy(pos, h->device->alias);
				pos += len;
				break;
			}
			case 'a': /* address */ {
				len = strlen(h->device->hex_addr);
				if ((pos + len) >= end)
					return -ENOMEM;
				strcpy(pos, h->device->hex_addr);
				pos += len;
				break;
			}
			case 'c': /* codec */ {
				len = strlen(h->codec);
				if ((pos + len) >= end)
					return -ENOMEM;
				strcpy(pos, h->codec);
				pos += len;
				break;
			}
			case 'l': /* line break */ {
				if ((pos + 2) >= end)
					return -ENOMEM;
				*pos++ = '\012';
				break;
			}
			case 'p': /* profile */ {
				const char *profile = profiles[h->profile].name;
				len = strlen(profile);
				if ((pos + len) >= end)
					return -ENOMEM;
				strcpy(pos, profile);
				pos += len;
				break;
			}
			case 's': /* stream direction */ {
				const char *stream =
						h->stream == STREAM_PLAYBACK ? "Output" :
							h->stream == STREAM_CAPTURE ?  "Input" : "Input/Output";
				len = strlen(stream);
				if ((pos + len) >=end)
					return -ENOMEM;
				strcpy(pos, stream);
				pos += len;
				break;
			}
			case '%': /* literal percent */
				*pos++ = '%';
				break;
			default:
				*pos++ = *p;
			}
		}
		else
			*pos++ = *p;
	}

	*pos = '\0';

	return pos < end ? 0 : -ENOSPC;
}

/**
 * Write out a namehint container to a file.
 * @param hint the namehint container.
 * @param file the file.
 * @param pattern template to be used for hint descriptions.
 */
int bluealsa_namehint_print(const struct bluealsa_namehint *hint, FILE *file, const char *pattern, bool with_service) {
	struct bluealsa_namehint_hint *h = hint->hints;
	struct bluealsa_namehint_device *d = hint->devices;
	unsigned int alsa_version = alsa_version_id();
	const char *desc_separator = alsa_version >= 0x010203 ? "|" : "|DESC";

	while (h != NULL) {
		char description[256];
		int ret = bluealsa_namehint_hint_expand_description(h, pattern, description, 256);
		if (ret < 0)
			return ret;

		fprintf(file, "namehint.pcm._bluealsa%u \"bluealsa:DEV=%s,PROFILE=%s%s%s%s%s%s\"\n",
			h->id,
			h->device->hex_addr,
			profiles[h->profile].type,
			with_service ? ",SRV=" : "",
			with_service ? h->device->service : "",
			desc_separator,
			description,
			h->stream == BA_PCM_MODE_SOURCE ? "|IOIDInput" :
				h->stream == BA_PCM_MODE_SINK ? "|IOIDOutput" : "");
		h = h->next;
	}
	while (d != NULL) {
		fprintf(file, "namehint.ctl._bluealsa%u \"bluealsa:DEV=%s%s%s%s%s\n"
				"Bluetooth Audio Control Device\"\n",
			d->id,
			d->hex_addr,
			with_service ? ",SRV=" : "",
			with_service ? d->service : "",
			desc_separator,
			d->alias);
		d = d->next;
	}

	const char *show = hint->pcms == NULL ? "off" : "on";
	fprintf(file, "bluealsa.pcm.hint.show %1$s\nbluealsa.ctl.hint.show %1$s\n", show);

	return 0;
}

/**
 * Write out most recently connected pcms for each profile and stream direction.
 * @param hint the namehint container.
 * @param file the file.
 */
void bluealsa_namehint_print_default(struct bluealsa_namehint *hint, FILE *file) {
	enum {
		CAPTURE_A2DP,
		PLAYBACK_A2DP,
		CAPTURE_SCO,
		PLAYBACK_SCO,
	};

	struct bluealsa_namehint_pcm *pcm = hint->pcms;
	struct bluealsa_namehint_pcm *defaults[4] = { 0 };

	while (pcm != NULL) {
		switch (pcm->stream) {
		case STREAM_CAPTURE:
			if (pcm->hint->profile == PROFILE_A2DP)
				defaults[CAPTURE_A2DP] = pcm;
			else
				defaults[CAPTURE_SCO] = pcm;
			break;
		case STREAM_PLAYBACK:
			if (pcm->hint->profile == PROFILE_A2DP)
				defaults[PLAYBACK_A2DP] = pcm;
			else
				defaults[PLAYBACK_SCO] = pcm;
			break;
		default:
			if (pcm->hint->profile == PROFILE_A2DP) {
				defaults[CAPTURE_A2DP] = pcm;
				defaults[PLAYBACK_A2DP] = pcm;
			}
			else {
				defaults[CAPTURE_SCO] = pcm;
				defaults[PLAYBACK_SCO] = pcm;
			}
			break;
		}

		pcm = pcm->next;
	}

	if (defaults[CAPTURE_A2DP] != NULL)
		fprintf(file, "capture.a2dp \"pcm.bluealsa:DEV=%s,PROFILE=a2dp,SRV=%s\"\n",
					defaults[CAPTURE_A2DP]->device->hex_addr,
					defaults[CAPTURE_A2DP]->device->service);
	if (defaults[PLAYBACK_A2DP] != NULL)
		fprintf(file, "playback.a2dp \"pcm.bluealsa:DEV=%s,PROFILE=a2dp,SRV=%s\"\n",
					defaults[PLAYBACK_A2DP]->device->hex_addr,
					defaults[PLAYBACK_A2DP]->device->service);
	if (defaults[CAPTURE_SCO] != NULL)
		fprintf(file, "capture.sco \"pcm.bluealsa:DEV=%s,PROFILE=sco,SRV=%s\"\n",
					defaults[CAPTURE_SCO]->device->hex_addr,
					defaults[CAPTURE_SCO]->device->service);
	if (defaults[PLAYBACK_SCO] != NULL)
		fprintf(file, "playback.sco \"pcm.bluealsa:DEV=%s,PROFILE=sco,SRV=%s\"\n",
					defaults[PLAYBACK_SCO]->device->hex_addr,
					defaults[PLAYBACK_SCO]->device->service);

	if (alsa_version_id() >= 0x010205) {
		/* ctl default requires features introduced in alsa-lib release 1.2.5 */

		if (defaults[PLAYBACK_A2DP] == NULL && defaults[PLAYBACK_SCO] == NULL)
			return;

		const char *addr = NULL;
		const char *service;

		if (defaults[PLAYBACK_A2DP] != NULL) {
			addr = defaults[PLAYBACK_A2DP]->device->hex_addr;
			service = defaults[PLAYBACK_A2DP]->device->service;
		}

		if (defaults[PLAYBACK_SCO] != NULL) {
			if (defaults[PLAYBACK_A2DP] == NULL) {
				addr = defaults[PLAYBACK_SCO]->device->hex_addr;
				service = defaults[PLAYBACK_SCO]->device->service;
			}
			else if (strcmp(defaults[PLAYBACK_SCO]->device->service, defaults[PLAYBACK_A2DP]->device->service) == 0 && strcmp(defaults[PLAYBACK_SCO]->device->hex_addr, defaults[PLAYBACK_A2DP]->device->hex_addr) != 0) {
				service = defaults[PLAYBACK_A2DP]->device->service;
				addr = "FF:FF:FF:FF:FF:FF";
			}
		}

		if (addr == NULL)
			return;

		/* alsa-lib "empty" ctl plugin fails if we create a reference here,
		 * so we resort to an explicit config definition. */
		fprintf(file, "ctl { type bluealsa device \"%s\" service \"%s\"", addr, service);

		if (bluealsa_config.has_ctl_bat_arg)
			fprintf(file, " battery { @func refer name defaults.bluealsa.ctl.battery }");

		if (bluealsa_config.has_ctl_btt_arg)
			fprintf(file, " bttransport { @func refer name defaults.bluealsa.ctl.bttransport }");

		if (bluealsa_config.has_ctl_dyn_arg)
			fprintf(file, " dynamic { @func refer name defaults.bluealsa.ctl.dynamic }");

		if (bluealsa_config.has_ctl_ext_arg)
			fprintf(file, " extended { @func refer name defaults.bluealsa.ctl.extended }");

		fprintf(file, "}\n");
	}

}

/**
 * Free a bluealsa_namehint structure.
 */
void bluealsa_namehint_free(struct bluealsa_namehint *hint) {
	bluealsa_namehint_remove_all(hint);
	free(hint);
}


