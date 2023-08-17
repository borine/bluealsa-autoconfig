/*
 * bluealsa-autoconfig - autoconfig.c
 * Copyright (c) 2023 @borine (https://github.com/borine/)
 *
 * This project is licensed under the terms of the MIT license.
 *
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <alsa/asoundlib.h>
#include <alsa/conf.h>
#include <errno.h>
#include <getopt.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "alsa.h"
#include "bluealsa-client.h"
#include "bluez-alsa/shared/log.h"
#include "namehint.h"
#include "version.h"

#define BLUEALSA_AUTOCONFIG_CONFIG_DIR "/var/lib/alsa/conf.d"
#define BLUEALSA_AUTOCONFIG_CONFIG_FILE BLUEALSA_AUTOCONFIG_CONFIG_DIR "/bluealsa-autoconfig.conf"
#define BLUEALSA_AUTOCONFIG_TEMP_FILE BLUEALSA_AUTOCONFIG_CONFIG_DIR "/.bluealsa-autoconfig.tmp"
#define BLUEALSA_AUTOCONFIG_CONFIG_TEMPLATE "%n %p (%c)%lBluetooth Audio %s"
#define BLUEALSA_AUTOCONFIG_RUN_DIR  "/run/bluealsa-autoconfig"
#define BLUEALSA_AUTOCONFIG_DEFAULTS_FILE  BLUEALSA_AUTOCONFIG_RUN_DIR "/defaults.conf"
#define BLUEALSA_AUTOCONFIG_LOCK_FILE  BLUEALSA_AUTOCONFIG_RUN_DIR "/lock"

struct bluealsa_autoconfig {
	bluealsa_client_t *client;
	struct bluealsa_namehint *hints;
	int timeout;
};

static bool udev_events = false;
static bool defaults = false;
static volatile bool running = true;
static char *pattern = NULL;

static void bluealsa_autoconfig_get_pattern(char **pattern) {
	snd_config_t *node;
	const char *config_pattern = NULL;

	if (snd_config_search(snd_config, "defaults.bluealsa.namehint", &node) >= 0)
		snd_config_get_string(node, &config_pattern);

	if (config_pattern == NULL)
		config_pattern = BLUEALSA_AUTOCONFIG_CONFIG_TEMPLATE;

	*pattern = strdup(config_pattern);
}

static void bluealsa_autoconfig_udev_trigger(void) {
	int fd = open("/sys/class/sound/controlC0/uevent", O_WRONLY);
	if (fd < 0) {
		debug("Unable to simulate udev events: %s", strerror(errno));
		udev_events = false;
		return;
	}
	if (write(fd, "change", 6) != 6) {
		debug("Unable to simulate udev events: %s", strerror(errno));
		udev_events = false;
	}
	close(fd);
}

static int bluealsa_autoconfig_init_alsa(const char *progname) {
	int fd;

	snd_config_update();

	/* Get the version of the **runtime** libasound2 */
	alsa_version_init();

	/* Ensure the required directories exist */
	int ret = mkdir(BLUEALSA_AUTOCONFIG_CONFIG_DIR, 0775);
	if (ret < 0 && errno != EEXIST) {
		error(BLUEALSA_AUTOCONFIG_CONFIG_DIR ": %s\n", strerror(errno));
		return -1;
	}

	mode_t mask = umask(~(S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH));
	ret = mkdir(BLUEALSA_AUTOCONFIG_RUN_DIR, 0775);
	if (ret < 0 && errno != EEXIST) {
		error(BLUEALSA_AUTOCONFIG_RUN_DIR ": %s\n", strerror(errno));
		return -1;
	}

	/* Clear the defaults file */
	fd = open(BLUEALSA_AUTOCONFIG_DEFAULTS_FILE, O_CREAT|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
	if (fd < 0 && defaults) {
		warn("Unable to open defaults file %s: %s\n", BLUEALSA_AUTOCONFIG_DEFAULTS_FILE, strerror(errno));
	}
	else
		close(fd);
	umask(mask);

	/* To prevent two instances of this program running,
	 * we create an exclusive lock file. The file descriptor, and therefore the
	 * lock on it, will be released automatically by the kernel when this
	 * program instance terminates. */
	fd = open(BLUEALSA_AUTOCONFIG_LOCK_FILE, O_CREAT|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR);
	if (fd < 0) {
		error("Unable to create lock file %s: %s\n", BLUEALSA_AUTOCONFIG_LOCK_FILE, strerror(errno));
		return -1;
	}
	if (flock(fd, LOCK_EX|LOCK_NB) < 0) {
		error("Another instance of %s is already running\n", progname);
		close(fd);
		return -1;
	}

	/* Create or truncate the ALSA config file. */
	mask = umask(~(S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH));
	fd = open(BLUEALSA_AUTOCONFIG_CONFIG_FILE, O_CREAT|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	umask(mask);
	if (fd < 0) {
		error("%s: %s\n", BLUEALSA_AUTOCONFIG_CONFIG_FILE, strerror(errno));
		return -1;
	}
	close(fd);

	return 0;
}

void bluealsa_autoconfig_set_timeout(struct bluealsa_autoconfig *config) {
	config->timeout = 100;
}

static void bluealsa_autoconfig_clear_timeout(struct bluealsa_autoconfig *config) {
	config->timeout = -1;
}

static void bluealsa_autoconfig_pcm_added(const struct ba_pcm *pcm, const char *service, void *data) {
	struct bluealsa_autoconfig *config = data;
	if (bluealsa_namehint_pcm_add(config->hints, pcm, config->client, service))
		bluealsa_autoconfig_set_timeout(config);
}

static void bluealsa_autoconfig_pcm_removed(const char *path, void *data) {
	struct bluealsa_autoconfig *config = data;
	if (bluealsa_namehint_pcm_remove(config->hints, path))
		bluealsa_autoconfig_set_timeout(config);
}

static void bluealsa_autoconfig_pcm_updated(const char *path, const char *service, struct bluealsa_pcm_properties *props, void *data) {
	(void) service;
	struct bluealsa_autoconfig *config = data;
	if (! (props->mask & BLUEALSA_PCM_PROPERTY_CHANGED_CODEC))
		return;

	if (bluealsa_namehint_pcm_update(config->hints, path, props->codec))
		bluealsa_autoconfig_set_timeout(config);
}

static void bluealsa_autoconfig_service_stopped(const char *service, void *data) {
	struct bluealsa_autoconfig *config = data;
	if (bluealsa_namehint_service_remove(config->hints, service))
		bluealsa_autoconfig_set_timeout(config);
}

static int bluealsa_autoconfig_init_client(struct bluealsa_autoconfig *config) {
	int ret;
	struct bluealsa_client_callbacks callbacks = {
		bluealsa_autoconfig_pcm_added,
		bluealsa_autoconfig_pcm_removed,
		bluealsa_autoconfig_pcm_updated,
		bluealsa_autoconfig_service_stopped,
		config,
	};
	if ((ret = bluealsa_client_open(&config->client, &callbacks)) < 0) {
		error("Unable to open bluealsa interface");
		return ret;
	}

	return 0;
}

static int bluealsa_autoconfig_commit_changes(struct bluealsa_autoconfig *config) {
	mode_t mask = umask(~(S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH));
	FILE *file = fopen(BLUEALSA_AUTOCONFIG_TEMP_FILE, "w");
	umask(mask);
	if (file == NULL) {
		error("Unable to write to %s: %s", BLUEALSA_AUTOCONFIG_TEMP_FILE, strerror(errno));
		return -1;
	}

	bluealsa_namehint_print(config->hints, file, pattern, bluealsa_client_num_services(config->client) > 1);

	fclose(file);

	if (defaults) {
		mask = umask(~(S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH));
		file = fopen(BLUEALSA_AUTOCONFIG_DEFAULTS_FILE, "w+");
		umask(mask);
		if (file != NULL) {
			bluealsa_namehint_print_default(config->hints, file);
			fclose(file);
		}
	}

	rename(BLUEALSA_AUTOCONFIG_TEMP_FILE, BLUEALSA_AUTOCONFIG_CONFIG_FILE);

	if (udev_events)
		bluealsa_autoconfig_udev_trigger();

	bluealsa_namehint_reset(config->hints);

	return 0;
}

static void bluealsa_autoconfig_cleanup(struct bluealsa_autoconfig *config) {
	bluealsa_namehint_remove_all(config->hints);
	bluealsa_autoconfig_commit_changes(config);
	bluealsa_namehint_free(config->hints);
	if (config->client != NULL)
		bluealsa_client_close(config->client);
	free(pattern);
	unlink(BLUEALSA_AUTOCONFIG_LOCK_FILE);
}

static void bluealsa_autoconfig_terminate(int sig) {
	(void) sig;
	running = false;
}

int main(int argc, char *argv[]) {
	struct bluealsa_autoconfig config = {
		.timeout = -1,
	};

	char **services = malloc(sizeof(char*));
	services[0] = strdup(BLUEALSA_SERVICE);
	unsigned int services_count = 1;

	int opt;
	const char *opts = "hVlB:du";
	const struct option longopts[] = {
		{ "help", no_argument, NULL, 'h' },
		{ "version", no_argument, NULL, 'V' },
		{ "dbus", required_argument, NULL, 'B'},
		{ "default", no_argument, NULL, 'd' },
		{ "udev", no_argument, NULL, 'u' },
		{ 0, 0, 0, 0 },
	};

	while ((opt = getopt_long(argc, argv, opts, longopts, NULL)) != -1)
		switch (opt) {
		case 'h' /* --help */ :
			printf("Usage:\n"
					"  %s [OPTION]...\n"
					"\nOptions:\n"
					"  -h, --help\t\tprint this help and exit\n"
					"  -V, --version\t\tprint version and exit\n"
					"  -B, --dbus=NAME\tBlueALSA service name suffix\n"
					"  -d, --default\t\tmanagement of default PCM and CTL\n"
					"  -u, --udev\t\tsimulate soundcard udev events\n",
					argv[0]);
			return EXIT_SUCCESS;

		case 'V' /* --version */ :
			printf("%s\n", PACKAGE_VERSION);
			return EXIT_SUCCESS;

		case 'B' /* --dbus=NAME */ : {
			char service[32];
			snprintf(service, sizeof(service), BLUEALSA_SERVICE ".%s", optarg);
			services = realloc(services, (services_count + 1) * sizeof(char*));
			services[services_count++] = strdup(service);
			break;
		}

		case 'd' /* --default */ :
			defaults = true;
			break;

		case 'u' /* --udev */ :
			udev_events = true;
			break;

		default:
			fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
			return EXIT_FAILURE;
		}


	log_open(argv[0], false);

	if (bluealsa_autoconfig_init_alsa(argv[0]) < 0)
		return EXIT_FAILURE;

	debug("Runtime ALSA libasound version: %s", alsa_version_string());

	if (bluealsa_namehint_init(&config.hints) < 0) {
		error("Out of memory");
		return EXIT_FAILURE;
	}

	if (bluealsa_autoconfig_init_client(&config) < 0)
		return EXIT_FAILURE;

	bluealsa_autoconfig_get_pattern(&pattern);

	unsigned int index;
	for (index = 0; index < services_count; index++) {
		bluealsa_client_watch_service(config.client, services[index]);
		bluealsa_client_get_pcms(config.client, services[index]);
		free(services[index]);
	}
	free(services);

	struct sigaction sigact = { .sa_handler = bluealsa_autoconfig_terminate };
	sigaction(SIGTERM, &sigact, NULL);
	sigaction(SIGINT, &sigact, NULL);

	while (running) {
		struct pollfd pfds[10];
		nfds_t pfds_len = ARRAYSIZE(pfds);
		int res;

		if (bluealsa_client_poll_fds(config.client, pfds, &pfds_len) < 0) {
			error("Couldn't get D-Bus connection file descriptors");
			return EXIT_FAILURE;
		}

		if ((res = poll(pfds, pfds_len, config.timeout)) == -1 &&
				errno == EINTR)
			continue;

		if (res == -1)
			break;

		/* timeout */
		if (res == 0) {
			bluealsa_autoconfig_clear_timeout(&config);
			if (bluealsa_autoconfig_commit_changes(&config) == -1)
				return EXIT_FAILURE;
			continue;
		}

		bluealsa_client_poll_dispatch(config.client, pfds, pfds_len);
	}

	bluealsa_autoconfig_cleanup(&config);

	return EXIT_SUCCESS;
}
