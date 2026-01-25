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
#if !defined(NO_INTERNAL_SERIAL)
    .SerialNumStrIndex      = STRING_ID_Serial,
#endif

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
        .TotalInterfaces        = INTERFACE_COUNT,
        .ConfigurationNumber    = 1,
        .ConfigurationStrIndex  = NO_DESCRIPTOR,
        .ConfigAttributes       = USB_CONFIG_ATTR_RESERVED,
        .MaxPowerConsumption    = USB_CONFIG_POWER_MA(100),
    },

#if !defined(CONTROL_ONLY_DEVICE)
    .CCI_Interface = {
        .Header = {
            .Size   = sizeof(USB_Descriptor_Interface_t),
            .Type   = DTYPE_Interface,
        },
        .InterfaceNumber        = INTERFACE_ID_CCI,
        .AlternateSetting       = 0,
        .TotalEndpoints         = 1,
        .Class                  = CDC_CSCP_CDCClass,
        .SubClass               = CDC_CSCP_ACMSubclass,
        .Protocol               = CDC_CSCP_NoDataProtocol,
        .InterfaceStrIndex      = NO_DESCRIPTOR,
    },

    .CCI_Functional_Header = {
        .Header = {
            .Size   = sizeof(USB_CDC_Descriptor_FunctionalHeader_t),
            .Type   = CDC_DTYPE_CSInterface,
        },
        .Subtype                = CDC_DSUBTYPE_CSInterface_Header,
        .CDCSpecification       = VERSION_BCD(1,1,0),
    },

    .CCI_Functional_ACM = {
        .Header = {
            .Size   = sizeof(USB_CDC_Descriptor_FunctionalACM_t),
            .Type   = CDC_DTYPE_CSInterface,
        },
        .Subtype                = CDC_DSUBTYPE_CSInterface_ACM,
        .Capabilities           = 0,
    },

    .CCI_Functional_Union = {
        .Header = {
            .Size   = sizeof(USB_CDC_Descriptor_FunctionalUnion_t),
            .Type   = CDC_DTYPE_CSInterface,
        },
        .Subtype                = CDC_DSUBTYPE_CSInterface_Union,
        .MasterInterfaceNumber  = INTERFACE_ID_CCI,
        .SlaveInterfaceNumber   = INTERFACE_ID_DCI,
    },

    .CCI_DataInEndpoint = {
        .Header = {
            .Size   = sizeof(USB_Descriptor_Endpoint_t),
            .Type   = DTYPE_Endpoint,
        },
        .EndpointAddress    = CCI_EPADDR,
        .Attributes         = (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC |
                                                        ENDPOINT_USAGE_DATA),
        .EndpointSize       = CCI_EPSIZE,
        .PollingIntervalMS  = 0xff,
    },

    .DCI_Interface = {
        .Header = {
            .Size   = sizeof(USB_Descriptor_Interface_t),
            .Type   = DTYPE_Interface,
        },
        .InterfaceNumber    = INTERFACE_ID_DCI,
        .AlternateSetting   = 0,
        .TotalEndpoints     = 2,
        .Class              = CDC_CSCP_CDCDataClass,
        .SubClass           = CDC_CSCP_NoDataSubclass,
        .Protocol           = CDC_CSCP_NoDataProtocol,
        .InterfaceStrIndex  = STRING_ID_DCIInterface,
    },

    .DCI_DataOutEndpoint = {
        .Header = {
            .Size   = sizeof(USB_Descriptor_Endpoint_t),
            .Type   = DTYPE_Endpoint,
        },
        .EndpointAddress    = DCI_RX_EPADDR,
        .Attributes         = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC |
                                                        ENDPOINT_USAGE_DATA),
        .EndpointSize       = DCI_TXRX_EPSIZE,
    },

    .DCI_DataInEndpoint = {
        .Header = {
            .Size   = sizeof(USB_Descriptor_Endpoint_t),
            .Type   = DTYPE_Endpoint,
        },
        .EndpointAddress    = DCI_TX_EPADDR,
        .Attributes         = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC |
                                                        ENDPOINT_USAGE_DATA),
        .EndpointSize       = DCI_TXRX_EPSIZE,
    },
#endif

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
#if !defined(CONTROL_ONLY_DEVICE)
const USB_Descriptor_String_t EEMEM DCIInterfaceString =
        USB_STRING_DESCRIPTOR(L"CDC ACM Interface");
#endif
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
#if !defined(NO_INTERNAL_SERIAL)
        case STRING_ID_Serial: {
            struct {
                USB_Descriptor_Header_t Header;
                uint16_t UnicodeString[INTERNAL_SERIAL_LENGTH_BITS / 4 + 20];
            } SignatureDescriptor;
            SignatureDescriptor.Header.Type = DTYPE_String;
            SignatureDescriptor.Header.Size =
                    USB_STRING_LEN(INTERNAL_SERIAL_LENGTH_BITS / 4);
            memcpy(SignatureDescriptor.UnicodeString, L"jkent.net:", 20);
            USB_Device_GetSerialString(SignatureDescriptor.UnicodeString + 10);
            Endpoint_ClearSETUP();
            Endpoint_Write_Control_Stream_LE(&SignatureDescriptor,
                    sizeof(SignatureDescriptor));
            Endpoint_ClearOUT();
            break;
        }
#endif
#if !defined(CONTROL_ONLY_DEVICE)
        case STRING_ID_DCIInterface:
            Address = &DCIInterfaceString;
            Size    = DCIInterfaceString.Header.Size;
            break;
#endif
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
