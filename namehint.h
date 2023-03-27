/*
 * BlueALSA - namehint.h
 * Copyright (c) 2023 @borine (https://github.com/borine/)
 *
 * This project is licensed under the terms of the MIT license.
 *
 */
#ifndef BLUEALSA_AUTOCONFIG_NAMEHINT_H
#define BLUEALSA_AUTOCONFIG_NAMEHINT_H

#include <stdbool.h>
#include <stdio.h>
#include "bluealsa-client.h"

struct bluealsa_namehint;

int bluealsa_namehint_init(struct bluealsa_namehint **hint);
void bluealsa_namehint_free(struct bluealsa_namehint *hint);

bool bluealsa_namehint_pcm_add(struct bluealsa_namehint *hint, const struct ba_pcm *pcm, struct bluealsa_client *client, const char *service);

bool bluealsa_namehint_pcm_remove(struct bluealsa_namehint *hint, const char *path);

bool bluealsa_namehint_service_remove(struct bluealsa_namehint *hint, const char *service);

void bluealsa_namehint_remove_all(struct bluealsa_namehint *hint);

bool bluealsa_namehint_pcm_update(struct bluealsa_namehint *hint, const char *path, const char *codec);

void bluealsa_namehint_reset(struct bluealsa_namehint *hint);

int bluealsa_namehint_print(const struct bluealsa_namehint *hint, FILE *file, const char *pattern);

void bluealsa_namehint_print_default(struct bluealsa_namehint *hint, FILE *file);

#endif
