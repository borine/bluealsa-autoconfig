/*
 * bluealsa-autoconfig - alsa.h
 * Copyright (c) 2023 @borine (https://github.com/borine/)
 *
 * This project is licensed under the terms of the MIT license.
 *
 */
#ifndef BLUEALSA_AUTOCONFIG_ALSA_H
#define BLUEALSA_AUTOCONFIG_ALSA_H


void alsa_version_init(void);
unsigned int alsa_version_id(void);
const char *alsa_version_string(void);

#endif
