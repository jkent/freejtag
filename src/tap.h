/* SPDX-License-Identifier: MIT */
/*
 * FreeJTAG
 * Copyright (C) 2026 Jeff Kent <jeff@jkent.net>
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>


extern void tap_command(const uint8_t *buf, uint8_t len);
extern void tap_response(const uint8_t *buf, uint8_t len, bool flush);
