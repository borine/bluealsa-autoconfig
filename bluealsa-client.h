/*
 * bluealsa-autoconfig - bluealsa-client.h
 * Copyright (c) 2023-2024 @borine (https://github.com/borine/)
 *
 * This project is licensed under the terms of the MIT license.
 *
 */

#ifndef BLUEALSA_CLIENT_H
#define BLUEALSA_CLIENT_H

#include <poll.h>
#include <stdbool.h>
#include "bluez-alsa/dbus.h"
#include "bluez-alsa/shared/dbus-client-pcm.h"

typedef struct bluealsa_client *bluealsa_client_t;

struct bluealsa_client_device {
	const char *path;
	char hex_addr[18];
	char alias[64];
};

struct bluealsa_pcm_properties {
	uint16_t mask;
	uint16_t format;
	uint8_t channels;
	uint32_t sampling;
	struct ba_pcm_codec codec;
	bool running;
	uint16_t delay;
	bool softvolume;
	uint16_t volume;
};

#define BLUEALSA_PCM_PROPERTY_CHANGED_FORMAT       (1 << 0)
#define BLUEALSA_PCM_PROPERTY_CHANGED_CHANNELS     (1 << 1)
#define BLUEALSA_PCM_PROPERTY_CHANGED_SAMPLING     (1 << 2)
#define BLUEALSA_PCM_PROPERTY_CHANGED_CODEC        (1 << 3)
#define BLUEALSA_PCM_PROPERTY_CHANGED_CODEC_CONFIG (1 << 4)
#define BLUEALSA_PCM_PROPERTY_CHANGED_RUNNING      (1 << 5)
#define BLUEALSA_PCM_PROPERTY_CHANGED_DELAY        (1 << 6)
#define BLUEALSA_PCM_PROPERTY_CHANGED_SOFTVOL      (1 << 7)
#define BLUEALSA_PCM_PROPERTY_CHANGED_VOLUME       (1 << 8)


typedef void (*pcm_added_t)(const struct ba_pcm *pcm, const char *service, void *data);
typedef void (*pcm_removed_t)(const char *path, void *data);
typedef void (*pcm_updated_t)(const char *path, const char *service, struct bluealsa_pcm_properties *props, void *data);
typedef void (*service_stopped_t)(const char *service, void *data);

struct bluealsa_client_callbacks {
	pcm_added_t add_func;
	pcm_removed_t remove_func;
	pcm_updated_t update_func;
	service_stopped_t stopped_func;
	void *data;
};

int bluealsa_client_open(bluealsa_client_t *client, struct bluealsa_client_callbacks *callbacks);
int bluealsa_client_close(bluealsa_client_t client);
int bluealsa_client_get_pcms(bluealsa_client_t client, const char *service);
int bluealsa_client_num_services(const bluealsa_client_t client);
int bluealsa_client_get_device(bluealsa_client_t client, struct bluealsa_client_device *device);
int bluealsa_client_watch_service(bluealsa_client_t client, const char *service);
int bluealsa_client_poll_fds(bluealsa_client_t client, struct pollfd *fds, nfds_t *nfds);
int bluealsa_client_poll_dispatch(bluealsa_client_t client, struct pollfd *fds, nfds_t nfds);

const char *bluealsa_client_transport_to_role(int transport_code);
const char *bluealsa_client_transport_to_type(int transport_code);
const char *bluealsa_client_transport_to_profile(int transport_code);
const char *bluealsa_client_mode_to_string(int pcm_mode);
const char *bluealsa_client_format_to_string(int pcm_format);

const char *bluealsa_client_codec_blob_to_string(const struct ba_pcm_codec *codec, char buffer[], size_t buflen);

#endif
