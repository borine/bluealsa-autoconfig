/*
 * bluealsa-autoconfig - autoconfig-filepaths.h
 * SPDX-FileCopyrightText: 2024-2026 @borine <https://github.com/borine>
 * SPDX-License-Identifier: MIT
 */

#pragma once
#ifndef BLUEALSA_AUTOCONFIG_FILEPATHS_H
#define BLUEALSA_AUTOCONFIG_FILEPATHS_H

#define BLUEALSA_AUTOCONFIG_CONFIG_DIR "/var/lib/alsa/conf.d"
#define BLUEALSA_AUTOCONFIG_CONFIG_FILE BLUEALSA_AUTOCONFIG_CONFIG_DIR "/bluealsa-autoconfig.conf"
#define BLUEALSA_AUTOCONFIG_TEMP_FILE BLUEALSA_AUTOCONFIG_CONFIG_DIR "/.bluealsa-autoconfig.tmp"

#define BLUEALSA_AUTOCONFIG_RUN_DIR  "/run/bluealsa-autoconfig"
#define BLUEALSA_AUTOCONFIG_DEFAULTS_FILE  BLUEALSA_AUTOCONFIG_RUN_DIR "/defaults.conf"
#define BLUEALSA_AUTOCONFIG_LOCK_FILE  BLUEALSA_AUTOCONFIG_RUN_DIR "/lock"

#endif
