/* SPDX-License-Identifier: MIT */
/*
 * FreeJTAG
 * Copyright (C) 2026 Jeff Kent <jeff@jkent.net>
 */

#include <avr/io.h>
#include <avr/power.h>
#include <LUFA/Drivers/USB/USB.h>

#include "cdc.h"
#include "descriptors.h"
#include "jtag.h"


static uint8_t USB_Device_LastConfigurationNumber = CONFIGURATION_NONE;

int main(void)
{
    USB_Init();
    GlobalInterruptEnable();

    while (true) {
        switch (USB_Device_ConfigurationNumber) {
            case CONFIGURATION_CDC:
            cdc_task();
            jtag_task();
            break;

        }

        USB_USBTask();
    }
}

void EVENT_USB_Device_ConfigurationChanged(void)
{
    // Unconfigure
    switch (USB_Device_LastConfigurationNumber) {
    case CONFIGURATION_CDC:
        cdc_deinit();
        jtag_deinit();
        break;
    }

    // Configure
    switch (USB_Device_ConfigurationNumber) {
    case CONFIGURATION_CDC:
        cdc_init();
        jtag_init();
        break;
    }

    USB_Device_LastConfigurationNumber = USB_Device_ConfigurationNumber;
}

void EVENT_USB_Device_ControlRequest(void)
{
    switch (USB_Device_ConfigurationNumber) {
    case CONFIGURATION_CDC:
        cdc_control_request();
        jtag_control_request();
        break;
    }
}
