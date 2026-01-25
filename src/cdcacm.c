/* SPDX-License-Identifier: MIT */
/*
 * FreeJTAG
 * Copyright (C) 2026 Jeff Kent <jeff@jkent.net>
 */

#include <avr/io.h>
#include <avr/power.h>
#include <util/atomic.h>
#include <LUFA/Drivers/USB/USB.h>
#include <stdbool.h>
#include <stdint.h>

#include "cdcacm.h"
#include "descriptors.h"


#if !defined(CONTROL_ONLY_DEVICE)
#define CDCACM_BAUDRATE    500000

static bool outstanding = false;

void CDCACM_Init(void)
{
    /* enable USART, 8-N-1 with RX interrupt enabled */
    UCSR1B = 0;
    UCSR1A = 0;
    UCSR1C = 0;
    UBRR1 = (F_CPU / (clock_prescale_get() + 1) / (8 * CDCACM_BAUDRATE)) - 1;
    UCSR1C = _BV(UCSZ11) | _BV(UCSZ10);
    UCSR1A = _BV(U2X1);
    UCSR1B = _BV(RXCIE1) | _BV(RXEN1) | _BV(TXEN1);

    /* Configure timer1 for 1 MHz and 1 KHz tick */
    TCCR1A = 0;
    TCCR1C = 0;
    OCR1A = 999;
    TCNT1 = 0;
    TCCR1B = _BV(WGM12) | _BV(CS11);
    TIMSK1 = _BV(OCIE1A);
}

void CDCACM_Task(void)
{
    uint8_t chunk, byte;

    Endpoint_SelectEndpoint(DCI_RX_EPADDR);
    if (Endpoint_IsOUTReceived()) {
        loop_until_bit_is_set(UEINTX, FIFOCON);
        chunk = Endpoint_BytesInEndpoint();
        for (uint8_t i = 0; i < chunk; i++) {
            byte = Endpoint_Read_8();
            loop_until_bit_is_set(UCSR1A, UDRE1);
            UDR1 = byte;
        }
        Endpoint_ClearOUT();
    }
}

void CDCACM_Configure(void)
{
    Endpoint_ConfigureEndpoint(DCI_TX_EPADDR, EP_TYPE_BULK, DCI_TXRX_EPSIZE, 2);
    Endpoint_ConfigureEndpoint(DCI_RX_EPADDR, EP_TYPE_BULK, DCI_TXRX_EPSIZE, 2);
}

ISR(USART1_RX_vect)
{
    uint8_t ucsra, udr, ep;

    ucsra = UCSR1A;
    (void) UCSR1B;
    udr = UDR1;

    if (ucsra & (_BV(FE1) | _BV(DOR1) | _BV(UPE1))) {
        return;
    }

    ep = Endpoint_GetCurrentEndpoint();
    Endpoint_SelectEndpoint(DCI_TX_EPADDR);
    loop_until_bit_is_set(UEINTX, FIFOCON);
    Endpoint_Write_8(udr);
    outstanding = true;
    if (Endpoint_BytesInEndpoint() == DCI_TXRX_EPSIZE) {
        Endpoint_ClearIN();
    }
    Endpoint_SelectEndpoint(ep);
}

ISR(TIMER1_COMPA_vect)
{
    uint8_t ep;

    if (outstanding) {
        outstanding = false;

        ep = Endpoint_GetCurrentEndpoint();
        Endpoint_SelectEndpoint(DCI_TX_EPADDR);
        loop_until_bit_is_set(UEINTX, FIFOCON);
        Endpoint_ClearIN();
        Endpoint_SelectEndpoint(ep);
    }
}
#endif