/* SPDX-License-Identifier: MIT */
/*
 * FreeJTAG
 * Copyright (C) 2026 Jeff Kent <jeff@jkent.net>
 */

#include <avr/io.h>


// OUT - White
#define FREEJTAG_TCK_BIT                _BV(5)
#define FREEJTAG_TCK_DDR                DDRB
#define FREEJTAG_TCK_PORT               PORTB

// IN - Yellow
#define FREEJTAG_TDO_BIT                _BV(6)
#define FREEJTAG_TDO_DDR                DDRB
#define FREEJTAG_TDO_PIN                PINB
#define FREEJTAG_TDO_PORT               PORTB

// OUT - Green
#define FREEJTAG_TMS_BIT                _BV(7)
#define FREEJTAG_TMS_DDR                DDRB
#define FREEJTAG_TMS_PORT               PORTB

// OUT - Blue
#define FREEJTAG_TDI_BIT                _BV(7)
#define FREEJTAG_TDI_DDR                DDRC
#define FREEJTAG_TDI_PORT               PORTC
