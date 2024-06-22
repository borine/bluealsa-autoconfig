/*
 * bluealsa-agent - agent.c
 * Copyright (c) 2024 @borine (https://github.com/borine/)
 *
 * This project is licensed under the terms of the MIT license.
 *
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <getopt.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>

#include "bluealsa-client.h"
#include "bluez-alsa/shared/log.h"
#include "version.h"

enum bluealsa_profile {
	PROFILE_ALL = 0,
	PROFILE_A2DP = BA_PCM_TRANSPORT_MASK_A2DP,
	PROFILE_SCO = BA_PCM_TRANSPORT_MASK_SCO,
};

enum bluealsa_mode {
	MODE_ALL = 0,
	MODE_SINK = BA_PCM_MODE_SINK,
	MODE_SOURCE = BA_PCM_MODE_SOURCE,
};

struct bluealsa_pcm_data {
	char path[128];
	char address[18];
	char alias[64];
	char profile[5];
	char mode[9];
	char codec[16];
	char transport[12];
	char service[32];
	char alsa_id[96];
};

struct bluealsa_agent {
	bluealsa_client_t client;
	char **progs;
	size_t prog_count;
	bool with_status;
	enum bluealsa_profile profile;
	enum bluealsa_mode mode;
	struct {
		struct bluealsa_pcm_data *data;
		size_t capacity;
		size_t count;
	} pcms;
};

typedef struct {
	char string[14][256];
	size_t count;
} envvars_t;


static volatile bool running = true;

static bool bluealsa_agent_filter(const struct bluealsa_agent *agent, const struct ba_pcm *pcm) {
	const bool profile_match = (agent->profile == PROFILE_ALL) ||
									(agent->profile & pcm->transport);
	const bool mode_match = (agent->mode == MODE_ALL) || (pcm->mode == agent->mode);
 	return profile_match && mode_match;
}


static struct bluealsa_pcm_data *bluealsa_agent_add_pcm_path(
				struct bluealsa_agent *agent,
				const struct ba_pcm *pcm,
				const char *service) {

	struct bluealsa_pcm_data *pcm_data;
	struct bluealsa_client_device device = { .path = pcm->device_path };
	const char *profile, *mode, *transport, *transport_type;

	if ((profile = bluealsa_client_transport_to_profile(pcm->transport)) == NULL)
		return NULL;

	if ((mode = bluealsa_client_mode_to_string(pcm->mode)) == NULL)
		return NULL;

	if ((transport = bluealsa_client_transport_to_role(pcm->transport)) == NULL)
		return NULL;

	if ((transport_type = bluealsa_client_transport_to_type(pcm->transport)) == NULL)
		return NULL;

	if (agent->pcms.count == agent->pcms.capacity) {
		const size_t new_size = 2 * agent->pcms.capacity;
		pcm_data = realloc(agent->pcms.data, new_size * sizeof(*agent->pcms.data));
		if (pcm_data == NULL)
			return NULL;

		agent->pcms.data = pcm_data;
		agent->pcms.capacity = new_size;
	}
	pcm_data = &agent->pcms.data[agent->pcms.count];

	bluealsa_client_get_device(agent->client, &device);

	memcpy(pcm_data->path, pcm->pcm_path, sizeof(pcm_data->path));
	memcpy(pcm_data->address, device.hex_addr, sizeof(pcm_data->address));
	memcpy(pcm_data->alias, device.alias, sizeof(pcm_data->alias));
	memcpy(pcm_data->profile, profile, sizeof(pcm_data->profile));
	memcpy(pcm_data->mode, mode, sizeof(pcm_data->mode));
	memcpy(pcm_data->codec, pcm->codec.name, sizeof(pcm_data->codec));
	memcpy(pcm_data->transport, transport, sizeof(pcm_data->transport));
	memcpy(pcm_data->service, service, sizeof(pcm_data->service));

	const bool show_service = (strcmp(service, "org.bluealsa.") > 0);
	snprintf(pcm_data->alsa_id, sizeof(pcm_data->alsa_id), "bluealsa:DEV=%s,PROFILE=%s%s%s", pcm_data->address, transport_type, show_service ? ",SRV=" : "", show_service ? service + strlen("org.bluealsa.") : "");

	agent->pcms.count++;
	return pcm_data;
}

static bool bluealsa_agent_remove_pcm_path(struct bluealsa_agent *agent, const char *path) {
	for (size_t n = 0; n < agent->pcms.count; n++) {
		if (strcmp(path, agent->pcms.data[n].path) == 0) {
			if (--agent->pcms.count > 0)
				memcpy(&agent->pcms.data[n], &agent->pcms.data[agent->pcms.count], sizeof(*agent->pcms.data));
			return true;
		}
	}
	return false;
}

static struct bluealsa_pcm_data *bluealsa_agent_find_pcm_data(struct bluealsa_agent *agent, const char *path) {
	for (size_t n = 0; n < agent->pcms.count; n++) {
		if (strcmp(path, agent->pcms.data[n].path) == 0)
			return &agent->pcms.data[n];
	}
	return NULL;
}

static void bluealsa_agent_run_prog(const char *prog, const char *event, const char *obj_path, envvars_t *envp, bool wait) {
	pid_t pid = fork();
	switch (pid) {
	case 0:
		{
			char *argv[] = {(char*)prog, (char*)event, (char*)obj_path, NULL};
			if (envp != NULL) {
				for (size_t n = 0; n < envp->count; n++)
					putenv(envp->string[n]);
			}
			execv(prog, argv);
			error("Failed to execute %s (%s)", prog, strerror(errno));
			exit(1);
		}
	case -1:
		error("Failed to fork process for %s (%s)", prog, strerror(errno));
		return;
	default:
		if (wait)
			waitpid(pid, NULL, 0);
		else
			signal(SIGCHLD,SIG_IGN);
		break;
	}
}

static void bluealsa_agent_run_progs(struct bluealsa_agent *agent, const char *event, const char *obj_path, envvars_t *envp) {
	const bool wait = agent->prog_count > 1;
	for (size_t n = 0; n < agent->prog_count; n++) {
		bluealsa_agent_run_prog(agent->progs[n], event, obj_path, envp, wait);
	}
}

static int cmpstringp(const void *p1, const void *p2) {
	return strcmp(*(const char **) p1, *(const char **) p2);
}

static void bluealsa_agent_get_progs(struct bluealsa_agent *agent, const char *program) {
	struct stat statbuf;
	DIR *dir;
	struct dirent *entry;
	char path[PATH_MAX];
	const size_t offset = strlen(program) + 1; 
	size_t capacity = 10;

	if (stat(program, &statbuf) != 0) {
		error("Cannot stat program path '%s' (%s)", program, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (S_ISDIR(statbuf.st_mode)) {
		if ((dir = opendir(program)) == NULL) {
			error("Cannot read directory '%s' (%s)", program, strerror(errno));
			exit(EXIT_FAILURE);
		}

		agent->progs = malloc(capacity * sizeof(char*));
		strcpy(path, program);
		strcat(path, "/");

		while ((entry = readdir(dir))) {
			if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
				continue;
			if (strlen(entry->d_name) + offset > PATH_MAX)
				continue;
			strcpy(path + offset, entry->d_name);
			if (agent->prog_count >= capacity) {
				capacity *= 2;
				agent->progs = realloc(agent->progs, capacity * sizeof(char*));
				if (agent->progs == NULL) {
					error("Out of memory");
					exit(EXIT_FAILURE);
				}
			}
			agent->progs[agent->prog_count] = strdup(path);
			if (agent->progs[agent->prog_count] == NULL) {
				error("Out of memory");
				exit(EXIT_FAILURE);
			}
			++agent->prog_count;
		}
		closedir(dir);

		qsort(agent->progs, agent->prog_count, sizeof(char *), cmpstringp);
	}
	else if (S_ISREG(statbuf.st_mode)) {
		agent->progs = malloc(sizeof(char*));
		agent->progs[0] = strdup(program);
		agent->prog_count = 1;
	}
	else {
		error("Invalid file type for program '%s'", program);
		exit(EXIT_FAILURE);
	}
}

static size_t bluealsa_agent_init_envvars(envvars_t *envvars, const struct bluealsa_pcm_data *pcm_data) {
	size_t n = 0;
	snprintf(envvars->string[n++], 256, "BLUEALSA_PCM_PROPERTY_ADDRESS=%s", pcm_data->address);
	snprintf(envvars->string[n++], 256, "BLUEALSA_PCM_PROPERTY_NAME=%s", pcm_data->alias);
	snprintf(envvars->string[n++], 256, "BLUEALSA_PCM_PROPERTY_PROFILE=%s", pcm_data->profile);
	snprintf(envvars->string[n++], 256, "BLUEALSA_PCM_PROPERTY_MODE=%s", pcm_data->mode);
	snprintf(envvars->string[n++], 256, "BLUEALSA_PCM_PROPERTY_CODEC=%s", pcm_data->codec);
	snprintf(envvars->string[n++], 256, "BLUEALSA_PCM_PROPERTY_TRANSPORT=%s", pcm_data->transport);
	snprintf(envvars->string[n++], 256, "BLUEALSA_PCM_PROPERTY_SERVICE=%s", pcm_data->service);
	snprintf(envvars->string[n++], 256, "BLUEALSA_PCM_PROPERTY_ALSA_ID=%s", pcm_data->alsa_id);
	envvars->count = n;
	return n;
}

static void bluealsa_agent_pcm_added(const struct ba_pcm *pcm, const char *service, void *data) {
	(void) data;
	struct bluealsa_agent *agent = data;
	struct bluealsa_pcm_data *pcm_data;
	envvars_t envvars;
	size_t n;

	const char *value;
	char buffer[64];

	if (!bluealsa_agent_filter(agent, pcm))
		return;

	if ((pcm_data = bluealsa_agent_add_pcm_path(agent, pcm, service)) == NULL) {
		error("Out of memory");
		return;
	}

	n = bluealsa_agent_init_envvars(&envvars, pcm_data);

	if (pcm->codec.data_len > 0) {
		bluealsa_client_codec_blob_to_string(&pcm->codec, buffer, sizeof(buffer));
		snprintf(envvars.string[n++], 256, "BLUEALSA_PCM_PROPERTY_CODEC_CONFIG=%s", buffer);
	}
	if ((value = bluealsa_client_format_to_string(pcm->format)) != NULL)
		snprintf(envvars.string[n++], 256, "BLUEALSA_PCM_PROPERTY_FORMAT=%s", value);
	snprintf(envvars.string[n++], 256, "BLUEALSA_PCM_PROPERTY_CHANNELS=%u", pcm->channels);
	snprintf(envvars.string[n++], 256, "BLUEALSA_PCM_PROPERTY_SAMPLING=%u", pcm->sampling);

	if (agent->with_status) {
		snprintf(envvars.string[n++], 256, "BLUEALSA_PCM_PROPERTY_RUNNING=%s", pcm->running ? "true" : "false");
		snprintf(envvars.string[n++], 256, "BLUEALSA_PCM_PROPERTY_SOFTVOL=%s", pcm->soft_volume ? "true" : "false");
	}

	envvars.count = n;

	bluealsa_agent_run_progs(agent, "add", pcm->pcm_path, &envvars);

}

static void bluealsa_agent_pcm_removed(const char *path, void *data) {
	struct bluealsa_agent *agent = data;
	const struct bluealsa_pcm_data *pcm_data;
	envvars_t envvars;

	if ((pcm_data = bluealsa_agent_find_pcm_data(agent, path)) == NULL)
		return;

	bluealsa_agent_init_envvars(&envvars, pcm_data);

	if (bluealsa_agent_remove_pcm_path(agent, path))
		bluealsa_agent_run_progs(agent, "remove", path, &envvars);
}

static void bluealsa_agent_pcm_updated(const char *path, const char *service, struct bluealsa_pcm_properties *props, void *data) {
	(void) service;
	struct bluealsa_agent *agent = data;
	envvars_t envvars;
	size_t n;
	char buffer[64], changes[64] = {0};
	struct bluealsa_pcm_data *pcm_data;

	if ((props->mask & ~(BLUEALSA_PCM_PROPERTY_CHANGED_VOLUME | BLUEALSA_PCM_PROPERTY_CHANGED_DELAY)) == 0)
		return;

	if ((pcm_data = bluealsa_agent_find_pcm_data(agent, path)) == NULL)
		return;

	if (props->mask & BLUEALSA_PCM_PROPERTY_CHANGED_CODEC) {
		memcpy(pcm_data->codec, props->codec.name, sizeof(pcm_data->codec));
		strcpy(changes, "CODEC ");
	}

	n = bluealsa_agent_init_envvars(&envvars, pcm_data);

	if (props->mask & BLUEALSA_PCM_PROPERTY_CHANGED_CODEC_CONFIG) {
		bluealsa_client_codec_blob_to_string(&props->codec, buffer, sizeof(buffer));
		snprintf(envvars.string[n++], 256, "BLUEALSA_PCM_PROPERTY_CODEC_CONFIG=%s", buffer);
		strcat(changes, "CODEC_CONFIG ");
	}
	if (props->mask & BLUEALSA_PCM_PROPERTY_CHANGED_FORMAT) {
		snprintf(envvars.string[n++], 256, "BLUEALSA_PCM_PROPERTY_FORMAT=%s", bluealsa_client_format_to_string(props->format));
		strcat(changes, "FORMAT ");
	}
	if (props->mask & BLUEALSA_PCM_PROPERTY_CHANGED_CHANNELS) {
		snprintf(envvars.string[n++], 256, "BLUEALSA_PCM_PROPERTY_CHANNELS=%u", props->channels);
		strcat(changes, "CHANNELS ");
	}
	if (props->mask & BLUEALSA_PCM_PROPERTY_CHANGED_SAMPLING) {
		snprintf(envvars.string[n++], 256, "BLUEALSA_PCM_PROPERTY_SAMPLING=%u", props->sampling);
		strcat(changes, "SAMPLING ");
	}
	if (agent->with_status) {
		if (props->mask & BLUEALSA_PCM_PROPERTY_CHANGED_RUNNING) {
			snprintf(envvars.string[n++], 256, "BLUEALSA_PCM_PROPERTY_RUNNING=%s", props->running ? "true" : "false");
			strcat(changes, "RUNNING ");
		}
		if (props->mask & BLUEALSA_PCM_PROPERTY_CHANGED_SOFTVOL) {
			snprintf(envvars.string[n++], 256, "BLUEALSA_PCM_PROPERTY_SOFTVOL=%s", props->softvolume ? "true" : "false");
			strcat(changes, "SOFTVOL ");
		}
	}
	if (strlen(changes) > 0) {
		changes[strlen(changes) - 1] = '\0';
		snprintf(envvars.string[n++], 256, "BLUEALSA_PCM_PROPERTY_CHANGES=%s", changes);
		envvars.count = n;
		bluealsa_agent_run_progs(agent, "update", path, &envvars);
	}

}

static int bluealsa_agent_init_client(struct bluealsa_agent *agent) {
	int ret;
	struct bluealsa_client_callbacks callbacks = {
		bluealsa_agent_pcm_added,
		bluealsa_agent_pcm_removed,
		bluealsa_agent_pcm_updated,
		NULL,
		agent,
	};
	if ((ret = bluealsa_client_open(&agent->client, &callbacks)) < 0) {
		error("Unable to open bluealsa interface (%s)", strerror(-ret));
		return ret;
	}

	return 0;
}

static void bluealsa_agent_terminate(int sig) {
	(void) sig;
	running = false;
}

int main(int argc, char *argv[]) {

	struct bluealsa_agent agent = { 0 };

	char **services = malloc(sizeof(char*));
	services[0] = strdup(BLUEALSA_SERVICE);
	unsigned int services_count = 1;
	const char *programs = NULL;

	int opt;
	const char *opts = "hVp:m:B:s";
	const struct option longopts[] = {
		{ "help", no_argument, NULL, 'h' },
		{ "version", no_argument, NULL, 'V' },
		{ "profile", required_argument, NULL, 'p' },
		{ "mode", required_argument, NULL, 'm' },
		{ "dbus", required_argument, NULL, 'B'},
		{ "status", no_argument, NULL, 's' },
		{ 0, 0, 0, 0 },
	};

	while ((opt = getopt_long(argc, argv, opts, longopts, NULL)) != -1)
		switch (opt) {
		case 'h' /* --help */ :
			printf("%1$s - Utility to run BlueALSA event handler\n"
					"\nUsage:\n"
					"  %1$s [OPTION]... PROGRAM\n"
					"\nOptions:\n"
					"  -h, --help\t\t\tprint this help and exit\n"
					"  -V, --version\t\t\tprint version and exit\n"
					"  -p, --profile=[a2dp|sco]\tselect only given profile\n"
					"  -m, --mode=[sink|source]\tselect only given mode\n"
					"  -B, --dbus=NAME\t\tBlueALSA service name suffix\n"
					"  -s, --status\t\t\thandle status change events\n"
					"\nPROGRAM:\n"
					"  path to program, or directory of programs, to be run "
					"when BlueALSA event occurs\n",
					argv[0]);
			return EXIT_SUCCESS;

		case 'V' /* --version */ :
			printf("%s\n", PACKAGE_VERSION);
			return EXIT_SUCCESS;

		case 'p' /* --profile=[a2dp|sco] */ : {
			if (strcmp(optarg, "a2dp") == 0)
				agent.profile = PROFILE_A2DP;
			else if (strcmp(optarg, "sco") == 0)
				agent.profile = PROFILE_SCO;
			else {
				fprintf(stderr, "Invalid profile (%s)\n", optarg);
				return EXIT_FAILURE;
			}
			break;
		}

		case 'm' /* --mode=[sink|source] */ : {
			if (strcmp(optarg, "sink") == 0)
				agent.mode = MODE_SINK;
			else if (strcmp(optarg, "source") == 0)
				agent.mode = MODE_SOURCE;
			else {
				fprintf(stderr, "Invalid mode (%s)\n", optarg);
				return EXIT_FAILURE;
			}
			break;
		}

		case 'B' /* --dbus=NAME */ : {
			char service[32];
			snprintf(service, sizeof(service), BLUEALSA_SERVICE ".%s", optarg);
			services = realloc(services, (services_count + 1) * sizeof(char*));
			services[services_count++] = strdup(service);
			break;
		}
		case 's' /* --status */ :
			agent.with_status = true;
			break;

		default:
			fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
			return EXIT_FAILURE;
		}

	log_open(argv[0], false);

	if (optind == argc) {
		error("No program(s) specified");
		exit(EXIT_FAILURE);
	}
	programs = argv[optind];

	agent.pcms.data = calloc(8, sizeof(*agent.pcms.data));
	if (agent.pcms.data == NULL) {
		error("Out of memory");
		exit(EXIT_FAILURE);
	}
	agent.pcms.capacity = 8;

	bluealsa_agent_get_progs(&agent, programs);
	if (agent.prog_count == 0)
		exit(EXIT_SUCCESS);

	if (bluealsa_agent_init_client(&agent) < 0)
		return EXIT_FAILURE;

	unsigned int index;
	for (index = 0; index < services_count; index++) {
		bluealsa_client_watch_service(agent.client, services[index]);
		bluealsa_client_get_pcms(agent.client, services[index]);
		free(services[index]);
	}
	free(services);

	struct sigaction sigact = { .sa_handler = bluealsa_agent_terminate };
	sigaction(SIGTERM, &sigact, NULL);
	sigaction(SIGINT, &sigact, NULL);

	while (running) {
		struct pollfd pfds[10];
		nfds_t pfds_len = ARRAYSIZE(pfds);
		int res;

		if (bluealsa_client_poll_fds(agent.client, pfds, &pfds_len) < 0) {
			error("Couldn't get D-Bus connection file descriptors");
			return EXIT_FAILURE;
		}

		if ((res = poll(pfds, pfds_len, -1)) == -1 &&
				errno == EINTR)
			continue;

		if (res == -1)
			break;

		bluealsa_client_poll_dispatch(agent.client, pfds, pfds_len);
	}

	bluealsa_client_close(agent.client);

	return EXIT_SUCCESS;
}
