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

struct bluealsa_agent {
	bluealsa_client_t client;
	char **progs;
	size_t prog_count;
	bool with_status;
};

typedef struct {
	char string[12][256];
	size_t count;
} envvars_t;

static volatile bool running = true;

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
			break;
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

static void bluealsa_agent_pcm_added(const struct ba_pcm *pcm, const char *service, void *data) {
	(void) data;
	struct bluealsa_agent *agent = data;
	envvars_t envvars;
	size_t n = 0;
	struct bluealsa_client_device device = { .path = pcm->device_path };
	const char *profile;

	bluealsa_client_get_device(agent->client, &device);

	sprintf(envvars.string[n++], "BLUEALSA_PCM_PROPERTY_ADDRESS=%s", device.hex_addr);
	sprintf(envvars.string[n++], "BLUEALSA_PCM_PROPERTY_NAME=%s", device.alias);
	if ((profile = bluealsa_client_transport_to_profile(pcm->transport)) != NULL)
		sprintf(envvars.string[n++], "BLUEALSA_PCM_PROPERTY_PROFILE=%s", profile);
	sprintf(envvars.string[n++], "BLUEALSA_PCM_PROPERTY_MODE=%s", bluealsa_client_mode_to_string(pcm->mode));
	sprintf(envvars.string[n++], "BLUEALSA_PCM_PROPERTY_CODEC=%s", pcm->codec.name);
	sprintf(envvars.string[n++], "BLUEALSA_PCM_PROPERTY_FORMAT=%s", bluealsa_client_format_to_string(pcm->format));
	sprintf(envvars.string[n++], "BLUEALSA_PCM_PROPERTY_CHANNELS=%u", pcm->channels);
	sprintf(envvars.string[n++], "BLUEALSA_PCM_PROPERTY_SAMPLING=%u", pcm->sampling);
	sprintf(envvars.string[n++], "BLUEALSA_PCM_PROPERTY_TRANSPORT=%s", bluealsa_client_transport_to_string(pcm->transport));
	sprintf(envvars.string[n++], "BLUEALSA_PCM_PROPERTY_SERVICE=%s", service);

	if (agent->with_status) {
info("setting status vars");
		sprintf(envvars.string[n++], "BLUEALSA_PCM_PROPERTY_RUNNING=%s", pcm->running ? "true" : "false");
		sprintf(envvars.string[n++], "BLUEALSA_PCM_PROPERTY_SOFTVOL=%s", pcm->soft_volume ? "true" : "false");
	}

	envvars.count = n;

	bluealsa_agent_run_progs(agent, "add", pcm->pcm_path, &envvars);

}

static void bluealsa_agent_pcm_removed(const char *path, void *data) {
	struct bluealsa_agent *agent = data;
	bluealsa_agent_run_progs(agent, "remove", path, NULL);
}

static void bluealsa_agent_pcm_updated(const char *path, const char *service, struct bluealsa_pcm_properties *props, void *data) {
	(void) service;
	struct bluealsa_agent *agent = data;
	envvars_t envvars;
	size_t n = 0;

	if (props->mask & BLUEALSA_PCM_PROPERTY_CHANGED_CODEC)
		sprintf(envvars.string[n++], "BLUEALSA_PCM_PROPERTY_CODEC=%s", props->codec);
	if (props->mask & BLUEALSA_PCM_PROPERTY_CHANGED_FORMAT)
		sprintf(envvars.string[n++], "BLUEALSA_PCM_PROPERTY_FORMAT=%s", bluealsa_client_format_to_string(props->format));
	if (props->mask & BLUEALSA_PCM_PROPERTY_CHANGED_CHANNELS)
		sprintf(envvars.string[n++], "BLUEALSA_PCM_PROPERTY_CHANNELS=%u", props->channels);
	if (props->mask & BLUEALSA_PCM_PROPERTY_CHANGED_SAMPLING)
		sprintf(envvars.string[n++], "BLUEALSA_PCM_PROPERTY_SAMPLING=%u", props->sampling);
	if (agent->with_status) {
		sprintf(envvars.string[n++], "BLUEALSA_PCM_PROPERTY_RUNNING=%s", props->running ? "true" : "false");
		sprintf(envvars.string[n++], "BLUEALSA_PCM_PROPERTY_SOFTVOL=%s", props->softvolume ? "true" : "false");
	}

	if (n == 0)
		return;

	envvars.count = n;
	bluealsa_agent_run_progs(agent, "update", path, &envvars);

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
	const char *opts = "hV:B:s";
	const struct option longopts[] = {
		{ "help", no_argument, NULL, 'h' },
		{ "version", no_argument, NULL, 'V' },
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
					"  -h, --help\t\tprint this help and exit\n"
					"  -V, --version\t\tprint version and exit\n"
					"  -B, --dbus=NAME\tBlueALSA service name suffix\n"
					"  -s, --status\t\thandle status change events\n"
					"\nPROGRAM:\n"
					"  path to program, or directory of programs, to be run "
					"when BlueALSA event occurs\n",
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
