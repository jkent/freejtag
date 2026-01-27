/* SPDX-License-Identifier: MIT */
/*
 * FreeJTAG
 * Copyright (C) 2026 Jeff Kent <jeff@jkent.net>
 */

#include "descriptors.h"


const USB_Descriptor_Device_t EEMEM DeviceDescriptor =
{
    .Header = {
        .Size    = sizeof(USB_Descriptor_Device_t),
        .Type    = DTYPE_Device,
    },

    .USBSpecification       = VERSION_BCD(1,1,0),
    .Class                  = USB_CSCP_NoDeviceClass,
    .SubClass               = USB_CSCP_NoDeviceSubclass,
    .Protocol               = USB_CSCP_NoDeviceProtocol,

    .Endpoint0Size          = FIXED_CONTROL_ENDPOINT_SIZE,

    .VendorID               = VENDOR_ID,
    .ProductID              = PRODUCT_ID,
    .ReleaseNumber          = VERSION_BCD(3,0,0),

    .ManufacturerStrIndex   = STRING_ID_Manufacturer,
    .ProductStrIndex        = STRING_ID_Product,
    .SerialNumStrIndex      = USE_INTERNAL_SERIAL,

    .NumberOfConfigurations = FIXED_NUM_CONFIGURATIONS
};

const USB_Descriptor_Configuration_t EEMEM ConfigurationDescriptor =
{
    .Config = {
        .Header = {
            .Size    = sizeof(USB_Descriptor_Configuration_Header_t),
            .Type    = DTYPE_Configuration,
        },
        .TotalConfigurationSize = sizeof(USB_Descriptor_Configuration_t),
        .TotalInterfaces        = INTERFACE_ID_COUNT,
        .ConfigurationNumber    = 1,
        .ConfigurationStrIndex  = NO_DESCRIPTOR,
        .ConfigAttributes       = USB_CONFIG_ATTR_RESERVED,
        .MaxPowerConsumption    = USB_CONFIG_POWER_MA(100),
    },

    .FreeJTAG_Interface = {
        .Header = {
            .Size   = sizeof(USB_Descriptor_Interface_t),
            .Type   = DTYPE_Interface,
        },
        .InterfaceNumber    = INTERFACE_ID_FREEJTAG,
        .AlternateSetting   = 0,
        .TotalEndpoints     = 0,
        .Class              = USB_CSCP_VendorSpecificClass,
        .SubClass           = USB_CSCP_NoDeviceSubclass,
        .Protocol           = USB_CSCP_NoDeviceProtocol,
        .InterfaceStrIndex  = STRING_ID_FreeJTAGInterface,
    },
};

const USB_Descriptor_String_t EEMEM LanguageString =
        USB_STRING_DESCRIPTOR_ARRAY(LANGUAGE_ID_ENG);
const USB_Descriptor_String_t EEMEM ManufacturerString =
        USB_STRING_DESCRIPTOR(L"Jeff Kent <jeff@jkent.net>");
const USB_Descriptor_String_t EEMEM ProductString =
        USB_STRING_DESCRIPTOR(L"FreeJTAG Reference Implementation");
const USB_Descriptor_String_t EEMEM FreeJTAGInterfaceString =
        USB_STRING_DESCRIPTOR(L"FreeJTAG Interface");

uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint16_t wIndex,
                                    const void** const DescriptorAddress)
{
    const uint8_t  DescriptorType   = (wValue >> 8);
    const uint8_t  DescriptorNumber = (wValue & 0xFF);

    const void* Address = NULL;
    uint16_t    Size    = NO_DESCRIPTOR;

    switch (DescriptorType) {
    case DTYPE_Device:
        Address = &DeviceDescriptor;
        Size    = sizeof(USB_Descriptor_Device_t);
        break;
    case DTYPE_Configuration:
        Address = &ConfigurationDescriptor;
        Size = sizeof(USB_Descriptor_Configuration_t);
        break;
    case DTYPE_String:
        switch (DescriptorNumber) {
        case STRING_ID_Language:
            Address = &LanguageString;
            Size    = LanguageString.Header.Size;
            break;
        case STRING_ID_Manufacturer:
            Address = &ManufacturerString;
            Size    = ManufacturerString.Header.Size;
            break;
        case STRING_ID_Product:
            Address = &ProductString;
            Size    = ProductString.Header.Size;
            break;
        case STRING_ID_FreeJTAGInterface:
            Address = &FreeJTAGInterfaceString;
            Size    = FreeJTAGInterfaceString.Header.Size;
            break;
        }
        break;
    }

    *DescriptorAddress = Address;
    return Size;
}
