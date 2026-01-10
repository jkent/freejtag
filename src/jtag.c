/* SPDX-License-Identifier: MIT */
/*
 * FreeJTAG
 * Copyright (C) 2026 Jeff Kent <jeff@jkent.net>
 */

#include <LUFA/Drivers/USB/USB.h>

#include "descriptors.h"
#include "jtag.h"
#include "jtag_pins.h"


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

enum JTAGCmd_t {
    CMD_UNLOCK,
    CMD_VERSION,
    CMD_ATTACH,
    CMD_STATE,
    CMD_MEMSET,
    CMD_BUFWRITE,
    CMD_BUFREAD,
    CMD_SELECT,
    CMD_SHIFT,
    CMD_BULKWRITE8,
    CMD_BULKREAD8,
};

typedef struct {
    uint8_t devices_before;
    uint8_t devices_after;
    uint16_t ir_before;
    uint16_t ir_after;
} chain_info_t;

enum JTAGState_t {
    JTAG_RESET = 0,
    JTAG_RUNIDLE,
    JTAG_DRSELECT,
    JTAG_DRCAPTURE,
    JTAG_DRSHIFT,
    JTAG_DREXIT1,
    JTAG_DRPAUSE,
    JTAG_DREXIT2,
    JTAG_DRUPDATE,
    JTAG_IRSELECT,
    JTAG_IRCAPTURE,
    JTAG_IRSHIFT,
    JTAG_IREXIT1,
    JTAG_IRPAUSE,
    JTAG_IREXIT2,
    JTAG_IRUPDATE,
    JTAG_DEFAULT,
};

static bool locked;
static enum JTAGState_t state;
static chain_info_t chain;
static uint8_t buf[256];

static void attach(void);
static void detach(void);
static bool stable_state(void);
static void change_state(enum JTAGState_t exit_state);
static void shift_data(uint16_t data_bits, bool ir, enum JTAGState_t state);
static void bulkread8(uint16_t ir_bits, uint32_t ir, uint16_t bytes);
static void bulkwrite8(uint16_t ir_bits, uint32_t ir, uint16_t bytes);

void jtag_deinit(void)
{
    detach();
}

void jtag_init(void)
{
    locked = true;
}

void jtag_task(void)
{
}

void jtag_control_request(void)
{
    if (!Endpoint_IsSETUPReceived()) {
        return;
    }

    if (USB_ControlRequest.bRequest == CMD_UNLOCK) {
        if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_VENDOR | REQREC_DEVICE)) {
            Endpoint_ClearSETUP();
            char text[8];
            uint16_t len = MIN(sizeof(text), USB_ControlRequest.wLength);
            memset(text, 0, sizeof(text));
            Endpoint_Read_Control_Stream_LE(text, len);
            locked = (strncmp(text, "FreeJTAG", sizeof(text)) != 0);
            if (locked) {
                detach();
            }
            Endpoint_ClearStatusStage();
            return;
        }
    }

    if (locked) {
        return;
    }

    switch (USB_ControlRequest.bRequest) {
    case CMD_VERSION:
        if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQREC_DEVICE)) {
            Endpoint_ClearSETUP();
            const uint16_t version = VERSION_BCD(1,0,0);
            Endpoint_Write_Control_Stream_LE(&version, sizeof(version));
            Endpoint_ClearStatusStage();
        }
        break;

    case CMD_ATTACH:
        if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_VENDOR | REQREC_DEVICE)) {
            Endpoint_ClearSETUP();
            if (!!USB_ControlRequest.wValue) {
                attach();
            } else {
                detach();
            }
            Endpoint_ClearStatusStage();
        }
        break;

    case CMD_STATE:
        if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_VENDOR | REQREC_DEVICE)) {
            Endpoint_ClearSETUP();
            change_state(USB_ControlRequest.wValue & 0xff);
            if (stable_state()) {
                while (USB_ControlRequest.wIndex--) {
                    JTAG_CLOCK();
                }
            }
            Endpoint_ClearStatusStage();
        }
        break;

    case CMD_MEMSET:
        if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_VENDOR | REQREC_DEVICE)) {
            Endpoint_ClearSETUP();
            uint8_t byte = USB_ControlRequest.wValue & 0xff;
            memset(buf, byte, sizeof(buf));
            Endpoint_ClearStatusStage();
        }
        break;

    case CMD_BUFWRITE:
        if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_VENDOR | REQREC_DEVICE)) {
            Endpoint_ClearSETUP();
            uint16_t len = MIN(sizeof(buf), USB_ControlRequest.wLength);
            Endpoint_Read_Control_Stream_LE(buf, len);
            Endpoint_ClearStatusStage();
        }
        break;

    case CMD_BUFREAD:
        if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQREC_DEVICE)) {
            Endpoint_ClearSETUP();
            uint16_t len = MIN(sizeof(buf), USB_ControlRequest.wLength);
            Endpoint_Write_Control_Stream_LE(buf, len);
            Endpoint_ClearStatusStage();
        }
        break;

    case CMD_SELECT:
        if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_VENDOR | REQREC_DEVICE)) {
            Endpoint_ClearSETUP();
            Endpoint_Read_Control_Stream_LE(&chain, sizeof(chain));
            Endpoint_ClearStatusStage();
        }
        break;

    case CMD_SHIFT:
        if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_VENDOR | REQREC_DEVICE)) {
            Endpoint_ClearSETUP();
            uint16_t data_bits = USB_ControlRequest.wValue;
            bool ir = !!(USB_ControlRequest.wIndex & 0x100);
            uint8_t end_state = USB_ControlRequest.wIndex & 0xff;
            shift_data(data_bits, ir, end_state);
            Endpoint_ClearStatusStage();
        }
        break;

    case CMD_BULKWRITE8:
        if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_VENDOR | REQREC_DEVICE)) {
            Endpoint_ClearSETUP();
            uint16_t len = MIN(sizeof(buf), USB_ControlRequest.wLength);
            uint16_t bits = MIN(16, USB_ControlRequest.wValue);
            uint32_t ir = USB_ControlRequest.wIndex;
            Endpoint_Read_Control_Stream_LE(buf, len);
            bulkwrite8(bits, ir, len);
            Endpoint_ClearStatusStage();
        }
        break;

    case CMD_BULKREAD8:
        if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQREC_DEVICE)) {
            Endpoint_ClearSETUP();
            uint16_t len = MIN(sizeof(buf), USB_ControlRequest.wLength);
            uint16_t bits = MIN(16, USB_ControlRequest.wValue);
            uint32_t ir = USB_ControlRequest.wIndex;
            bulkread8(bits, ir, len);
            Endpoint_Write_Control_Stream_LE(buf, len);
            Endpoint_ClearStatusStage();
        }
        break;

    default:
        return;
    }
}

static void attach(void)
{
    JTAG_TCK_PORT &= ~JTAG_TCK_BIT;
    JTAG_TDO_PORT |= JTAG_TDO_BIT;
    JTAG_TMS_PORT |= JTAG_TMS_BIT;
    JTAG_TDI_PORT &= ~JTAG_TDI_BIT;

    JTAG_TCK_DDR |= JTAG_TCK_BIT;
    JTAG_TDO_DDR &= ~JTAG_TDO_BIT;
    JTAG_TMS_DDR |= JTAG_TMS_BIT;
    JTAG_TDI_DDR |= JTAG_TDI_BIT;

    change_state(JTAG_RESET);
}

static void detach(void)
{
    JTAG_TCK_DDR &= ~JTAG_TCK_BIT;
    JTAG_TDO_DDR &= ~JTAG_TDO_BIT;
    JTAG_TMS_DDR &= ~JTAG_TMS_BIT;
    JTAG_TDI_DDR &= ~JTAG_TDI_BIT;

    JTAG_TCK_PORT &= ~JTAG_TCK_BIT;
    JTAG_TDO_PORT &= ~JTAG_TDO_BIT;
    JTAG_TMS_PORT &= ~JTAG_TMS_BIT;
    JTAG_TDI_PORT &= ~JTAG_TDI_BIT;
}

static bool stable_state(void)
{
    switch (state) {
    case JTAG_RESET:
    case JTAG_RUNIDLE:
    case JTAG_DRSHIFT:
    case JTAG_DRPAUSE:
    case JTAG_IRSHIFT:
    case JTAG_IRPAUSE:
        return true;
    default:
        return false;
    }
}

static void change_state(enum JTAGState_t end_state)
{
    switch (end_state) {
    case JTAG_RESET:
        JTAG_TMS(1);
        JTAG_CLOCK(); /* ??? */
        JTAG_CLOCK(); /* ??? */
        JTAG_CLOCK(); /* ??? */
        JTAG_CLOCK(); /* ??? */
        JTAG_CLOCK(); /* RESET */
        break;

    case JTAG_RUNIDLE:
        switch (state) {
        case JTAG_RESET:
            JTAG_TMS(0);
            JTAG_CLOCK(); /* RUNIDLE */
            break;
        case JTAG_DRCAPTURE:
        case JTAG_DRPAUSE:
        case JTAG_IRCAPTURE:
        case JTAG_IRPAUSE:
            JTAG_TMS(1);
            JTAG_CLOCK(); /* DREXIT2/IREXIT2 */
        case JTAG_DREXIT1:
        case JTAG_DREXIT2:
        case JTAG_IREXIT1:
        case JTAG_IREXIT2:
            JTAG_TMS(1);
            JTAG_CLOCK(); /* DRUPDATE/IRUPDATE */
        case JTAG_DRUPDATE:
        case JTAG_IRUPDATE:
            JTAG_TMS(0);
            JTAG_CLOCK(); /* RUNIDLE */
        case JTAG_RUNIDLE:
            break;
        default:
            return;
        }
        break;

    case JTAG_DRSHIFT:
        switch (state) {
        case JTAG_RESET:
            JTAG_TMS(0);
            JTAG_CLOCK(); /* RUNIDLE */
        case JTAG_RUNIDLE:
        case JTAG_DRUPDATE:
        case JTAG_IRUPDATE:
            JTAG_TMS(1);
            JTAG_CLOCK(); /* DRSELECT */
        case JTAG_DRSELECT:
            JTAG_TMS(0);
            JTAG_CLOCK(); /* DRCAPTURE */
        case JTAG_DRCAPTURE:
            JTAG_TMS(0);
            JTAG_CLOCK(); /* DRSHIFT */
        case JTAG_DRSHIFT:
            break;
        case JTAG_DRPAUSE:
            JTAG_TMS(1);
            JTAG_CLOCK(); /* DREXIT2 */
        case JTAG_DREXIT2:
            JTAG_TMS(0);
            JTAG_CLOCK(); /* DRSHIFT */
            break;
        case JTAG_IRSELECT:
            JTAG_TMS(0);
            JTAG_CLOCK(); /* IRCAPTURE */
        case JTAG_IRCAPTURE:
        case JTAG_IRPAUSE:
            JTAG_TMS(1);
            JTAG_CLOCK(); /* IREXIT1/IREXIT2 */
        case JTAG_IREXIT1:
        case JTAG_IREXIT2:
            JTAG_TMS(1);
            JTAG_CLOCK(); /* IRUPDATE */
            JTAG_CLOCK(); /* DRSELECT */
            JTAG_TMS(0);
            JTAG_CLOCK(); /* DRCAPTURE */
            JTAG_CLOCK(); /* DRSHFIT */
            break;
        default:
            return;
        }
        break;

    case JTAG_DRPAUSE:
        switch (state) {
        case JTAG_RESET:
            JTAG_TMS(0);
            JTAG_CLOCK(); /* RUNIDLE */
        case JTAG_RUNIDLE:
        case JTAG_DRUPDATE:
        case JTAG_IRUPDATE:
            JTAG_TMS(1);
            JTAG_CLOCK(); /* DRSELECT */
        case JTAG_DRSELECT:
            JTAG_TMS(0);
            JTAG_CLOCK(); /* DRCAPTURE */
        case JTAG_DRCAPTURE:
            JTAG_TMS(1);
            JTAG_CLOCK(); /* DREXIT1 */
        case JTAG_DREXIT1:
            JTAG_TMS(0);
            JTAG_CLOCK(); /* DRPAUSE */
        case JTAG_DRPAUSE:
            break;
        case JTAG_IRSELECT:
            JTAG_TMS(0);
            JTAG_CLOCK(); /* IRCAPTURE */
        case JTAG_IRCAPTURE:
        case JTAG_IRPAUSE:
            JTAG_TMS(1);
            JTAG_CLOCK(); /* IREXIT1/IREXIT2 */
        case JTAG_IREXIT1:
        case JTAG_IREXIT2:
            JTAG_TMS(1);
            JTAG_CLOCK(); /* IRUPDATE */
            JTAG_CLOCK(); /* DRSELECT */
            JTAG_TMS(0);
            JTAG_CLOCK(); /* DRCAPTURE */
            JTAG_TMS(1);
            JTAG_CLOCK(); /* DREXIT1 */
            JTAG_TMS(0);
            JTAG_CLOCK(); /* DRPAUSE */
            break;
        default:
            return;
        }
        break;

    case JTAG_DRUPDATE:
        switch (state) {
        case JTAG_DRPAUSE:
            JTAG_TMS(1);
            JTAG_CLOCK(); /* DREXIT2 */
        case JTAG_DREXIT1:
        case JTAG_DREXIT2:
            JTAG_TMS(1);
            JTAG_CLOCK(); /* DRUPDATE */
        case JTAG_DRUPDATE:
            break;
        default:
            return;
        }
        break;

    case JTAG_IRSHIFT:
        switch (state) {
        case JTAG_RESET:
            JTAG_TMS(0);
            JTAG_CLOCK(); /* RUNIDLE */
        case JTAG_RUNIDLE:
        case JTAG_DRUPDATE:
        case JTAG_IRUPDATE:
            JTAG_TMS(1);
            JTAG_CLOCK(); /* DRSELECT */
        case JTAG_DRSELECT:
            JTAG_TMS(1);
            JTAG_CLOCK(); /* IRSELECT */
        case JTAG_IRSELECT:
            JTAG_TMS(0);
            JTAG_CLOCK(); /* IRCAPTURE */
        case JTAG_IRCAPTURE:
            JTAG_TMS(0);
            JTAG_CLOCK(); /* IRSHIFT */
        case JTAG_IRSHIFT:
            break;
        case JTAG_IRPAUSE:
            JTAG_TMS(1);
            JTAG_CLOCK(); /* IREXIT2 */
        case JTAG_IREXIT2:
            JTAG_TMS(0);
            JTAG_CLOCK(); /* IRSHIFT */
            break;
        case JTAG_DRCAPTURE:
        case JTAG_DRPAUSE:
            JTAG_TMS(1);
            JTAG_CLOCK(); /* DREXIT1/DREXIT2 */
        case JTAG_DREXIT1:
        case JTAG_DREXIT2:
            JTAG_TMS(1);
            JTAG_CLOCK(); /* DRUPDATE */
            JTAG_CLOCK(); /* DRSELECT */
            JTAG_CLOCK(); /* IRSELECT */
            JTAG_TMS(0);
            JTAG_CLOCK(); /* IRCAPTURE */
            JTAG_CLOCK(); /* IRSHIFT */
            break;
        default:
            return;
        }
        break;

    case JTAG_IRPAUSE:
        switch (state) {
        case JTAG_RESET:
            JTAG_TMS(0);
            JTAG_CLOCK(); /* RUNIDLE */
        case JTAG_RUNIDLE:
        case JTAG_DRUPDATE:
        case JTAG_IRUPDATE:
            JTAG_TMS(1);
            JTAG_CLOCK(); /* DRSELECT */
        case JTAG_DRSELECT:
            JTAG_TMS(1);
            JTAG_CLOCK(); /* IRSELECT */
        case JTAG_IRSELECT:
            JTAG_TMS(0);
            JTAG_CLOCK(); /* IRCAPTURE */
        case JTAG_IRCAPTURE:
            JTAG_TMS(1);
            JTAG_CLOCK(); /* IREXIT1 */
        case JTAG_IREXIT1:
            JTAG_TMS(0);
            JTAG_CLOCK(); /* IRPAUSE */
        case JTAG_IRPAUSE:
            break;
        case JTAG_DRCAPTURE:
        case JTAG_DRPAUSE:
            JTAG_TMS(1);
            JTAG_CLOCK(); /* DREXIT1/DREXIT2 */
        case JTAG_DREXIT1:
        case JTAG_DREXIT2:
            JTAG_TMS(1);
            JTAG_CLOCK(); /* DRUPDATE */
            JTAG_CLOCK(); /* DRSELECT */
            JTAG_CLOCK(); /* IRSELECT */
            JTAG_TMS(0);
            JTAG_CLOCK(); /* IRCAPTURE */
            JTAG_TMS(1);
            JTAG_CLOCK(); /* IREXIT1 */
            JTAG_TMS(0);
            JTAG_CLOCK(); /* IRPAUSE */
            break;
        default:
            return;
        }
        break;

    case JTAG_IRUPDATE:
        switch (state) {
        case JTAG_IRPAUSE:
            JTAG_TMS(1);
            JTAG_CLOCK(); /* IREXIT2 */
        case JTAG_IREXIT1:
        case JTAG_IREXIT2:
            JTAG_TMS(1);
            JTAG_CLOCK(); /* IRUPDATE */
        case JTAG_IRUPDATE:
            break;
        default:
            return;
        }
        break;

    default:
        return;
    }

    state = end_state;
}

static void shift_data(uint16_t data_bits, bool ir, enum JTAGState_t end_state)
{
    uint16_t head_bits, tail_bits, total_bits;

    if (ir) {
        head_bits = chain.ir_before > 16384 ? 16384 : chain.ir_before;
        tail_bits = chain.ir_after > 16384 ? 16384 : chain.ir_after;
    } else {
        head_bits = chain.devices_before;
        tail_bits = chain.devices_after;
    }
    data_bits = data_bits > (sizeof(buf) * 8) ? (sizeof(buf) * 8) : data_bits;
    total_bits = head_bits + data_bits + tail_bits;

    change_state(ir ? JTAG_IRSHIFT : JTAG_DRSHIFT);

    uint8_t *p = buf;
    for (int i = 0; i < total_bits; i++) {
        if (i >= head_bits && i < head_bits + data_bits) {
            uint8_t bit = (i - head_bits) & 7;
            uint8_t mask = 1 << bit;
            JTAG_TDI(*p & mask);
            if (JTAG_TDO()) {
                *p |= mask;
            } else {
                *p &= ~mask;
            }
            if (bit == 7) {
                p++;
            }
        } else {
            JTAG_TDI(1);
        }

        if (i == total_bits - 1) {
            JTAG_TMS(1);
            state = ir ? JTAG_IREXIT1 : JTAG_DREXIT1;
        }

        JTAG_CLOCK();
    }

    change_state(ir ? JTAG_IRUPDATE : JTAG_RUNIDLE);
}

static void write_ir(uint16_t data_bits, uint32_t ir)
{
    uint16_t head_bits, tail_bits, total_bits;

    head_bits = chain.ir_before > 16384 ? 16384 : chain.ir_before;
    tail_bits = chain.ir_after > 16384 ? 16384 : chain.ir_after;
    data_bits = data_bits > sizeof(ir) ? sizeof(ir) : data_bits;
    total_bits = head_bits + data_bits + tail_bits;

    change_state(JTAG_IRSHIFT);

    for (int i = 0; i < total_bits; i++) {
        if (i >= head_bits && i < head_bits + data_bits) {
            uint8_t bit = (i - head_bits);
            uint32_t mask = 1 << bit;
            JTAG_TDI(ir & mask);
        } else {
            JTAG_TDI(1);
        }

        if (i == total_bits - 1) {
            JTAG_TMS(1);
            state = JTAG_IREXIT1;
        }

        JTAG_CLOCK();
    }

    change_state(JTAG_IRUPDATE);
}

static void bulkwrite8(uint16_t ir_bits, uint32_t ir, uint16_t bytes)
{
    uint16_t head_bits, data_bits, tail_bits, total_bits;

    write_ir(ir_bits, ir);

    head_bits = chain.devices_before;
    tail_bits = chain.devices_after;
    data_bits = 8;
    total_bits = head_bits + data_bits + tail_bits;

    bytes = bytes > sizeof(buf) ? sizeof(buf) : bytes;

    uint8_t *p = buf;
    for (uint16_t byte = 0; byte < bytes; byte++) {
        change_state(JTAG_DRSHIFT);
        for (int i = 0; i < total_bits; i++) {
            if (i >= head_bits && i < head_bits + data_bits) {
                uint8_t bit = (i - head_bits) & 7;
                uint8_t mask = 1 << bit;
                JTAG_TDI(*p & mask);
            }
            if (i == total_bits - 1) {
                JTAG_TMS(1);
                state = JTAG_DREXIT1;
            }
            JTAG_CLOCK();
        }
        p++;
        change_state(JTAG_RUNIDLE);
    }
}

static void bulkread8(uint16_t ir_bits, uint32_t ir, uint16_t bytes)
{
    uint16_t head_bits, data_bits, tail_bits, total_bits;

    write_ir(ir_bits, ir);

    head_bits = chain.devices_before;
    tail_bits = chain.devices_after;
    data_bits = 8;
    total_bits = head_bits + data_bits + tail_bits;

    bytes = bytes > sizeof(buf) ? sizeof(buf) : bytes;

    uint8_t *p = buf;
    for (uint16_t byte = 0; byte < bytes; byte++) {
        change_state(JTAG_DRSHIFT);
        for (int i = 0; i < total_bits; i++) {
            if (i >= head_bits && i < head_bits + data_bits) {
                uint8_t bit = (i - head_bits) & 7;
                uint8_t mask = 1 << bit;
                if (JTAG_TDO()) {
                    *p |= mask;
                } else {
                    *p &= ~mask;
                }
            }
            if (i == total_bits - 1) {
                JTAG_TMS(1);
                state = JTAG_DREXIT1;
            }
            JTAG_CLOCK();
        }
        p++;
        change_state(JTAG_RUNIDLE);
    }
}
