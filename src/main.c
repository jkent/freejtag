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
#include "freejtag.h"


int main(void)
{
    clock_prescale_set(clock_div_1);

    USB_Init();
    FreeJTAG_Init();

    GlobalInterruptEnable();

    while (true) {
        asm volatile ("sleep");
    }
}

void EVENT_USB_Device_ControlRequest()
{
    FreeJTAG_ControlRequest();
}
