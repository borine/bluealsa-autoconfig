/*
 * bluealsa-autoconfig - bluealsa-client.c
 * Copyright (c) 2023 @borine (https://github.com/borine/)
 *
 * This project is licensed under the terms of the MIT license.
 *
 */

#include "bluealsa-client.h"
#include "bluez-alsa/dbus.h"
#include "bluez-alsa/shared/dbus-client-pcm.h"
#include "bluez-alsa/shared/log.h"

#include <bluetooth/bluetooth.h>
#include <dbus/dbus.h>
#include <errno.h>
#include <stdlib.h>

struct bluealsa_client_service {
	char well_known_name[32];
	char unique_name[16];
};

struct bluealsa_client {
	struct ba_dbus_ctx dbus_ctx;
	pcm_added_t add_func;
	pcm_removed_t remove_func;
	pcm_updated_t update_func;
	service_stopped_t stopped_func;
	void *user_data;
	struct bluealsa_client_service *services;
	size_t services_count;
};

static const char *bluealsa_client_get_unique_name(DBusConnection *conn, const char *well_known_name) {

	DBusError error = DBUS_ERROR_INIT;
	DBusMessage *msg;
	DBusMessage *rep = NULL;

	if ((msg = dbus_message_new_method_call("org.freedesktop.DBus", "/org/freedesktop/DBus",
					"org.freedesktop.DBus", "GetNameOwner")) == NULL) {
		dbus_set_error(&error, DBUS_ERROR_NO_MEMORY, NULL);
		return NULL;
	}

	DBusMessageIter iter;
	dbus_message_iter_init_append(msg, &iter);
	if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &well_known_name)) {
		dbus_set_error(&error, DBUS_ERROR_NO_MEMORY, NULL);
		goto fail;
	}

	if ((rep = dbus_connection_send_with_reply_and_block(conn,
					msg, DBUS_TIMEOUT_USE_DEFAULT, &error)) == NULL)
		goto fail;

	if (!dbus_message_iter_init(rep, &iter)) {
		dbus_set_error(&error, DBUS_ERROR_INVALID_SIGNATURE, "Empty response message");
		goto fail;
	}

	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING) {
		char *signature = dbus_message_iter_get_signature(&iter);
		dbus_set_error(&error, DBUS_ERROR_INVALID_SIGNATURE,
				"Incorrect signature: %s != s", signature);
		dbus_free(signature);
		goto fail;
	}

	const char *unique_name;
	dbus_message_iter_get_basic(&iter, &unique_name);

	return unique_name;
fail:
	if (rep != NULL)
		dbus_message_unref(rep);
	if (msg != NULL)
		dbus_message_unref(msg);

	return NULL;
}

static void	bluealsa_client_service_started(bluealsa_client_t client, const char *well_known_name, const char *unique_name) {
	for (unsigned int index = 0; index < client->services_count; index++) {
		struct bluealsa_client_service *service = &client->services[index];
		if (strcmp(service->well_known_name, well_known_name) == 0) {
			strncpy(service->unique_name, unique_name, sizeof(service->unique_name) - 1);
			return;
		}
	}
}

static void	bluealsa_client_service_stopped(bluealsa_client_t client, const char *well_known_name) {
	if (client->stopped_func == NULL)
		return;
	for (unsigned int index = 0; index < client->services_count; index++) {
		struct bluealsa_client_service *service = &client->services[index];
		if (strcmp(service->well_known_name, well_known_name) == 0) {
			service->unique_name[0] = '\0';
			client->stopped_func(service->well_known_name, client->user_data);
			return;
		}
	}
}

static void	bluealsa_client_pcm_added(bluealsa_client_t client, struct ba_pcm *pcm, const char *unique_name) {
	if (client->add_func == NULL)
		return;
	for (unsigned int index = 0; index < client->services_count; index++) {
		struct bluealsa_client_service *service = &client->services[index];
		if (strcmp(service->unique_name, unique_name) == 0) {
			client->add_func(pcm, service->well_known_name, client->user_data);
			return;
		}
	}
}

static void	bluealsa_client_pcm_removed(bluealsa_client_t client, const char *path, const char *unique_name) {
	if (client->remove_func == NULL)
		return;
	for (unsigned int index = 0; index < client->services_count; index++) {
		struct bluealsa_client_service *service = &client->services[index];
		if (strcmp(service->unique_name, unique_name) == 0) {
			client->remove_func(path, client->user_data);
			return;
		}
	}
}

static void	bluealsa_client_pcm_updated(bluealsa_client_t client, const char *path, const char *unique_name, struct bluealsa_pcm_properties *props) {
	if (client->update_func == NULL)
		return;
	for (unsigned int index = 0; index < client->services_count; index++) {
		struct bluealsa_client_service *service = &client->services[index];
		if (strcmp(service->unique_name, unique_name) == 0) {
			client->update_func(path, service->well_known_name, props, client->user_data);
			return;
		}
	}
}

static DBusHandlerResult bluealsa_client_objmgr_signal_handler(bluealsa_client_t client, const char *signal, const char *service, DBusMessageIter *iter) {

	if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_OBJECT_PATH)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	const char *path;
	dbus_message_iter_get_basic(iter, &path);

	if (strcmp(signal, "InterfacesAdded") == 0) {
		DBusMessageIter iter_copy = *iter;
		if (!dbus_message_iter_next(&iter_copy))
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
		DBusMessageIter iter_ifaces;
		for (dbus_message_iter_recurse(&iter_copy, &iter_ifaces);
				dbus_message_iter_get_arg_type(&iter_ifaces) != DBUS_TYPE_INVALID;
				dbus_message_iter_next(&iter_ifaces)) {

			DBusMessageIter iter_iface_entry;
			if (dbus_message_iter_get_arg_type(&iter_ifaces) != DBUS_TYPE_DICT_ENTRY)
				return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
			dbus_message_iter_recurse(&iter_ifaces, &iter_iface_entry);

			const char *iface;
			if (dbus_message_iter_get_arg_type(&iter_iface_entry) != DBUS_TYPE_STRING)
				return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
			dbus_message_iter_get_basic(&iter_iface_entry, &iface);

			if (strcmp(iface, BLUEALSA_INTERFACE_PCM) == 0) {
				struct ba_pcm pcm;
				DBusError err = DBUS_ERROR_INIT;
				if (!dbus_message_iter_get_ba_pcm(iter, &err, &pcm)) {
					error("Couldn't read PCM properties: %s", err.message);
					dbus_error_free(&err);
					return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
				}
				bluealsa_client_pcm_added(client, &pcm, service);
			}
		}
	}
	else if (strcmp(signal, "InterfacesRemoved") == 0) {
		if (!dbus_message_iter_next(iter))
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
		DBusMessageIter iter_ifaces;
		for (dbus_message_iter_recurse(iter, &iter_ifaces);
				dbus_message_iter_get_arg_type(&iter_ifaces) != DBUS_TYPE_INVALID;
				dbus_message_iter_next(&iter_ifaces)) {

			const char *iface;
			if (dbus_message_iter_get_arg_type(&iter_ifaces) != DBUS_TYPE_STRING)
				return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
			dbus_message_iter_get_basic(&iter_ifaces, &iface);

			if (strcmp(iface, BLUEALSA_INTERFACE_PCM) != 0)
				return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

			bluealsa_client_pcm_removed(client, path, service);
		}
	}

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult bluealsa_client_name_signal_handler(bluealsa_client_t client, const char *signal, DBusMessageIter *iter) {
	if (strcmp(signal, "NameOwnerChanged") != 0)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	const char *arg0 = NULL, *arg1 = NULL, *arg2 = NULL;

	if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_STRING)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	dbus_message_iter_get_basic(iter, &arg0);

	if (dbus_message_iter_next(iter) &&
			dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_STRING)
		dbus_message_iter_get_basic(iter, &arg1);
	else
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	if (dbus_message_iter_next(iter) &&
			dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_STRING)
		dbus_message_iter_get_basic(iter, &arg2);
	else
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	if (strncmp(arg0, "org.bluealsa", strlen("org.bluealsa")) != 0)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	if (strlen(arg1) == 0)
		bluealsa_client_service_started(client, arg0, arg2);
	if (strlen(arg2) == 0)
		bluealsa_client_service_stopped(client, arg0);
	else
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult bluealsa_client_parse_pcm_property(const char *name, DBusMessageIter *iter, void *data) {
	struct bluealsa_pcm_properties *props = data;
	if (strcmp(name, "Format") == 0) {
		if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_UINT16)
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
		dbus_message_iter_get_basic(iter, &props->format);
		props->mask |= BLUEALSA_PCM_PROPERTY_CHANGED_FORMAT;
	}
	else if (strcmp(name, "Channels") == 0) {
		if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_BYTE)
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
		dbus_message_iter_get_basic(iter, &props->channels);
		props->mask |= BLUEALSA_PCM_PROPERTY_CHANGED_CHANNELS;
	}
	else if (strcmp(name, "Sampling") == 0) {
		if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_UINT32)
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
		dbus_message_iter_get_basic(iter, &props->sampling);
		props->mask |= BLUEALSA_PCM_PROPERTY_CHANGED_SAMPLING;
	}
	else if (strcmp(name, "Codec") == 0) {
		if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_STRING)
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
		const char *codec;
		dbus_message_iter_get_basic(iter, &codec);
		strncpy(props->codec, codec, sizeof(props->codec) - 1);
		props->mask |= BLUEALSA_PCM_PROPERTY_CHANGED_CODEC;
	}
	else if (strcmp(name, "Delay") == 0) {
		if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_UINT16)
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
		dbus_message_iter_get_basic(iter, &props->delay);
		props->mask |= BLUEALSA_PCM_PROPERTY_CHANGED_DELAY;
	}
	else if (strcmp(name, "SoftVolume") == 0) {
		if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_BOOLEAN)
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
		dbus_message_iter_get_basic(iter, &props->softvolume);
		props->mask |= BLUEALSA_PCM_PROPERTY_CHANGED_SOFTVOL;
	}
	else if (strcmp(name, "Volume") == 0) {
		if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_UINT16)
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
		dbus_message_iter_get_basic(iter, &props->volume);
		props->mask |= BLUEALSA_PCM_PROPERTY_CHANGED_VOLUME;
	}
	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult bluealsa_client_parse_properties(DBusMessageIter *iter, DBusHandlerResult (parse_property)(const char *, DBusMessageIter *, void *), void *data) {
	DBusMessageIter iter_properties;
	for (dbus_message_iter_recurse(iter, &iter_properties);
			dbus_message_iter_get_arg_type(&iter_properties) != DBUS_TYPE_INVALID;
			dbus_message_iter_next(&iter_properties)) {

		if (dbus_message_iter_get_arg_type(&iter_properties) != DBUS_TYPE_DICT_ENTRY)
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

		DBusMessageIter iter_property;
		dbus_message_iter_recurse(&iter_properties, &iter_property);
		if (dbus_message_iter_get_arg_type(&iter_property) != DBUS_TYPE_STRING)
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
		const char *name = NULL;
		dbus_message_iter_get_basic(&iter_property, &name);
		if (!dbus_message_iter_next(&iter_property) ||
				dbus_message_iter_get_arg_type(&iter_property) != DBUS_TYPE_VARIANT)
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
		DBusMessageIter iter_val;
		dbus_message_iter_recurse(&iter_property, &iter_val);
		parse_property(name, &iter_val, data);
	}
	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult bluealsa_client_pcm_properties_changed(bluealsa_client_t client, const char *path, const char *service, DBusMessageIter *iter) {
	struct bluealsa_pcm_properties props = { 0 };
	bluealsa_client_parse_properties(iter, bluealsa_client_parse_pcm_property, &props);
	if (props.mask != 0)
		bluealsa_client_pcm_updated(client, path, service, &props);
	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult bluealsa_client_properties_signal_handler(bluealsa_client_t client, const char *path, const char *signal, const char *service, DBusMessageIter *iter) {
	if (strcmp(signal, "PropertiesChanged") != 0)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_STRING)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	const char *interface = NULL;
	dbus_message_iter_get_basic(iter, &interface);

	if (!dbus_message_iter_next(iter))
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	if (strcmp(interface, "org.bluealsa.PCM1") == 0)
		return bluealsa_client_pcm_properties_changed(client, path, service, iter);

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static DBusHandlerResult bluealsa_client_dbus_signal_handler(DBusConnection *conn, DBusMessage *message, void *data) {
	(void)conn;
	bluealsa_client_t client = data;

	if (dbus_message_get_type(message) != DBUS_MESSAGE_TYPE_SIGNAL)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	const char *interface = dbus_message_get_interface(message);
	const char *signal = dbus_message_get_member(message);
	const char *service = dbus_message_get_sender(message);

	DBusMessageIter iter;
	if (!dbus_message_iter_init(message, &iter))
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	if (strcmp(interface, DBUS_INTERFACE_OBJECT_MANAGER) == 0)
		return bluealsa_client_objmgr_signal_handler(client, signal, service, &iter);

	else if (strcmp(interface, DBUS_INTERFACE_DBUS) == 0)
		return bluealsa_client_name_signal_handler(client, signal, &iter);

	else if (strcmp(interface, DBUS_INTERFACE_PROPERTIES) == 0) {
		const char *path = dbus_message_get_path(message);
		return bluealsa_client_properties_signal_handler(client, path, signal, service, &iter);
	}

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

int bluealsa_client_open(bluealsa_client_t *client, struct bluealsa_client_callbacks *callbacks) {
	int ret = 0;
	bluealsa_client_t new_client = calloc(sizeof(struct bluealsa_client), 1);
	if (new_client == NULL)
		return -ENOMEM;

	DBusError err = DBUS_ERROR_INIT;
	dbus_threads_init_default();
	if (!ba_dbus_connection_ctx_init(&new_client->dbus_ctx, "", &err)) {
		free(new_client);
		error("Couldn't initialize D-Bus context: %s", err.message);
		ret = EIO;
		goto fail;
	}

	if (callbacks != NULL) {
		new_client->add_func = callbacks->add_func;
		new_client->remove_func = callbacks->remove_func;
		new_client->update_func = callbacks->update_func;
		new_client->stopped_func = callbacks->stopped_func;
		new_client->user_data = callbacks->data;

		if (!dbus_connection_add_filter(new_client->dbus_ctx.conn, bluealsa_client_dbus_signal_handler, new_client, NULL)) {
			error("Couldn't add D-Bus filter");
			ret = ENOMEM;
			goto fail;
		}
	}

	*client = new_client;
	return 0;

fail:
	ba_dbus_connection_ctx_free(&new_client->dbus_ctx);
	free(new_client);
	return -ret;
}

int bluealsa_client_close(bluealsa_client_t client) {
	ba_dbus_connection_ctx_free(&client->dbus_ctx);
	free(client->services);
	free(client);
	return 0;
}

int bluealsa_client_get_pcms(bluealsa_client_t client, const char *service) {
	if (strlen(service) >= sizeof(client->dbus_ctx.ba_service))
		return -EINVAL;

	DBusError error = DBUS_ERROR_INIT;
	struct ba_pcm *pcms = NULL;
	size_t count = 0;
	strcpy(client->dbus_ctx.ba_service, service);
	if (!ba_dbus_pcm_get_all(&client->dbus_ctx, &pcms, &count, &error))
		return -ENOENT;

	size_t i;
	for (i = 0; i < count; i++)
		client->add_func(&pcms[i], service, client->user_data);

	return 0;
}

int bluealsa_client_num_services(const bluealsa_client_t client) {
	return client->services_count;
}

int bluealsa_client_poll_fds(bluealsa_client_t client, struct pollfd *fds, nfds_t *nfds) {
	if (!ba_dbus_connection_poll_fds(&client->dbus_ctx, fds, nfds))
		return -1;
	return 0;
}

int bluealsa_client_poll_dispatch(bluealsa_client_t client, struct pollfd *pfds, nfds_t nfds) {
	if (ba_dbus_connection_poll_dispatch(&client->dbus_ctx, pfds, nfds))
		while (dbus_connection_dispatch(client->dbus_ctx.conn) == DBUS_DISPATCH_DATA_REMAINS)
			continue;
	return 0;
}

int bluealsa_client_watch_service(bluealsa_client_t client, const char *service) {
	struct bluealsa_client_service *services = realloc(client->services, (client->services_count + 1) * sizeof(struct bluealsa_client_service));
	if (services == NULL)
		return -ENOMEM;

	client->services = services;

	struct bluealsa_client_service *new_service = &client->services[client->services_count++];

	strncpy(new_service->well_known_name, service, sizeof(new_service->well_known_name) - 1);
	const char *unique_name = bluealsa_client_get_unique_name(client->dbus_ctx.conn, service);
	if (unique_name != NULL)
		strncpy(new_service->unique_name, unique_name, sizeof(new_service->unique_name) - 1);

	if (client->add_func)
			ba_dbus_connection_signal_match_add(&client->dbus_ctx,
				service, NULL, DBUS_INTERFACE_OBJECT_MANAGER,
				"InterfacesAdded", "path_namespace='/org/bluealsa'");
	if (client->remove_func)
		ba_dbus_connection_signal_match_add(&client->dbus_ctx,
				service, NULL, DBUS_INTERFACE_OBJECT_MANAGER,
				"InterfacesRemoved", "path_namespace='/org/bluealsa'");

	if (client->update_func)
		ba_dbus_connection_signal_match_add(&client->dbus_ctx,
				service, NULL, DBUS_INTERFACE_PROPERTIES,
				"PropertiesChanged", "path_namespace='/org/bluealsa'");

	char dbus_args[50];
	/* service stopped */
	snprintf(dbus_args, sizeof(dbus_args), "arg0='%s',arg2=''", service);
	ba_dbus_connection_signal_match_add(&client->dbus_ctx,
			DBUS_SERVICE_DBUS, NULL, DBUS_INTERFACE_DBUS,
			"NameOwnerChanged", dbus_args);
	/* service started */
	snprintf(dbus_args, sizeof(dbus_args), "arg0='%s',arg1=''", service);
	ba_dbus_connection_signal_match_add(&client->dbus_ctx,
			DBUS_SERVICE_DBUS, NULL, DBUS_INTERFACE_DBUS,
			"NameOwnerChanged", dbus_args);
	return 0;
}

int bluealsa_client_get_device(bluealsa_client_t client, struct bluealsa_client_device *device) {
	struct bluez_device dev = { 0 };
	if (dbus_bluez_get_device(client->dbus_ctx.conn, device->path, &dev, NULL) < 0)
		return -1;

	strncpy(device->alias, dev.name, sizeof(device->alias));
	device->alias[sizeof(device->alias) - 1] = '\0';
	ba2str(&dev.bt_addr, device->hex_addr);

	return 0;
}

const char *bluealsa_client_transport_to_string(int transport_code) {
	switch (transport_code) {
	case BA_PCM_TRANSPORT_A2DP_SOURCE:
		return "A2DP-source";
	case BA_PCM_TRANSPORT_A2DP_SINK:
		return "A2DP-sink";
	case BA_PCM_TRANSPORT_HFP_AG:
		return "HFP-AG";
	case BA_PCM_TRANSPORT_HFP_HF:
		return "HFP-HF";
	case BA_PCM_TRANSPORT_HSP_AG:
		return "HSP-AG";
	case BA_PCM_TRANSPORT_HSP_HS:
		return "HSP-HS";
	case BA_PCM_TRANSPORT_MASK_A2DP:
		return "A2DP";
	case BA_PCM_TRANSPORT_MASK_HFP:
		return "HFP";
	case BA_PCM_TRANSPORT_MASK_HSP:
		return "HSP";
	case BA_PCM_TRANSPORT_MASK_SCO:
		return "SCO";
	case BA_PCM_TRANSPORT_MASK_AG:
		return "AG";
	case BA_PCM_TRANSPORT_MASK_HF:
		return "HF";
	default:
		return "Invalid";
	}
}

const char *bluealsa_client_mode_to_string(int pcm_mode) {
	switch (pcm_mode) {
	case BA_PCM_MODE_SINK:
		return "sink";
	case BA_PCM_MODE_SOURCE:
		return "source";
	default:
		return "Invalid";
	}
}

const char *bluealsa_client_format_to_string(int pcm_format) {
	switch (pcm_format) {
	case 0x0108:
		return "U8";
	case 0x8210:
		return "S16_LE";
	case 0x8318:
		return "S24_3LE";
	case 0x8418:
		return "S24_LE";
	case 0x8420:
		return "S32_LE";
	default:
		return "Invalid";
	}
}

const char *bluealsa_client_transport_to_profile(int transport_code) {
	const char *profile = NULL;
	if (transport_code & BA_PCM_TRANSPORT_MASK_A2DP)
		profile = "A2DP";
	else if (transport_code & BA_PCM_TRANSPORT_MASK_HFP)
		profile = "HFP";
	else if (transport_code & BA_PCM_TRANSPORT_MASK_HSP)
		profile = "HSP";

	return profile;
}
