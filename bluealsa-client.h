/*
 * BlueALSA - bluealsa-client.h
 * Copyright (c) 2023 @borine (https://github.com/borine/)
 *
 * This project is licensed under the terms of the MIT license.
 *
 */

#ifndef BLUEALSA_CLIENT_H
#define BLUEALSA_CLIENT_H

#include <poll.h>
#include <stdbool.h>
#include "bluez-alsa/dbus.h"
#include "bluez-alsa/shared/dbus-client.h"

typedef struct bluealsa_client bluealsa_client_t;

struct bluealsa_client_device {
	const char *path;
	char hex_addr[18];
	char alias[64];
};

struct bluealsa_pcm_properties {
	uint8_t mask;
	uint16_t format;
	uint8_t channels;
	uint32_t sampling;
	char codec[16];
	uint16_t delay;
	bool softvolume;
	uint16_t volume;
};

#define BLUEALSA_PCM_PROPERTY_CHANGED_FORMAT   (1 << 0)
#define BLUEALSA_PCM_PROPERTY_CHANGED_CHANNELS (1 << 1)
#define BLUEALSA_PCM_PROPERTY_CHANGED_SAMPLING (1 << 2)
#define BLUEALSA_PCM_PROPERTY_CHANGED_CODEC    (1 << 3)
#define BLUEALSA_PCM_PROPERTY_CHANGED_DELAY    (1 << 4)
#define BLUEALSA_PCM_PROPERTY_CHANGED_SOFTVOL  (1 << 5)
#define BLUEALSA_PCM_PROPERTY_CHANGED_VOLUME   (1 << 6)


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

int bluealsa_client_open(bluealsa_client_t **client, struct bluealsa_client_callbacks *callbacks);
int bluealsa_client_close(bluealsa_client_t *client);
int bluealsa_client_get_services(bluealsa_client_t *client, const char **list, size_t *len);
int bluealsa_client_get_pcms(bluealsa_client_t *client, const char *service);
int bluealsa_client_get_device(bluealsa_client_t *client, struct bluealsa_client_device *device);
int bluealsa_client_watch_service(bluealsa_client_t *client, const char *service);
int bluealsa_client_poll_fds(bluealsa_client_t *client, struct pollfd *fds, nfds_t *nfds);
int bluealsa_client_poll_dispatch(bluealsa_client_t *client, struct pollfd *fds, nfds_t nfds);


#endif
