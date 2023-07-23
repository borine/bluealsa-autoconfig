/*
 * bluealsa-autoconfig - alsa.c
 * Copyright (c) 2023 @borine (https://github.com/borine/)
 *
 * This project is licensed under the terms of the MIT license.
 *
 */

#include <alsa/asoundlib.h>
#include <stdio.h>

#include "alsa.h"

static struct {
	const char *string;
	unsigned int major;
	unsigned int minor;
	unsigned int subminor;
	unsigned int extra;
	unsigned int id;
} alsa_version = { 0 };


void alsa_version_init(void) {
	alsa_version.string = snd_asoundlib_version();
	sscanf(alsa_version.string, "%u.%u.%u.%u",
				&alsa_version.major,
				&alsa_version.minor,
				&alsa_version.subminor,
				&alsa_version.extra);
	alsa_version.id = (alsa_version.major << 16) | (alsa_version.minor << 8) | (alsa_version.subminor);

}

unsigned int alsa_version_id(void) {
	return alsa_version.id;
}

const char *alsa_version_string(void) {
	return alsa_version.string;
}
