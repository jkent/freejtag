/* SPDX-License-Identifier: MIT */
/*
 * FreeJTAG
 * Copyright (C) 2026 Jeff Kent <jeff@jkent.net>
 */

#include <avr/io.h>


// OUT - White
#define JTAG_TCK_BIT                    _BV(5)
#define JTAG_TCK_DDR                    DDRB
#define JTAG_TCK_PIN                    PINB
#define JTAG_TCK_PORT                   PORTB

// IN - Yellow
#define JTAG_TDO_BIT                    _BV(6)
#define JTAG_TDO_DDR                    DDRB
#define JTAG_TDO_PIN                    PINB
#define JTAG_TDO_PORT                   PORTB

// OUT - Green
#define JTAG_TMS_BIT                    _BV(7)
#define JTAG_TMS_DDR                    DDRB
#define JTAG_TMS_PIN                    PINB
#define JTAG_TMS_PORT                   PORTB

// OUT - Blue
#define JTAG_TDI_BIT                    _BV(7)
#define JTAG_TDI_DDR                    DDRC
#define JTAG_TDI_PIN                    PINC
#define JTAG_TDI_PORT                   PORTC

#define JTAG_TMS(bit) ({ \
    if (bit) { \
        JTAG_TMS_PORT |= JTAG_TMS_BIT; \
    } else { \
        JTAG_TMS_PORT &= ~JTAG_TMS_BIT; \
    } \
})

#define JTAG_TDI(bit) ({ \
    if (bit) { \
        JTAG_TDI_PORT |= JTAG_TDI_BIT; \
    } else { \
        JTAG_TDI_PORT &= ~JTAG_TDI_BIT; \
    } \
})

#define JTAG_TCK(bit) ({ \
    if (bit) { \
        JTAG_TCK_PORT |= JTAG_TCK_BIT; \
    } else { \
        JTAG_TCK_PORT &= ~JTAG_TCK_BIT; \
    } \
})

#define JTAG_TDO() ({ \
    !!(JTAG_TDO_PIN & JTAG_TDO_BIT); \
})

#define JTAG_CLOCK() ({ \
    JTAG_TCK(1); \
    JTAG_TCK(0); \
})
