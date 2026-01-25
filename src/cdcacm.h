/* SPDX-License-Identifier: MIT */
/*
 * FreeJTAG
 * Copyright (C) 2026 Jeff Kent <jeff@jkent.net>
 */

#pragma once


#if !defined(CONTROL_ONLY_DEVICE)
void CDCACM_Init(void);
void CDCACM_Task(void);
void CDCACM_Configure(void);
#endif
