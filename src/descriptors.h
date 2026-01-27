/* SPDX-License-Identifier: MIT */
/*
 * FreeJTAG
 * Copyright (C) 2026 Jeff Kent <jeff@jkent.net>
 */

#pragma once

#include <LUFA/Drivers/USB/USB.h>


#define VENDOR_ID   0x0403
#define PRODUCT_ID  0x7ba8

typedef struct {
    USB_Descriptor_Configuration_Header_t   Config;
    USB_Descriptor_Interface_t              FreeJTAG_Interface;
} USB_Descriptor_Configuration_t;

enum InterfaceDescriptors_t
{
    INTERFACE_ID_FREEJTAG,
    INTERFACE_ID_COUNT,
};

enum StringDescriptors_t {
    STRING_ID_Language,
    STRING_ID_Manufacturer,
    STRING_ID_Product,
    STRING_ID_Serial,
    STRING_ID_FreeJTAGInterface,
};

uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint16_t wIndex,
                                    const void** const DescriptorAddress)
                                    ATTR_WARN_UNUSED_RESULT ATTR_NON_NULL_PTR_ARG(3);
