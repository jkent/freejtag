/* SPDX-License-Identifier: MIT */
/*
 * FreeJTAG
 * Copyright (C) 2026 Jeff Kent <jeff@jkent.net>
 */

#include <assert.h>
#include <avr/io.h>
#include <avr/power.h>
#include <LUFA/Drivers/USB/USB.h>

#include "descriptors.h"
#include "tap.h"


uint8_t cdc_buf[32];
int8_t cdc_bytes = 0;

int main(void)
{
    clock_prescale_set(clock_div_1);
    USB_Init();
    GlobalInterruptEnable();

    while(true) {
        Endpoint_SelectEndpoint(JTAG_RX_EPADDR);
        if (Endpoint_IsOUTReceived()) {
            uint8_t tap_bytes = Endpoint_BytesInEndpoint();
            if (tap_bytes) {
                uint8_t tap_buf[8];
                Endpoint_Read_Stream_LE(tap_buf, tap_bytes, NULL);
                tap_command(tap_buf, tap_bytes);
                Endpoint_SelectEndpoint(JTAG_RX_EPADDR);
            }
            Endpoint_ClearOUT();
        }

        if (cdc_bytes == 0) {
            Endpoint_SelectEndpoint(CDC_RX_EPADDR);
            if (Endpoint_IsOUTReceived()) {
                cdc_bytes = Endpoint_BytesInEndpoint();
                if (cdc_bytes) {
                    Endpoint_Read_Stream_LE(cdc_buf, cdc_bytes, NULL);
                }
                Endpoint_ClearOUT();
            }
        }

        if (cdc_bytes > 0) {
            Endpoint_SelectEndpoint(CDC_TX_EPADDR);
            if (Endpoint_IsINReady()) {
                Endpoint_WaitUntilReady();
                Endpoint_Write_Stream_LE(cdc_buf, cdc_bytes, NULL);
                Endpoint_ClearIN();
                cdc_bytes = 0;
            }
        }

        Endpoint_SelectEndpoint(ENDPOINT_CONTROLEP);
        if (Endpoint_IsSETUPReceived()) {
            USB_Device_ProcessControlRequest();
        }
    }
}

void tap_response(const uint8_t *buf, uint8_t len, bool flush)
{
    assert(len > 0 && len <= JTAG_TXRX_EPSIZE);

    Endpoint_SelectEndpoint(JTAG_TX_EPADDR);
    Endpoint_WaitUntilReady();
    Endpoint_Write_Stream_LE(buf, len, NULL);

    if (flush) {
        if (!Endpoint_IsReadWriteAllowed()) {
            Endpoint_ClearIN();
            Endpoint_WaitUntilReady();
        }
    }
    Endpoint_ClearIN();
}

void EVENT_USB_Device_ConfigurationChanged()
{
    Endpoint_ConfigureEndpoint(JTAG_TX_EPADDR, EP_TYPE_BULK, JTAG_TXRX_EPSIZE, 1);
    Endpoint_ConfigureEndpoint(JTAG_RX_EPADDR, EP_TYPE_BULK, JTAG_TXRX_EPSIZE, 1);
    Endpoint_ConfigureEndpoint(CDC_TX_EPADDR, EP_TYPE_BULK, CDC_TXRX_EPSIZE, 2);
    Endpoint_ConfigureEndpoint(CDC_RX_EPADDR, EP_TYPE_BULK, CDC_TXRX_EPSIZE, 2);
}
