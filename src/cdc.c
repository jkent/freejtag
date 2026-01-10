/* SPDX-License-Identifier: MIT */
/*
 * FreeJTAG
 * Copyright (C) 2026 Jeff Kent <jeff@jkent.net>
 */

#include <avr/io.h>
#include <avr/power.h>
#include <LUFA/Drivers/Peripheral/Serial.h>
#include <LUFA/Drivers/USB/USB.h>

#include "cdc.h"
#include "descriptors.h"


#define CTS_PORT PORTD
#define CTS_PIN PIND
#define CTS_DDR DDRD
#define CTS_BIT _BV(PD4)
#define CTS (!(CTS_PIN & CTS_BIT))

static USB_ClassInfo_CDC_Device_t CDCInterfaceInfo = {
    .Config = {
        .ControlInterfaceNumber     = INTERFACE_ID_CDC_CCI,
        .DataINEndpoint = {
            .Address                = CDC_TX_EPADDR,
            .Size                   = CDC_TXRX_EPSIZE,
            .Banks                  = 2,
        },
        .DataOUTEndpoint = {
            .Address                = CDC_RX_EPADDR,
            .Size                   = CDC_TXRX_EPSIZE,
            .Banks                  = 2,
        },
        .NotificationEndpoint = {
            .Address                = CDC_NOTIFICATION_EPADDR,
            .Size                   = CDC_NOTIFICATION_EPSIZE,
            .Banks                  = 1,
        },
    },
};

void cdc_deinit(void)
{
   /* UART Deinit */
    UCSR1B &= ~_BV(RXCIE1);
}

void cdc_init(void)
{
    /* UART Init */
    UBRR1 = SERIAL_2X_UBBRVAL(1000000);
    UCSR1C = _BV(UCSZ11) | _BV(UCSZ10);
    UCSR1A = _BV(U2X1);
    UCSR1B = _BV(RXCIE1) | _BV(RXEN1) | _BV(TXEN1);

    /* CTS init */
    CTS_DDR &= ~CTS_BIT;
    CTS_PORT |= CTS_BIT;

    /* CDC Init */
    CDC_Device_ConfigureEndpoints(&CDCInterfaceInfo);
}

void cdc_task(void)
{
    static int16_t recv = -1;

    if (recv < 0) {
again:
        recv = CDC_Device_ReceiveByte(&CDCInterfaceInfo);
    }
    if (recv >= 0 && CTS && (UCSR1A & _BV(UDRE1))) {
        UDR1 = recv;
        goto again;
    }

    CDC_Device_USBTask(&CDCInterfaceInfo);
}

void cdc_control_request(void)
{
    CDC_Device_ProcessControlRequest(&CDCInterfaceInfo);
}

ISR(USART1_RX_vect)
{
    uint8_t ep = Endpoint_GetCurrentEndpoint();
    CDC_Device_SendByte(&CDCInterfaceInfo, UDR1);
    Endpoint_SelectEndpoint(ep);
}
