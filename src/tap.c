/* SPDX-License-Identifier: MIT */
/*
 * FreeJTAG
 * Copyright (C) 2026 Jeff Kent <jeff@jkent.net>
 */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "jtag_pins.h"
#include "tap.h"


typedef enum {
    TAP_STATE_UNKNOWN = 0,
    TAP_STATE_RESET,
    TAP_STATE_RUNIDLE,
    TAP_STATE_DRSELECT,
    TAP_STATE_DRCAPTURE,
    TAP_STATE_DRSHIFT,
    TAP_STATE_DREXIT1,
    TAP_STATE_DRPAUSE,
    TAP_STATE_DREXIT2,
    TAP_STATE_DRUPDATE,
    TAP_STATE_IRSELECT,
    TAP_STATE_IRCAPTURE,
    TAP_STATE_IRSHIFT,
    TAP_STATE_IREXIT1,
    TAP_STATE_IRPAUSE,
    TAP_STATE_IREXIT2,
    TAP_STATE_IRUPDATE,
} state_t;

typedef enum {
    TAP_CMD_NOP = 0,
    TAP_CMD_VERSION,
    TAP_CMD_ATTACH,
    TAP_CMD_SET_STATE,
    TAP_CMD_CLOCK,
    TAP_CMD_CLOCK_OUT,
    TAP_CMD_CLOCK_IN,
    TAP_CMD_CLOCK_OUTIN,
    TAP_CMD_BULK_LOAD_BYTES,
    TAP_CMD_BULK_READ_BYTES,
    TAP_CMD_AVR_READ_OCDR = 128,
    TAP_CMD_RESET = 255,
} tap_cmd_t;

#define IR_AVR_OCD          11
#define AVR_OCD_OCDR        12
#define AVR_OCD_CTRLSTATUS  13

static state_t state = TAP_STATE_UNKNOWN;
static uint16_t bulk_bytes;

static void tap_set_state(state_t new_state);
static void tap_shift_exit(bool exit);
static void tap_clock(uint8_t bits, bool exit);
static void tap_clock_out(uint8_t bits, uint32_t data, bool exit);
static uint32_t tap_clock_in(uint8_t bits, bool exit);
static uint32_t tap_clock_outin(uint8_t bits, uint32_t data, bool exit);
static void tap_bulk_load_bytes(const uint8_t *buf, uint8_t len);
static void tap_bulk_read_bytes(uint8_t len, bool flush);
static int16_t tap_avr_read_ocdr(void);

void tap_command(const uint8_t *buf, uint8_t len)
{
    assert(len != 0);

    if (bulk_bytes > 0) {
        uint8_t chunk = bulk_bytes < len ? bulk_bytes : len;
        tap_bulk_load_bytes(buf, chunk);
        bulk_bytes -= chunk;
        if (chunk < 8) {
            bulk_bytes = 0;
        }
        return;
    }

    switch (buf[0]) {
    case TAP_CMD_NOP:
        break;

    case TAP_CMD_VERSION: {
            assert(len == 1);

            const uint8_t version[2] = {0x00, 0x02};
            tap_response(version, sizeof(version), true);
        }
        break;

    case TAP_CMD_ATTACH: {
            assert(len == 2);

            bool value = !!buf[1];

            if (value) {
                JTAG_TCK_PORT &= ~JTAG_TCK_BIT;
                JTAG_TDO_PORT |= JTAG_TDO_BIT;
                JTAG_TMS_PORT |= JTAG_TMS_BIT;
                JTAG_TDI_PORT &= ~JTAG_TDI_BIT;

                JTAG_TCK_DDR |= JTAG_TCK_BIT;
                JTAG_TDO_DDR &= ~JTAG_TDO_BIT;
                JTAG_TMS_DDR |= JTAG_TMS_BIT;
                JTAG_TDI_DDR |= JTAG_TDI_BIT;

                tap_set_state(TAP_STATE_RESET);
            } else {
                JTAG_TCK_DDR &= ~JTAG_TCK_BIT;
                JTAG_TDO_DDR &= ~JTAG_TDO_BIT;
                JTAG_TMS_DDR &= ~JTAG_TMS_BIT;
                JTAG_TDI_DDR &= ~JTAG_TDI_BIT;

                JTAG_TCK_PORT &= ~JTAG_TCK_BIT;
                JTAG_TDO_PORT &= ~JTAG_TDO_BIT;
                JTAG_TMS_PORT &= ~JTAG_TMS_BIT;
                JTAG_TDI_PORT &= ~JTAG_TDI_BIT;
            }
        }
        break;

    case TAP_CMD_SET_STATE: {
            state_t new_state = buf[1];

            assert(len == 2);

            tap_set_state(new_state);
            tap_response(&state, 1, true);
        }
        break;

    case TAP_CMD_CLOCK: {
            uint8_t bits = buf[1];
            uint8_t exit = !!buf[2];

            assert(len == 3);
            assert(bits > 0 && bits <= 32);

            tap_clock(bits, exit);
        }
        break;

    case TAP_CMD_CLOCK_OUT: {
            uint8_t bits = buf[1];
            uint8_t bytes = (bits + 7) / 8;
            uint8_t exit = !!buf[2];
            uint32_t data = *(uint32_t *) &buf[3];

            assert(len >= 3);
            assert(bits > 0 && bits <= 32);
            assert(len == 3 + bytes);

            tap_clock_out(bits, data, exit);
        }
        break;

    case TAP_CMD_CLOCK_IN: {
            uint8_t bits = buf[1];
            uint8_t bytes = (bits + 7) / 8;
            uint8_t exit = !!buf[2];
            uint32_t data;

            assert(len == 3);
            assert(bits > 0 && bits <= 32);

            data = tap_clock_in(bits, exit);
            tap_response((const uint8_t *) &data, bytes, true);
        }
        break;

    case TAP_CMD_CLOCK_OUTIN: {
            uint8_t bits = buf[1];
            uint8_t bytes = (bits + 7) / 8;
            uint8_t exit = !!buf[2];
            uint32_t data = *(uint32_t *) &buf[3];

            assert(len >= 3);
            assert(bits > 0 && bits <= 32);
            assert(len == 3 + bytes);

            data = tap_clock_outin(bits, data, exit);
            tap_response((const uint8_t *) &data, bytes, true);
        }
        break;

    case TAP_CMD_BULK_LOAD_BYTES: {
            bulk_bytes = *(uint16_t *) &buf[1];

            assert(len == 3);
        }
        break;

    case TAP_CMD_BULK_READ_BYTES: {
            bulk_bytes = *(uint16_t *) &buf[1];

            assert(len == 3);

            while (bulk_bytes > 0) {
                uint8_t chunk = bulk_bytes > 8 ? 8 : bulk_bytes;
                tap_bulk_read_bytes(chunk, bulk_bytes <= 8);
                bulk_bytes -= chunk;
            }
        }
        break;

    case TAP_CMD_AVR_READ_OCDR: {
            int16_t ch;

            assert(len == 1);

            ch = tap_avr_read_ocdr();
            tap_response((uint8_t *) &ch, sizeof(ch), true);
        }
        break;

    case TAP_CMD_RESET: {
            assert(len == 1);

            bulk_bytes = 0;
        }
        break;

    default:
        break;
    }
}

static void tap_set_state(state_t new_state)
{
    switch (new_state) {
    case TAP_STATE_RESET:
        JTAG_TMS(1);
        JTAG_CLOCK(); /* ??? */
        JTAG_CLOCK(); /* ??? */
        JTAG_CLOCK(); /* ??? */
        JTAG_CLOCK(); /* ??? */
        JTAG_CLOCK(); /* RESET */
        break;

    case TAP_STATE_RUNIDLE:
        switch(state) {
        case TAP_STATE_RESET:
            JTAG_TMS(0);
            JTAG_CLOCK();   /* RUNIDLE */
            break;

        case TAP_STATE_DRCAPTURE:
        case TAP_STATE_DRPAUSE:
        case TAP_STATE_IRCAPTURE:
        case TAP_STATE_IRPAUSE:
            JTAG_TMS(1);
            JTAG_CLOCK();   /* DREXIT2/IREXIT2 */

        case TAP_STATE_DREXIT1:
        case TAP_STATE_DREXIT2:
        case TAP_STATE_IREXIT1:
        case TAP_STATE_IREXIT2:
            JTAG_TMS(1);
            JTAG_CLOCK();   /* DRUPDATE/IRUPDATE */

        case TAP_STATE_DRUPDATE:
        case TAP_STATE_IRUPDATE:
            JTAG_TMS(0);
            JTAG_CLOCK();   /* RUNIDLE */
            break;

        default:
            return;
        }
        break;

    case TAP_STATE_DRCAPTURE:
        switch (state) {
        case TAP_STATE_RESET:
            JTAG_TMS(0);
            JTAG_CLOCK();   /* RUNIDLE */

        case TAP_STATE_RUNIDLE:
        case TAP_STATE_DRUPDATE:
        case TAP_STATE_IRUPDATE:
            JTAG_TMS(1);
            JTAG_CLOCK();   /* DRSELECT */

        case TAP_STATE_DRSELECT:
            JTAG_TMS(0);
            JTAG_CLOCK();   /* DRCAPTURE */
            break;

        default:
            return;
        }
        break;

    case TAP_STATE_DRSHIFT:
        switch (state) {
        case TAP_STATE_RESET:
            JTAG_TMS(0);
            JTAG_CLOCK();   /* RUNIDLE */

        case TAP_STATE_RUNIDLE:
        case TAP_STATE_DRUPDATE:
        case TAP_STATE_IRUPDATE:
            JTAG_TMS(1);
            JTAG_CLOCK();   /* DRSELECT */

        case TAP_STATE_DRSELECT:
            JTAG_TMS(0);
            JTAG_CLOCK();   /* DRCAPTURE */

        case TAP_STATE_DRCAPTURE:
            JTAG_TMS(0);
            JTAG_CLOCK();   /* DRSHIFT */
            break;

        case TAP_STATE_DRPAUSE:
            JTAG_TMS(1);
            JTAG_CLOCK();   /* DREXIT2 */

        case TAP_STATE_DREXIT2:
            JTAG_TMS(0);
            JTAG_CLOCK();   /* DRSHIFT */
            break;

        case TAP_STATE_IRSELECT:
            JTAG_TMS(0);
            JTAG_CLOCK();   /* IRCAPTURE */

        case TAP_STATE_IRCAPTURE:
        case TAP_STATE_IRPAUSE:
            JTAG_TMS(1);
            JTAG_CLOCK();   /* IREXIT1/IREXIT2 */

        case TAP_STATE_IREXIT1:
        case TAP_STATE_IREXIT2:
            JTAG_TMS(1);
            JTAG_CLOCK();   /* IRUPDATE */
            JTAG_CLOCK();   /* DRSELECT */
            JTAG_TMS(0);
            JTAG_CLOCK();   /* DRCAPTURE */
            JTAG_CLOCK();   /* DRSHIFT */
            break;

        default:
            return;
        }
        break;

    case TAP_STATE_DRPAUSE:
        switch(state) {
        case TAP_STATE_RESET:
            JTAG_TMS(0);
            JTAG_CLOCK();   /* RUNIDLE */

        case TAP_STATE_RUNIDLE:
        case TAP_STATE_DRUPDATE:
        case TAP_STATE_IRUPDATE:
            JTAG_TMS(1);
            JTAG_CLOCK();   /* DRSELECT */

        case TAP_STATE_DRSELECT:
            JTAG_TMS(0);
            JTAG_CLOCK();   /* DRCAPTURE */

        case TAP_STATE_DRCAPTURE:
            JTAG_TMS(1);
            JTAG_CLOCK();   /* DREXIT1 */

        case TAP_STATE_DREXIT1:
            JTAG_TMS(0);
            JTAG_CLOCK();   /* DRPAUSE */
            break;

        case TAP_STATE_IRSELECT:
            JTAG_TMS(0);
            JTAG_CLOCK();   /* IRCAPTURE */

        case TAP_STATE_IRCAPTURE:
        case TAP_STATE_IRPAUSE:
            JTAG_TMS(1);
            JTAG_CLOCK();   /* IREXIT1/IREXIT2 */

        case TAP_STATE_IREXIT1:
        case TAP_STATE_IREXIT2:
            JTAG_TMS(1);
            JTAG_CLOCK();   /* IRUPDATE */
            JTAG_CLOCK();   /* DRSELECT */
            JTAG_TMS(0);
            JTAG_CLOCK();   /* DRCAPTURE */
            JTAG_TMS(1);
            JTAG_CLOCK();   /* DREXIT1 */
            JTAG_TMS(0);
            JTAG_CLOCK();   /* DRPAUSE */
            break;

        default:
            return;
        }
        break;

    case TAP_STATE_DRUPDATE:
        switch (state) {
        case TAP_STATE_DRCAPTURE:
            JTAG_TMS(1);
            JTAG_CLOCK();   /* DREXIT1 */
            JTAG_CLOCK();   /* DRUPDATE */
            break;

        case TAP_STATE_DRPAUSE:
            JTAG_TMS(1);
            JTAG_CLOCK();   /* DREXIT2 */

        case TAP_STATE_DREXIT1:
        case TAP_STATE_DREXIT2:
            JTAG_TMS(1);
            JTAG_CLOCK();   /* DRUPDATE */
            break;

        default:
            return;
        }
        break;

    case TAP_STATE_IRCAPTURE:
        switch (state) {
        case TAP_STATE_RESET:
            JTAG_TMS(0);
            JTAG_CLOCK();   /* RUNIDLE */

        case TAP_STATE_RUNIDLE:
        case TAP_STATE_DRUPDATE:
        case TAP_STATE_IRUPDATE:
            JTAG_TMS(1);
            JTAG_CLOCK();   /* DRSELECT */

        case TAP_STATE_DRSELECT:
            JTAG_TMS(1);
            JTAG_CLOCK();   /* IRSELECT */

        case TAP_STATE_IRSELECT:
            JTAG_TMS(0);
            JTAG_CLOCK();   /* IRCAPTURE */
            break;

        default:
            return;
        }
        break;

    case TAP_STATE_IRSHIFT:
        switch (state) {
        case TAP_STATE_RESET:
            JTAG_TMS(0);
            JTAG_CLOCK();   /* RUNIDLE */

        case TAP_STATE_RUNIDLE:
        case TAP_STATE_DRUPDATE:
        case TAP_STATE_IRUPDATE:
            JTAG_TMS(1);
            JTAG_CLOCK();   /* DRSELECT */

        case TAP_STATE_DRSELECT:
            JTAG_TMS(1);
            JTAG_CLOCK();   /* IRSELECT */

        case TAP_STATE_IRSELECT:
            JTAG_TMS(0);
            JTAG_CLOCK();   /* IRCAPTURE */

        case TAP_STATE_IRCAPTURE:
            JTAG_TMS(0);
            JTAG_CLOCK();   /* IRSHIFT */
            break;

        case TAP_STATE_IRPAUSE:
            JTAG_TMS(1);
            JTAG_CLOCK();   /* IREXIT2 */

        case TAP_STATE_IREXIT2:
            JTAG_TMS(0);
            JTAG_CLOCK();   /* IRSHIFT */
            break;

        case TAP_STATE_DRCAPTURE:
        case TAP_STATE_DRPAUSE:
            JTAG_TMS(1);
            JTAG_CLOCK();   /* DREXIT1/DREXIT2 */

        case TAP_STATE_DREXIT1:
        case TAP_STATE_DREXIT2:
            JTAG_TMS(1);
            JTAG_CLOCK();   /* DRUPDATE */
            JTAG_CLOCK();   /* DRSELECT */
            JTAG_CLOCK();   /* IRSELECT */
            JTAG_TMS(0);
            JTAG_CLOCK();   /* IRCAPTURE */
            JTAG_CLOCK();   /* IRSHIFT */
            break;

        default:
            return;
        }
        break;

    case TAP_STATE_IRPAUSE:
        switch (state) {
        case TAP_STATE_RESET:
            JTAG_TMS(0);
            JTAG_CLOCK();   /* RUNIDLE */

        case TAP_STATE_RUNIDLE:
        case TAP_STATE_DRUPDATE:
        case TAP_STATE_IRUPDATE:
            JTAG_TMS(1);
            JTAG_CLOCK();   /* DRSELECT */

        case TAP_STATE_DRSELECT:
            JTAG_TMS(1);
            JTAG_CLOCK();   /* IRSELECT */

        case TAP_STATE_IRSELECT:
            JTAG_TMS(0);
            JTAG_CLOCK();   /* IRCAPTURE */

        case TAP_STATE_IRCAPTURE:
            JTAG_TMS(1);
            JTAG_CLOCK();   /* IREXIT1 */

        case TAP_STATE_IREXIT1:
            JTAG_TMS(0);
            JTAG_CLOCK();   /* IRPAUSE */
            break;

        case TAP_STATE_DRCAPTURE:
        case TAP_STATE_DRPAUSE:
            JTAG_TMS(1);
            JTAG_CLOCK();   /* DREXIT1/DREXIT2 */

        case TAP_STATE_DREXIT1:
        case TAP_STATE_DREXIT2:
            JTAG_TMS(1);
            JTAG_CLOCK();   /* DRUPDATE */
            JTAG_CLOCK();   /* DRSELECT */
            JTAG_CLOCK();   /* IRSELECT */
            JTAG_TMS(0);
            JTAG_CLOCK();   /* IRCAPTURE */
            JTAG_TMS(1);
            JTAG_CLOCK();   /* IREXIT1 */
            JTAG_TMS(0);
            JTAG_CLOCK();   /* IRPAUSE */
            break;

        default:
            return;
        }
        break;

    case TAP_STATE_IRUPDATE:
        switch (state) {
        case TAP_STATE_IRCAPTURE:
            JTAG_TMS(1);
            JTAG_CLOCK();   /* IREXIT1 */
            JTAG_CLOCK();   /* IRUPDATE */
            break;

        case TAP_STATE_IRPAUSE:
            JTAG_TMS(1);
            JTAG_CLOCK();   /* IREXIT2 */

        case TAP_STATE_IREXIT1:
        case TAP_STATE_IREXIT2:
            JTAG_TMS(1);
            JTAG_CLOCK();   /* IRUPDATE */
            break;

        default:
            return;
        }
        break;

    default:
        return;
    }

    state = new_state;
}

static void tap_shift_exit(bool exit)
{
    JTAG_TMS(exit);
    if (exit) {
        switch (state) {
        case TAP_STATE_DRSHIFT:
            state = TAP_STATE_DREXIT1;
            break;

        case TAP_STATE_DRPAUSE:
            state = TAP_STATE_DREXIT2;
            break;

        case TAP_STATE_IRSHIFT:
            state = TAP_STATE_IREXIT1;
            break;

        case TAP_STATE_IRPAUSE:
            state = TAP_STATE_IREXIT2;
            break;

        default:
            break;
        }
    }
}

static void tap_clock(uint8_t bits, bool exit)
{
    JTAG_TDI(0);

    for (uint8_t i = 0; i < bits - 1; i++) {
        JTAG_CLOCK();
    }

    tap_shift_exit(exit);
    JTAG_CLOCK();
}

static void tap_clock_out(uint8_t bits, uint32_t value, bool exit)
{
    uint32_t mask = 1UL << (bits - 1);

    for (uint8_t i = 0; i < bits - 1; i++) {
        JTAG_TDI(value & 1);
        value >>= 1;
        if (JTAG_TDO()) {
            value |= mask;
        } else {
            value &= ~mask;
        }
        JTAG_CLOCK();
    }

    tap_shift_exit(exit);
    JTAG_TDI(value & 1);
    value >>= 1;
    if (JTAG_TDO()) {
        value |= mask;
    } else {
        value &= ~mask;
    }
    JTAG_CLOCK();
}

static uint32_t tap_clock_in(uint8_t bits, bool exit)
{
    uint32_t value = 0x0BADC0DE;
    uint32_t mask = 1UL << (bits - 1);

    JTAG_TDI(0);

    for (uint8_t i = 0; i < bits - 1; i++) {
        value >>= 1;
        if (JTAG_TDO()) {
            value |= (uint32_t) mask;
        } else {
            value &= ~mask;
        }
        JTAG_CLOCK();
    }

    tap_shift_exit(exit);
    value >>= 1;
    if (JTAG_TDO()) {
        value |= mask;
    } else {
        value &= ~mask;
    }
    JTAG_CLOCK();

    return value;
}

static uint32_t tap_clock_outin(uint8_t bits, uint32_t value, bool exit)
{
    uint32_t mask = 1UL << (bits - 1);

    for (uint8_t i = 0; i < bits - 1; i++) {
        JTAG_TDI(value & 1);
        value >>= 1;
        if (JTAG_TDO()) {
            value |= mask;
        } else {
            value &= ~mask;
        }
        JTAG_CLOCK();
    }

    tap_shift_exit(exit);
    JTAG_TDI(value & 1);
    value >>= 1;
    if (JTAG_TDO()) {
        value |= mask;
    } else {
        value &= ~mask;
    }
    JTAG_CLOCK();

    return value;
}

static void tap_bulk_load_bytes(const uint8_t *buf, uint8_t len)
{
    for (uint8_t i = 0; i < len; i++) {
        tap_set_state(TAP_STATE_DRSHIFT);
        tap_clock_out(8, buf[i], true);
        tap_set_state(TAP_STATE_RUNIDLE);
    }
}

static void tap_bulk_read_bytes(uint8_t len, bool flush)
{
    uint8_t buf[8];

    for (uint8_t i = 0; i < len; i++) {
        tap_set_state(TAP_STATE_DRSHIFT);
        buf[i] = tap_clock_in(8, true);
        tap_set_state(TAP_STATE_RUNIDLE);
    }

    tap_response(buf, len, flush);
}

static int16_t tap_avr_read_ocdr(void)
{
    uint8_t ir;
    uint16_t status;
    int16_t value = -1;

    tap_set_state(TAP_STATE_IRSHIFT);
    ir = tap_clock_outin(4, IR_AVR_OCD, true);
    tap_set_state(TAP_STATE_RUNIDLE);

    tap_set_state(TAP_STATE_DRSHIFT);
    tap_clock_out(5, AVR_OCD_CTRLSTATUS, true);
    tap_set_state(TAP_STATE_RUNIDLE);

    tap_set_state(TAP_STATE_DRSHIFT);
    status = tap_clock_in(16, true);
    tap_set_state(TAP_STATE_RUNIDLE);

    if (status & 0x10) {
        tap_set_state(TAP_STATE_DRSHIFT);
        tap_clock_out(5, AVR_OCD_OCDR, true);
        tap_set_state(TAP_STATE_RUNIDLE);

        tap_set_state(TAP_STATE_DRSHIFT);
        value = tap_clock_in(16, true) >> 8;
        tap_set_state(TAP_STATE_RUNIDLE);
    }

    tap_set_state(TAP_STATE_IRSHIFT);
    tap_clock_out(4, ir, true);
    tap_set_state(TAP_STATE_RUNIDLE);

    return value;
}
