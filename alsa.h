/*
 * bluealsa-autoconfig - alsa.h
 * SPDX-FileCopyrightText: 2024-2025 @borine <https://github.com/borine>
 * SPDX-License-Identifier: MIT
 */

#pragma once
#ifndef BLUEALSA_AUTOCONFIG_ALSA_H
#define BLUEALSA_AUTOCONFIG_ALSA_H


void alsa_version_init(void);
unsigned int alsa_version_id(void);
const char *alsa_version_string(void);

#endif
