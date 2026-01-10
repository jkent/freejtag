/* SPDX-License-Identifier: MIT */
/*
 * FreeJTAG
 * Copyright (C) 2026 Jeff Kent <jeff@jkent.net>
 */

#pragma once


void jtag_deinit(void);
void jtag_init(void);
void jtag_task(void);
void jtag_control_request(void);
