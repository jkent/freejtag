/* SPDX-License-Identifier: MIT */
/*
 * FreeJTAG
 * Copyright (C) 2026 Jeff Kent <jeff@jkent.net>
 */

#pragma once

#include <LUFA/Drivers/USB/USB.h>


#define VENDOR_ID   0x16c0
#define PRODUCT_ID  0x27dd

typedef struct {
    USB_Descriptor_Configuration_Header_t   Config;

// CDC ACM Interfaces
#define CCI_EPADDR                          (ENDPOINT_DIR_IN  | 5)
#define CCI_EPSIZE                          8
    USB_Descriptor_Interface_t              CCI_Interface;
    USB_CDC_Descriptor_FunctionalHeader_t   CCI_Functional_Header;
    USB_CDC_Descriptor_FunctionalACM_t      CCI_Functional_ACM;
    USB_CDC_Descriptor_FunctionalUnion_t    CCI_Functional_Union;
    USB_Descriptor_Endpoint_t               CCI_DataInEndpoint;

#define DCI_TX_EPADDR                       (ENDPOINT_DIR_IN  | 3)
#define DCI_RX_EPADDR                       (ENDPOINT_DIR_OUT | 4)
#define DCI_TXRX_EPSIZE                     8
    USB_Descriptor_Interface_t              DCI_Interface;
    USB_Descriptor_Endpoint_t               DCI_DataOutEndpoint;
    USB_Descriptor_Endpoint_t               DCI_DataInEndpoint;

// FreeJTAG TAP Interface
    USB_Descriptor_Interface_t              FreeJTAG_Interface;
} USB_Descriptor_Configuration_t;

enum InterfaceDescriptors_t
{
    INTERFACE_ID_CCI,
    INTERFACE_ID_DCI,
    INTERFACE_ID_FREEJTAG,
    INTERFACE_COUNT,
};

enum StringDescriptors_t {
    STRING_ID_Language,
    STRING_ID_Manufacturer,
    STRING_ID_Product,
    STRING_ID_Serial,
    STRING_ID_DCIInterface,
    STRING_ID_FreeJTAGInterface,
};

uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint16_t wIndex,
                                    const void** const DescriptorAddress)
                                    ATTR_WARN_UNUSED_RESULT ATTR_NON_NULL_PTR_ARG(3);
