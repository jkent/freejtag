/* SPDX-License-Identifier: MIT */
/*
 * FreeJTAG
 * Copyright (C) 2026 Jeff Kent <jeff@jkent.net>
 */

#pragma once

#include <LUFA/Drivers/USB/USB.h>


#define VENDOR_ID 0x16c0
#define PRODUCT_ID 0x27dd

typedef struct {
    USB_Descriptor_Configuration_Header_t   Config;

// FreeJTAG CDC Command Interface (defunc)
#define CDC_NOTIFICATION_EPADDR             (ENDPOINT_DIR_IN | 5)
#define CDC_NOTIFICATION_EPSIZE             (8)
    USB_Descriptor_Interface_t              CDC_CCI_Interface;
    USB_CDC_Descriptor_FunctionalHeader_t   CDC_Functional_Header;
    USB_CDC_Descriptor_FunctionalACM_t      CDC_Functional_ACM;
    USB_CDC_Descriptor_FunctionalUnion_t    CDC_Functional_Union;
    USB_Descriptor_Endpoint_t               CDC_NotificationEndpoint;

// FreeJTAG CDC Data Interface
#define CDC_TX_EPADDR                       (ENDPOINT_DIR_IN  | 3)
#define CDC_RX_EPADDR                       (ENDPOINT_DIR_OUT | 4)
#define CDC_TXRX_EPSIZE                     32
    USB_Descriptor_Interface_t              CDC_DCI_Interface;
    USB_Descriptor_Endpoint_t               CDC_DataOutEndpoint;
    USB_Descriptor_Endpoint_t               CDC_DataInEndpoint;

// FreeJTAG TAP Interface
#define JTAG_TX_EPADDR                       (ENDPOINT_DIR_IN  | 1)
#define JTAG_RX_EPADDR                       (ENDPOINT_DIR_OUT | 2)
#define JTAG_TXRX_EPSIZE                     8
    USB_Descriptor_Interface_t               JTAG_Interface;
    USB_Descriptor_Endpoint_t                JTAG_DataOutEndpoint;
    USB_Descriptor_Endpoint_t                JTAG_DataInEndpoint;

} USB_Descriptor_Configuration_t;

enum InterfaceDescriptors_t
{
    INTERFACE_ID_CDC_CCI = 0,
    INTERFACE_ID_CDC_DCI = 1,
    INTERFACE_ID_JTAG = 2,
};

enum StringDescriptors_t {
    STRING_ID_Language      = 0,
    STRING_ID_Manufacturer  = 1,
    STRING_ID_Product       = 2,
    STRING_ID_Serial        = 3,
    STRING_ID_DataInterface = 4,
    STRING_ID_JTAGInterface  = 5,
};

uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint16_t wIndex,
                                    const void** const DescriptorAddress)
                                    ATTR_WARN_UNUSED_RESULT ATTR_NON_NULL_PTR_ARG(3);
