/* SPDX-License-Identifier: MIT */
/*
 * FreeJTAG
 * Copyright (C) 2026 Jeff Kent <jeff@jkent.net>
 */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "descriptors.h"
#include "freejtag_pins.h"
#include "freejtag.h"


typedef enum {
    FREEJTAG_STATE_RESET            = 0x0,
    FREEJTAG_STATE_RUNIDLE,
    FREEJTAG_STATE_DRSELECT,
    FREEJTAG_STATE_DRCAPTURE,
    FREEJTAG_STATE_DRSHIFT,
    FREEJTAG_STATE_DREXIT1,
    FREEJTAG_STATE_DRPAUSE,
    FREEJTAG_STATE_DREXIT2,
    FREEJTAG_STATE_DRUPDATE,
    FREEJTAG_STATE_IRSELECT,
    FREEJTAG_STATE_IRCAPTURE,
    FREEJTAG_STATE_IRSHIFT,
    FREEJTAG_STATE_IREXIT1,
    FREEJTAG_STATE_IRPAUSE,
    FREEJTAG_STATE_IREXIT2,
    FREEJTAG_STATE_IRUPDATE,
    FREEJTAG_STATE_UNKNOWN,
} freejtag_state_t;

typedef enum {
    FREEJTAG_REQ_VERSION            = 0x00, // IN
    FREEJTAG_REQ_RESET,                     // OUT
    FREEJTAG_REQ_EXECUTE,                   // OUT
    FREEJTAG_REQ_READBUF,                   // IN
    FREEJTAG_REQ_BULKBYTE,                  // OUT & IN
#if !defined(MINI_FREEJTAG)
    FREEJTAG_REQ_READOCDR           = 0x80, // IN
#endif
} freejtag_req_t;

typedef enum {
    FREEJTAG_CMD_NOP                = 0x00,
    FREEJTAG_CMD_ATTACH,
    FREEJTAG_CMD_SET_TDI,
    FREEJTAG_CMD_SET_TMS,
    FREEJTAG_CMD_SET_STATE,
    FREEJTAG_CMD_CLOCK,
    FREEJTAG_CMD_SHIFT,
    FREEJTAG_CMD_SHIFT_EXIT,
    FREEJTAG_CMD_SHIFT_OUT          = 0x40,
    FREEJTAG_CMD_SHIFT_OUT_EXIT,
    FREEJTAG_CMD_SHIFT_IN           = 0x80,
    FREEJTAG_CMD_SHIFT_IN_EXIT,
    FREEJTAG_CMD_SHIFT_OUTIN        = 0xC0,
    FREEJTAG_CMD_SHIFT_OUTIN_EXIT,
} freejtag_cmd_t;

#define IR_AVR_OCD          11
#define AVR_OCD_OCDR        12
#define AVR_OCD_CTRLSTATUS  13

static freejtag_state_t state;
static uint8_t rxbuf[FIXED_CONTROL_ENDPOINT_SIZE];
static uint8_t txbuf[FIXED_CONTROL_ENDPOINT_SIZE];
static uint8_t rxlen, txlen;

static void FreeJTAG_Attach(bool attach);
static void FreeJTAG_SetState(freejtag_state_t new_state);
static void FreeJTAG_ShiftExit(void);
static void FreeJTAG_Shift(int bits, bool exit);
static void FreeJTAG_ShiftOutBuf(int bits, bool exit);
static void FreeJTAG_ShiftInBuf(int bits, bool exit);
static void FreeJTAG_ShiftOutInBuf(int bits, bool exit);
static void FreeJTAG_BulkWrite(void);
static void FreeJTAG_BulkRead(void);
#if !defined(MINI_FREEJTAG)
static uint32_t FreeJTAG_ShiftOutIn(int bits, uint32_t value);
static int16_t FreeJTAG_AVR_ReadOCDR(void);
#endif

void FreeJTAG_Init(void)
{
    state = FREEJTAG_STATE_UNKNOWN;
    txlen = 0;
}

void FreeJTAG_ControlRequest(void)
{
    uint8_t cmd, val;

    if (!Endpoint_IsSETUPReceived()) {
        return;
    }

    if ((USB_ControlRequest.bmRequestType & 0x7F) !=
            (REQTYPE_VENDOR | REQREC_INTERFACE) ||
            (USB_ControlRequest.wIndex & 0xff) != INTERFACE_ID_FREEJTAG) {
        return;
    }

    cmd = USB_ControlRequest.wValue & 0xff;
    val = USB_ControlRequest.wValue >> 8;

    if (USB_ControlRequest.bmRequestType & REQDIR_DEVICETOHOST) {
        switch (USB_ControlRequest.bRequest) {
        case FREEJTAG_REQ_VERSION: {
                const uint16_t version = VERSION_BCD(3,0,0);
                Endpoint_ClearSETUP();
                Endpoint_Write_Control_Stream_LE(&version, sizeof(version));
                Endpoint_ClearOUT();
                break;
            }

        case FREEJTAG_REQ_READBUF:
            Endpoint_ClearSETUP();
            Endpoint_Write_Control_Stream_LE(txbuf, txlen);
            Endpoint_ClearOUT();
            txlen = 0;
            break;

        case FREEJTAG_REQ_BULKBYTE:
            Endpoint_ClearSETUP();
            txlen = USB_ControlRequest.wLength < FIXED_CONTROL_ENDPOINT_SIZE ?
                    USB_ControlRequest.wLength : FIXED_CONTROL_ENDPOINT_SIZE;
            FreeJTAG_BulkRead();
            Endpoint_Write_Control_Stream_LE(txbuf, txlen);
            Endpoint_ClearOUT();
            txlen = 0;
            break;

#if !defined(MINI_FREEJTAG)
        case FREEJTAG_REQ_READOCDR: {
                int16_t value = FreeJTAG_AVR_ReadOCDR();
                Endpoint_ClearSETUP();
                Endpoint_Write_Control_Stream_LE(&value, sizeof(value));
                Endpoint_ClearOUT();
            }
            break;
#endif
        }
    } else {
        switch (USB_ControlRequest.bRequest) {
        case FREEJTAG_REQ_RESET:
            Endpoint_ClearSETUP();
            Endpoint_ClearStatusStage();
            state = FREEJTAG_STATE_UNKNOWN;
            txlen = 0;
            break;

        case FREEJTAG_REQ_EXECUTE: {
                Endpoint_ClearSETUP();

                switch (cmd) {
                case FREEJTAG_CMD_NOP:
                    break;

                case FREEJTAG_CMD_ATTACH:
                    FreeJTAG_Attach(!!val);
                    break;

                case FREEJTAG_CMD_SET_TDI:
                   FREEJTAG_TDI(!!val);
                   break;

                case FREEJTAG_CMD_SET_TMS:
                   FREEJTAG_TMS(!!val);
                   break;

                case FREEJTAG_CMD_SET_STATE:
                    FreeJTAG_SetState(val & 0xf);
                    break;

                case FREEJTAG_CMD_CLOCK: {
                        int cycles = val + 1;

                        for (int i = 0; i < cycles; i++) {
                            FREEJTAG_CLOCK();
                        }
                    }
                    break;

                case FREEJTAG_CMD_SHIFT:
                case FREEJTAG_CMD_SHIFT_EXIT: {
                        int bits = val + 1;

                        FreeJTAG_Shift(bits, cmd == FREEJTAG_CMD_SHIFT_EXIT);
                    }
                    break;

                case FREEJTAG_CMD_SHIFT_OUT:
                case FREEJTAG_CMD_SHIFT_OUT_EXIT: {
                        int bits = val + 1;

                        rxlen = (bits + 7) / 8;
                        Endpoint_Read_Control_Stream_LE(rxbuf, rxlen);
                        FreeJTAG_ShiftOutBuf(bits,
                                cmd == FREEJTAG_CMD_SHIFT_OUT_EXIT);
                    }
                    break;

                case FREEJTAG_CMD_SHIFT_IN:
                case FREEJTAG_CMD_SHIFT_IN_EXIT: {
                        int bits = val + 1;

                        txlen = (bits + 7) / 8;
                        FreeJTAG_ShiftInBuf(bits,
                                cmd == FREEJTAG_CMD_SHIFT_IN_EXIT);
                    }
                    break;

                case FREEJTAG_CMD_SHIFT_OUTIN:
                case FREEJTAG_CMD_SHIFT_OUTIN_EXIT: {
                        int bits = val + 1;

                        rxlen = (bits + 7) / 8;
                        txlen = (bits + 7) / 8;
                        Endpoint_Read_Control_Stream_LE(rxbuf, rxlen);
                        FreeJTAG_ShiftOutInBuf(bits,
                                cmd == FREEJTAG_CMD_SHIFT_OUTIN_EXIT);
                    }
                    break;
                }
                Endpoint_ClearStatusStage();
            }
            break;

        case FREEJTAG_REQ_BULKBYTE:
            rxlen = USB_ControlRequest.wLength < FIXED_CONTROL_ENDPOINT_SIZE ?
                    USB_ControlRequest.wLength : FIXED_CONTROL_ENDPOINT_SIZE;
            Endpoint_ClearSETUP();
            Endpoint_Read_Control_Stream_LE(rxbuf, rxlen);
            FreeJTAG_BulkWrite();
            Endpoint_ClearStatusStage();
            break;
        }
    }
}

static void FreeJTAG_Attach(bool attach)
{
    if (attach) {
        FREEJTAG_TCK_PORT &= ~FREEJTAG_TCK_BIT;
        FREEJTAG_TDO_PORT |= FREEJTAG_TDO_BIT;
        FREEJTAG_TMS_PORT |= FREEJTAG_TMS_BIT;
        FREEJTAG_TDI_PORT &= ~FREEJTAG_TDI_BIT;

        FREEJTAG_TCK_DDR |= FREEJTAG_TCK_BIT;
        FREEJTAG_TDO_DDR &= ~FREEJTAG_TDO_BIT;
        FREEJTAG_TMS_DDR |= FREEJTAG_TMS_BIT;
        FREEJTAG_TDI_DDR |= FREEJTAG_TDI_BIT;

        FREEJTAG_TMS(1);
        FreeJTAG_Shift(1024, false);
        state = FREEJTAG_STATE_RESET;
    } else {
        FREEJTAG_TCK_DDR &= ~FREEJTAG_TCK_BIT;
        FREEJTAG_TDO_DDR &= ~FREEJTAG_TDO_BIT;
        FREEJTAG_TMS_DDR &= ~FREEJTAG_TMS_BIT;
        FREEJTAG_TDI_DDR &= ~FREEJTAG_TDI_BIT;

        FREEJTAG_TCK_PORT &= ~FREEJTAG_TCK_BIT;
        FREEJTAG_TDO_PORT &= ~FREEJTAG_TDO_BIT;
        FREEJTAG_TMS_PORT &= ~FREEJTAG_TMS_BIT;
        FREEJTAG_TDI_PORT &= ~FREEJTAG_TDI_BIT;
    }
}

static void FreeJTAG_SetState(freejtag_state_t new_state)
{
    FREEJTAG_TDI(1);

    switch (new_state) {
    case FREEJTAG_STATE_RESET:
        FREEJTAG_TMS(1);
        FREEJTAG_CLOCK(); /* ??? */
        FREEJTAG_CLOCK(); /* ??? */
        FREEJTAG_CLOCK(); /* ??? */
        FREEJTAG_CLOCK(); /* ??? */
        FREEJTAG_CLOCK(); /* RESET */
        break;

    case FREEJTAG_STATE_RUNIDLE:
        switch(state) {
        case FREEJTAG_STATE_RESET:
            FREEJTAG_TMS(0);
            FREEJTAG_CLOCK();   /* RUNIDLE */
            break;

        case FREEJTAG_STATE_DREXIT1:
        case FREEJTAG_STATE_DREXIT2:
        case FREEJTAG_STATE_IREXIT1:
        case FREEJTAG_STATE_IREXIT2:
            FREEJTAG_TMS(1);
            FREEJTAG_CLOCK();   /* DRUPDATE/IRUPDATE */

        case FREEJTAG_STATE_DRUPDATE:
        case FREEJTAG_STATE_IRUPDATE:
            FREEJTAG_TMS(0);
            FREEJTAG_CLOCK();   /* RUNIDLE */
            break;

        default:
            return;
        }
        break;

    case FREEJTAG_STATE_DRSHIFT:
        switch (state) {
        case FREEJTAG_STATE_RESET:
            FREEJTAG_TMS(0);
            FREEJTAG_CLOCK();   /* RUNIDLE */

        case FREEJTAG_STATE_RUNIDLE:
        case FREEJTAG_STATE_DRUPDATE:
        case FREEJTAG_STATE_IRUPDATE:
            FREEJTAG_TMS(1);
            FREEJTAG_CLOCK();   /* DRSELECT */
            FREEJTAG_TMS(0);
            FREEJTAG_CLOCK();   /* DRCAPTURE */
            FREEJTAG_CLOCK();   /* DRSHIFT */
            break;

        case FREEJTAG_STATE_DREXIT2:
            FREEJTAG_TMS(0);
            FREEJTAG_CLOCK();   /* DRSHIFT */
            break;

        case FREEJTAG_STATE_IREXIT1:
        case FREEJTAG_STATE_IREXIT2:
            FREEJTAG_TMS(1);
            FREEJTAG_CLOCK();   /* IRUPDATE */
            FREEJTAG_CLOCK();   /* DRSELECT */
            FREEJTAG_TMS(0);
            FREEJTAG_CLOCK();   /* DRCAPTURE */
            FREEJTAG_CLOCK();   /* DRSHIFT */
            break;

        default:
            return;
        }
        break;

    case FREEJTAG_STATE_DRPAUSE:
        switch(state) {
        case FREEJTAG_STATE_RESET:
            FREEJTAG_TMS(0);
            FREEJTAG_CLOCK();   /* RUNIDLE */

        case FREEJTAG_STATE_RUNIDLE:
        case FREEJTAG_STATE_DRUPDATE:
        case FREEJTAG_STATE_IRUPDATE:
            FREEJTAG_TMS(1);
            FREEJTAG_CLOCK();   /* DRSELECT */
            FREEJTAG_TMS(0);
            FREEJTAG_CLOCK();   /* DRCAPTURE */
            FREEJTAG_TMS(1);
            FREEJTAG_CLOCK();   /* DREXIT1 */

        case FREEJTAG_STATE_DREXIT1:
            FREEJTAG_TMS(0);
            FREEJTAG_CLOCK();   /* DRPAUSE */
            break;

        case FREEJTAG_STATE_IREXIT1:
        case FREEJTAG_STATE_IREXIT2:
            FREEJTAG_TMS(1);
            FREEJTAG_CLOCK();   /* IRUPDATE */
            FREEJTAG_CLOCK();   /* DRSELECT */
            FREEJTAG_TMS(0);
            FREEJTAG_CLOCK();   /* DRCAPTURE */
            FREEJTAG_TMS(1);
            FREEJTAG_CLOCK();   /* DREXIT1 */
            FREEJTAG_TMS(0);
            FREEJTAG_CLOCK();   /* DRPAUSE */
            break;

        default:
            return;
        }
        break;

    case FREEJTAG_STATE_DRUPDATE:
        switch (state) {
        case FREEJTAG_STATE_DREXIT1:
        case FREEJTAG_STATE_DREXIT2:
            FREEJTAG_TMS(1);
            FREEJTAG_CLOCK();   /* DRUPDATE */
            break;

        default:
            return;
        }
        break;

    case FREEJTAG_STATE_IRSHIFT:
        switch (state) {
        case FREEJTAG_STATE_RESET:
            FREEJTAG_TMS(0);
            FREEJTAG_CLOCK();   /* RUNIDLE */

        case FREEJTAG_STATE_RUNIDLE:
        case FREEJTAG_STATE_DRUPDATE:
        case FREEJTAG_STATE_IRUPDATE:
            FREEJTAG_TMS(1);
            FREEJTAG_CLOCK();   /* DRSELECT */
            FREEJTAG_CLOCK();   /* IRSELECT */
            FREEJTAG_TMS(0);
            FREEJTAG_CLOCK();   /* IRCAPTURE */
            FREEJTAG_CLOCK();   /* IRSHIFT */
            break;

        case FREEJTAG_STATE_IREXIT2:
            FREEJTAG_TMS(0);
            FREEJTAG_CLOCK();   /* IRSHIFT */
            break;

        case FREEJTAG_STATE_DREXIT1:
        case FREEJTAG_STATE_DREXIT2:
            FREEJTAG_TMS(1);
            FREEJTAG_CLOCK();   /* DRUPDATE */
            FREEJTAG_CLOCK();   /* DRSELECT */
            FREEJTAG_CLOCK();   /* IRSELECT */
            FREEJTAG_TMS(0);
            FREEJTAG_CLOCK();   /* IRCAPTURE */
            FREEJTAG_CLOCK();   /* IRSHIFT */
            break;

        default:
            return;
        }
        break;

    case FREEJTAG_STATE_IRPAUSE:
        switch (state) {
        case FREEJTAG_STATE_RESET:
            FREEJTAG_TMS(0);
            FREEJTAG_CLOCK();   /* RUNIDLE */

        case FREEJTAG_STATE_RUNIDLE:
        case FREEJTAG_STATE_DRUPDATE:
        case FREEJTAG_STATE_IRUPDATE:
            FREEJTAG_TMS(1);
            FREEJTAG_CLOCK();   /* DRSELECT */
            FREEJTAG_CLOCK();   /* IRSELECT */
            FREEJTAG_TMS(0);
            FREEJTAG_CLOCK();   /* IRCAPTURE */
            FREEJTAG_TMS(1);
            FREEJTAG_CLOCK();   /* IREXIT1 */

        case FREEJTAG_STATE_IREXIT1:
            FREEJTAG_TMS(0);
            FREEJTAG_CLOCK();   /* IRPAUSE */
            break;

        case FREEJTAG_STATE_DREXIT1:
        case FREEJTAG_STATE_DREXIT2:
            FREEJTAG_TMS(1);
            FREEJTAG_CLOCK();   /* DRUPDATE */
            FREEJTAG_CLOCK();   /* DRSELECT */
            FREEJTAG_CLOCK();   /* IRSELECT */
            FREEJTAG_TMS(0);
            FREEJTAG_CLOCK();   /* IRCAPTURE */
            FREEJTAG_TMS(1);
            FREEJTAG_CLOCK();   /* IREXIT1 */
            FREEJTAG_TMS(0);
            FREEJTAG_CLOCK();   /* IRPAUSE */
            break;

        default:
            return;
        }
        break;

    case FREEJTAG_STATE_IRUPDATE:
        switch (state) {
        case FREEJTAG_STATE_IREXIT1:
        case FREEJTAG_STATE_IREXIT2:
            FREEJTAG_TMS(1);
            FREEJTAG_CLOCK();   /* IRUPDATE */
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

static void FreeJTAG_ShiftExit(void)
{
    FREEJTAG_TMS(1);
    switch (state) {
    case FREEJTAG_STATE_DRSHIFT:
        state = FREEJTAG_STATE_DREXIT1;
        break;

    case FREEJTAG_STATE_DRPAUSE:
        state = FREEJTAG_STATE_DREXIT2;
        break;

    case FREEJTAG_STATE_IRSHIFT:
        state = FREEJTAG_STATE_IREXIT1;
        break;

    case FREEJTAG_STATE_IRPAUSE:
        state = FREEJTAG_STATE_IREXIT2;
        break;

    default:
        break;
    }
}

static void FreeJTAG_Shift(int bits, bool exit)
{
    FREEJTAG_TDI(0);

    for (int bit = 0; bit < bits - 1; bit++) {
        FREEJTAG_CLOCK();
    }

    if (exit) {
        FreeJTAG_ShiftExit();
    }
    FREEJTAG_CLOCK();
}

static void FreeJTAG_ShiftOutBuf(int bits, bool exit)
{
    uint8_t byte = 0;
    int bit, i = 0;

    for (bit = 0; bit < bits - 1; bit++) {
        if ((bit & 7) == 0) {
            i = bit >> 3;
            byte = rxbuf[i];
        }
        FREEJTAG_TDI(byte & 1);
        byte >>= 1;
        FREEJTAG_CLOCK();
    }

    if (exit) {
        FreeJTAG_ShiftExit();
    }

    if ((bit & 7) == 0) {
        i = bit >> 3;
        byte = rxbuf[i];
    }
    FREEJTAG_TDI(byte & 1);
    byte >>= 1;
    FREEJTAG_CLOCK();
}

static void FreeJTAG_ShiftInBuf(int bits, bool exit)
{
    uint8_t byte = 0, mask = 0;
    int bit, i = 0;

    for (bit = 0; bit < bits - 1; bit++) {
        if ((bit & 7) == 0) {
            i = bit >> 3;
            byte = 0;
            mask = bits - bit >= 8 ? 0x80 : 1 << ((bits - 1) & 7);
        }
        byte >>= 1;
        if (FREEJTAG_TDO()) {
            byte |= mask;
        } else {
            byte &= ~mask;
        }
        FREEJTAG_CLOCK();
        if ((bit & 7) == 7) {
            txbuf[i] = byte;
        }
    }

    if (exit) {
        FreeJTAG_ShiftExit();
    }

    if ((bit & 7) == 0) {
        i = bit >> 3;
        byte = 0;
        mask = 0x01;
    }
    byte >>= 1;
    if (FREEJTAG_TDO()) {
        byte |= mask;
    } else {
        byte &= ~mask;
    }
    FREEJTAG_CLOCK();
    txbuf[i] = byte;
}

static void FreeJTAG_ShiftOutInBuf(int bits, bool exit)
{
    uint8_t byte = 0, mask = 0;
    int bit, i = 0;

    for (bit = 0; bit < bits - 1; bit++) {
        if ((bit & 7) == 0) {
            i = bit >> 3;
            byte = rxbuf[i];
            mask = bits - bit >= 8 ? 0x80 : 1 << ((bits - 1) & 7);
        }
        FREEJTAG_TDI(byte & 1);
        byte >>= 1;
        if (FREEJTAG_TDO()) {
            byte |= mask;
        } else {
            byte &= ~mask;
        }
        FREEJTAG_CLOCK();
        if ((bit & 7) == 7) {
            txbuf[i] = byte;
        }
    }

    if (exit) {
        FreeJTAG_ShiftExit();
    }

    if ((bit & 7) == 0) {
        i = bit >> 3;
        byte = rxbuf[i];
        mask = 0x01;
    }
    FREEJTAG_TDI(byte & 1);
    byte >>= 1;
    if (FREEJTAG_TDO()) {
        byte |= mask;
    } else {
        byte &= ~mask;
    }
    FREEJTAG_CLOCK();
    txbuf[i] = byte;
}

static void FreeJTAG_BulkWrite(void)
{
    uint8_t byte;

    for (uint8_t i = 0; i < rxlen; i++) {
        byte = rxbuf[i];
        FreeJTAG_SetState(FREEJTAG_STATE_DRSHIFT);
        for (int bit = 0; bit < 7; bit++) {
            FREEJTAG_TDI(byte & 1);
            byte >>= 1;
            FREEJTAG_CLOCK();
        }
        FreeJTAG_ShiftExit();
        FREEJTAG_TDI(byte & 1);
        FREEJTAG_CLOCK();
        FreeJTAG_SetState(FREEJTAG_STATE_RUNIDLE);
    }
}

static void FreeJTAG_BulkRead(void)
{
    uint8_t byte = 0;

    for (uint8_t i = 0; i < txlen; i++) {
        FreeJTAG_SetState(FREEJTAG_STATE_DRSHIFT);
        for (int bit = 0; bit < 7; bit++) {
            byte >>= 1;
            if (FREEJTAG_TDO()) {
                byte |= 0x80;
            } else {
                byte &= ~0x80;
            }
            FREEJTAG_CLOCK();
        }
        FreeJTAG_ShiftExit();
        byte >>= 1;
        if (FREEJTAG_TDO()) {
            byte |= 0x80;
        } else {
            byte &= ~0x80;
        }
        FREEJTAG_CLOCK();
        txbuf[i] = byte;
        FreeJTAG_SetState(FREEJTAG_STATE_RUNIDLE);
    }
}

#if !defined(MINI_FREEJTAG)
static uint32_t FreeJTAG_ShiftOutIn(int bits, uint32_t value)
{
    int mask = 1ULL << (bits - 1);

    for (int bit = 0; bit < bits - 1; bit++) {
        FREEJTAG_TDI(value & 1);
        value >>= 1;
        if (FREEJTAG_TDO()) {
            value |= mask;
        } else {
            value &= ~mask;
        }
        FREEJTAG_CLOCK();
    }

    FreeJTAG_ShiftExit();
    FREEJTAG_TDI(value & 1);
    value >>= 1;
    if (FREEJTAG_TDO()) {
        value |= mask;
    } else {
        value &= ~mask;
    }
    FREEJTAG_CLOCK();

    return value & ((1ULL << bits) - 1);
}

static int16_t FreeJTAG_AVR_ReadOCDR(void)
{
    uint8_t ir;
    uint16_t status;
    int16_t value = -1;

    FreeJTAG_SetState(FREEJTAG_STATE_IRSHIFT);
    ir = FreeJTAG_ShiftOutIn(4, IR_AVR_OCD);
    FreeJTAG_SetState(FREEJTAG_STATE_RUNIDLE);

    FreeJTAG_SetState(FREEJTAG_STATE_DRSHIFT);
    FreeJTAG_ShiftOutIn(5, AVR_OCD_CTRLSTATUS);
    FreeJTAG_SetState(FREEJTAG_STATE_RUNIDLE);

    FreeJTAG_SetState(FREEJTAG_STATE_DRSHIFT);
    status = FreeJTAG_ShiftOutIn(16, 0);
    FreeJTAG_SetState(FREEJTAG_STATE_RUNIDLE);

    if (status & 0x10) {
        FreeJTAG_SetState(FREEJTAG_STATE_DRSHIFT);
        FreeJTAG_ShiftOutIn(5, AVR_OCD_OCDR);
        FreeJTAG_SetState(FREEJTAG_STATE_RUNIDLE);

        FreeJTAG_SetState(FREEJTAG_STATE_DRSHIFT);
        value = FreeJTAG_ShiftOutIn(16, 0) >> 8;
        FreeJTAG_SetState(FREEJTAG_STATE_RUNIDLE);
    }

    FreeJTAG_SetState(FREEJTAG_STATE_IRSHIFT);
    FreeJTAG_ShiftOutIn(4, ir);
    FreeJTAG_SetState(FREEJTAG_STATE_RUNIDLE);

    return value;
}
#endif