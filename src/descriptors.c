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
    .Class                  = CDC_CSCP_CDCClass,
    .SubClass               = CDC_CSCP_NoSpecificSubclass,
    .Protocol               = CDC_CSCP_NoSpecificProtocol,

    .Endpoint0Size          = FIXED_CONTROL_ENDPOINT_SIZE,

    .VendorID               = VENDOR_ID,
    .ProductID              = PRODUCT_ID,
    .ReleaseNumber          = VERSION_BCD(0,0,1),

    .ManufacturerStrIndex   = STRING_ID_Manufacturer,
    .ProductStrIndex        = STRING_ID_Product,
    .SerialNumStrIndex      = STRING_ID_Serial,

    .NumberOfConfigurations = FIXED_NUM_CONFIGURATIONS
};

const USB_Descriptor_Configuration_t EEMEM CDC_ConfigurationDescriptor =
{
    .Config = {
        .Header = {
            .Size    = sizeof(USB_Descriptor_Configuration_Header_t),
            .Type    = DTYPE_Configuration,
        },
        .TotalConfigurationSize = sizeof(USB_Descriptor_Configuration_t),
        .TotalInterfaces        = 3,
        .ConfigurationNumber    = 1,
        .ConfigurationStrIndex  = NO_DESCRIPTOR,
        .ConfigAttributes       = USB_CONFIG_ATTR_RESERVED,
        .MaxPowerConsumption    = USB_CONFIG_POWER_MA(100),
    },

    .CDC_CCI_Interface = {
        .Header = {
            .Size    = sizeof(USB_Descriptor_Interface_t),
            .Type    = DTYPE_Interface,
        },
        .InterfaceNumber        = INTERFACE_ID_CDC_CCI,
        .AlternateSetting       = 0,
        .TotalEndpoints         = 1,
        .Class                  = CDC_CSCP_CDCClass,
        .SubClass               = CDC_CSCP_ACMSubclass,
        .Protocol               = CDC_CSCP_NoDataProtocol,
        .InterfaceStrIndex      = NO_DESCRIPTOR,
    },

    .CDC_Functional_Header = {
        .Header = {
            .Size    = sizeof(USB_CDC_Descriptor_FunctionalHeader_t),
            .Type    = CDC_DTYPE_CSInterface,
        },
        .Subtype = CDC_DSUBTYPE_CSInterface_Header,
        .CDCSpecification       = VERSION_BCD(1,1,0),
    },

    .CDC_Functional_ACM = {
        .Header = {
            .Size    = sizeof(USB_CDC_Descriptor_FunctionalACM_t),
            .Type    = CDC_DTYPE_CSInterface,
        },
        .Subtype                = CDC_DSUBTYPE_CSInterface_ACM,
        .Capabilities           = 0,
    },

    .CDC_Functional_Union = {
        .Header = {
            .Size    = sizeof(USB_CDC_Descriptor_FunctionalUnion_t),
            .Type    = CDC_DTYPE_CSInterface,
        },
        .Subtype = CDC_DSUBTYPE_CSInterface_Union,
        .MasterInterfaceNumber  = INTERFACE_ID_CDC_CCI,
        .SlaveInterfaceNumber   = INTERFACE_ID_CDC_DCI,
    },

    .CDC_NotificationEndpoint = {
        .Header = {
            .Size    = sizeof(USB_Descriptor_Endpoint_t),
            .Type    = DTYPE_Endpoint,
        },
        .EndpointAddress        = CDC_NOTIFICATION_EPADDR,
        .Attributes             = (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        .EndpointSize           = CDC_NOTIFICATION_EPSIZE,
        .PollingIntervalMS      = 0xff,
    },

    .CDC_DCI_Interface = {
        .Header = {
            .Size   = sizeof(USB_Descriptor_Interface_t),
            .Type   = DTYPE_Interface,
        },
        .InterfaceNumber    = INTERFACE_ID_CDC_DCI,
        .AlternateSetting   = 0,
        .TotalEndpoints     = 2,
        .Class              = CDC_CSCP_CDCDataClass,
        .SubClass           = CDC_CSCP_NoDataSubclass,
        .Protocol           = CDC_CSCP_NoDataProtocol,
        .InterfaceStrIndex  = STRING_ID_DataInterface,
    },

    .CDC_DataOutEndpoint = {
        .Header = {
            .Size   = sizeof(USB_Descriptor_Endpoint_t),
            .Type   = DTYPE_Endpoint,
        },
        .EndpointAddress    = CDC_RX_EPADDR,
        .Attributes         = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        .EndpointSize       = CDC_TXRX_EPSIZE,
    },

    .CDC_DataInEndpoint = {
        .Header = {
            .Size   = sizeof(USB_Descriptor_Endpoint_t),
            .Type   = DTYPE_Endpoint,
        },
        .EndpointAddress    = CDC_TX_EPADDR,
        .Attributes         = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        .EndpointSize       = CDC_TXRX_EPSIZE,
    },

    .JTAG_Interface = {
        .Header = {
            .Size   = sizeof(USB_Descriptor_Interface_t),
            .Type   = DTYPE_Interface,
        },
        .InterfaceNumber    = INTERFACE_ID_JTAG,
        .AlternateSetting   = 0,
        .TotalEndpoints     = 2,
        .Class              = USB_CSCP_VendorSpecificClass,
        .SubClass           = USB_CSCP_NoDeviceSubclass,
        .Protocol           = USB_CSCP_NoDeviceProtocol,
        .InterfaceStrIndex  = STRING_ID_JTAGInterface,
    },

    .JTAG_DataOutEndpoint = {
        .Header = {
            .Size   = sizeof(USB_Descriptor_Endpoint_t),
            .Type   = DTYPE_Endpoint,
        },
        .EndpointAddress    = JTAG_RX_EPADDR,
        .Attributes         = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        .EndpointSize       = JTAG_TXRX_EPSIZE,
    },

    .JTAG_DataInEndpoint = {
        .Header = {
            .Size   = sizeof(USB_Descriptor_Endpoint_t),
            .Type   = DTYPE_Endpoint,
        },
        .EndpointAddress    = JTAG_TX_EPADDR,
        .Attributes         = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        .EndpointSize       = JTAG_TXRX_EPSIZE,
    },
};

const USB_Descriptor_String_t EEMEM LanguageString = USB_STRING_DESCRIPTOR_ARRAY(LANGUAGE_ID_ENG);
const USB_Descriptor_String_t EEMEM ManufacturerString = USB_STRING_DESCRIPTOR(L"Jeff Kent <jeff@jkent.net>");
const USB_Descriptor_String_t EEMEM ProductString = USB_STRING_DESCRIPTOR(L"SRXE Basestation with FreeJTAG");
const USB_Descriptor_String_t EEMEM DataInterfaceString = USB_STRING_DESCRIPTOR(L"FreeJTAG CDC Interface");
const USB_Descriptor_String_t EEMEM JTAGInterfaceString = USB_STRING_DESCRIPTOR(L"FreeJTAG TAP Interface");

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
        case STRING_ID_Serial: {
            struct {
                USB_Descriptor_Header_t Header;
                uint16_t                UnicodeString[INTERNAL_SERIAL_LENGTH_BITS / 4 + 20];
            } SignatureDescriptor;
            SignatureDescriptor.Header.Type = DTYPE_String;
            SignatureDescriptor.Header.Size = USB_STRING_LEN(INTERNAL_SERIAL_LENGTH_BITS / 4);
            memcpy(SignatureDescriptor.UnicodeString, L"jkent.net:", 20);
            USB_Device_GetSerialString(SignatureDescriptor.UnicodeString + 10);
            Endpoint_ClearSETUP();
            Endpoint_Write_Control_Stream_LE(&SignatureDescriptor, sizeof(SignatureDescriptor));
            Endpoint_ClearOUT();
            break;
        }
        case STRING_ID_DataInterface:
            Address = &DataInterfaceString;
            Size    = DataInterfaceString.Header.Size;
            break;
        case STRING_ID_JTAGInterface:
            Address = &JTAGInterfaceString;
            Size    = JTAGInterfaceString.Header.Size;
            break;
        }
        break;
    }

    *DescriptorAddress = Address;
    return Size;
}
