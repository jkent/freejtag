/* SPDX-License-Identifier: MIT */
/*
 * FreeJTAG
 * Copyright (C) 2026 Jeff Kent <jeff@jkent.net>
 */

#pragma once


void cdc_deinit(void);
void cdc_init(void);
void cdc_task(void);
void cdc_control_request(void);
