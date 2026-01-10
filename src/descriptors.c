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

    .USBSpecification       = VERSION_BCD(2,0,0),
    .Class                  = CDC_CSCP_CDCClass,
    .SubClass               = USB_CSCP_NoDeviceSubclass,
    .Protocol               = USB_CSCP_NoDeviceProtocol,

    .Endpoint0Size          = FIXED_CONTROL_ENDPOINT_SIZE,

    .VendorID               = 0x03EB,
    .ProductID              = 0x204B,
    .ReleaseNumber          = VERSION_BCD(0,0,1),

    .ManufacturerStrIndex   = STRING_ID_Manufacturer,
    .ProductStrIndex        = STRING_ID_Product,
    .SerialNumStrIndex      = USE_INTERNAL_SERIAL,

    .NumberOfConfigurations = FIXED_NUM_CONFIGURATIONS
};

const USB_Descriptor_CDC_Configuration_t EEMEM CDC_ConfigurationDescriptor =
{
    .Config = {
        .Header = {
            .Size    = sizeof(USB_Descriptor_Configuration_Header_t),
            .Type    = DTYPE_Configuration,
        },

        .TotalConfigurationSize = sizeof(USB_Descriptor_CDC_Configuration_t),
        .TotalInterfaces        = 2,

        .ConfigurationNumber    = CONFIGURATION_CDC,

        .ConfigAttributes       = USB_CONFIG_ATTR_RESERVED,
        .MaxPowerConsumption    = USB_CONFIG_POWER_MA(100),
    },

    .CDC_Association = {
        .Header = {
            .Size   = sizeof(USB_Descriptor_Interface_Association_t),
            .Type   = DTYPE_InterfaceAssociation,
        },

        .FirstInterfaceIndex    = INTERFACE_ID_CDC_CCI,
        .TotalInterfaces        = 2,

        .Class                  = CDC_CSCP_CDCClass,
        .SubClass               = CDC_CSCP_ACMSubclass,
        .Protocol               = CDC_CSCP_NoDataProtocol,
    },

    .CDC_CCI_Interface = {
        .Header = {
            .Size   = sizeof(USB_Descriptor_Interface_t),
            .Type   = DTYPE_Interface,
        },

        .InterfaceNumber        = INTERFACE_ID_CDC_CCI,
        .TotalEndpoints         = 1,

        .Class                  = CDC_CSCP_CDCClass,
        .SubClass               = CDC_CSCP_ACMSubclass,
        .Protocol               = CDC_CSCP_NoDataProtocol,
    },

    .CDC_Functional_Header = {
        .Header = {
            .Size   = sizeof(USB_CDC_Descriptor_FunctionalHeader_t),
            .Type   = CDC_DTYPE_CSInterface,
        },
        .Subtype                = CDC_DSUBTYPE_CSInterface_Header,

        .CDCSpecification       = VERSION_BCD(2,0,0),
    },

    .CDC_Functional_ACM = {
        .Header = {
            .Size   = sizeof(USB_CDC_Descriptor_FunctionalACM_t),
            .Type   = CDC_DTYPE_CSInterface,
        },
        .Subtype                = CDC_DSUBTYPE_CSInterface_ACM,

        .Capabilities           = 0x06,
    },

    .CDC_Functional_Union = {
        .Header = {
            .Size   = sizeof(USB_CDC_Descriptor_FunctionalUnion_t),
            .Type   = CDC_DTYPE_CSInterface,
        },
        .Subtype                = CDC_DSUBTYPE_CSInterface_Union,

        .MasterInterfaceNumber  = INTERFACE_ID_CDC_CCI,
        .SlaveInterfaceNumber   = INTERFACE_ID_CDC_DCI,
    },

    .CDC_NotificationEndpoint = {
        .Header = {
            .Size   = sizeof(USB_Descriptor_Endpoint_t),
            .Type   = DTYPE_Endpoint,
        },

        .EndpointAddress        = CDC_NOTIFICATION_EPADDR,
        .Attributes             = (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC |
                                    ENDPOINT_USAGE_DATA),
        .EndpointSize           = CDC_NOTIFICATION_EPSIZE,
        .PollingIntervalMS      = 0xFF,
    },

    .CDC_DCI_Interface = {
        .Header = {
            .Size    = sizeof(USB_Descriptor_Interface_t),
            .Type    = DTYPE_Interface,
        },

        .InterfaceNumber        = INTERFACE_ID_CDC_DCI,
        .TotalEndpoints         = 2,

        .Class                  = CDC_CSCP_CDCDataClass,
        .SubClass               = CDC_CSCP_NoDataSubclass,
        .Protocol               = CDC_CSCP_NoDataProtocol,
    },

    .CDC_DataOutEndpoint = {
        .Header = {
            .Size   = sizeof(USB_Descriptor_Endpoint_t),
            .Type   = DTYPE_Endpoint,
        },

        .EndpointAddress        = CDC_RX_EPADDR,
        .Attributes             = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC |
                                    ENDPOINT_USAGE_DATA),
        .EndpointSize           = CDC_TXRX_EPSIZE,
    },

    .CDC_DataInEndpoint = {
        .Header = {
            .Size   = sizeof(USB_Descriptor_Endpoint_t),
            .Type   = DTYPE_Endpoint,
        },

        .EndpointAddress        = CDC_TX_EPADDR,
        .Attributes             = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC |
                                    ENDPOINT_USAGE_DATA),
        .EndpointSize           = CDC_TXRX_EPSIZE,
    },
};

const USB_Descriptor_String_t EEMEM LanguageString = USB_STRING_DESCRIPTOR_ARRAY(LANGUAGE_ID_ENG);
const USB_Descriptor_String_t EEMEM ManufacturerString = USB_STRING_DESCRIPTOR(L"Jeff Kent <jeff@jkent.net>");
const USB_Descriptor_String_t EEMEM ProductString = USB_STRING_DESCRIPTOR(L"CDC ACM FreeJTAG");

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
        Address = &CDC_ConfigurationDescriptor;
        Size = sizeof(USB_Descriptor_CDC_Configuration_t);
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
        }
        break;
    }

    *DescriptorAddress = Address;
    return Size;
}
