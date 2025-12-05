/*
 * bluealsa-autoconfig - alsa.c
 * SPDX-FileCopyrightText: 2024-2025 @borine <https://github.com/borine>
 * SPDX-License-Identifier: MIT
 */

#include <alsa/asoundlib.h>
#include <stdio.h>

#include "alsa.h"

#include <pthread.h>

static struct {
	const char *string;
	unsigned int id;
} alsa_version = { 0 };

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void alsa_version_init(void) {
	unsigned int major;
	unsigned int minor;
	unsigned int subminor;
	unsigned int extra;

	pthread_mutex_lock(&mutex);

	if (alsa_version.string != NULL) {
		pthread_mutex_unlock(&mutex);
		return;
	}

	alsa_version.string = snd_asoundlib_version();
	sscanf(alsa_version.string, "%u.%u.%u.%u",
				&major,
				&minor,
				&subminor,
				&extra);
	alsa_version.id = major << 16 | minor << 8 | subminor;
	pthread_mutex_unlock(&mutex);
}

unsigned int alsa_version_id(void) {
	if (alsa_version.string == NULL)
		alsa_version_init();
	return alsa_version.id;
}

const char *alsa_version_string(void) {
	if (alsa_version.string == NULL)
		alsa_version_init();
	return alsa_version.string;
}
