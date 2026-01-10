/* SPDX-License-Identifier: MIT */
/*
 * FreeJTAG
 * Copyright (C) 2026 Jeff Kent <jeff@jkent.net>
 */

#pragma once

#include <LUFA/Drivers/USB/USB.h>


#define CDC_NOTIFICATION_EPADDR             (ENDPOINT_DIR_IN  | 2)
#define CDC_RX_EPADDR                       (ENDPOINT_DIR_OUT | 3)
#define CDC_TX_EPADDR                       (ENDPOINT_DIR_IN  | 4)

#define CDC_NOTIFICATION_EPSIZE             8
#define CDC_TXRX_EPSIZE                     32

typedef struct {
    USB_Descriptor_Configuration_Header_t   Config;

    // Interface Association
    USB_Descriptor_Interface_Association_t  CDC_Association;

    // CDC Control Interface
    USB_Descriptor_Interface_t              CDC_CCI_Interface;
    USB_CDC_Descriptor_FunctionalHeader_t   CDC_Functional_Header;
    USB_CDC_Descriptor_FunctionalACM_t      CDC_Functional_ACM;
    USB_CDC_Descriptor_FunctionalUnion_t    CDC_Functional_Union;
    USB_Descriptor_Endpoint_t               CDC_NotificationEndpoint;

    // CDC Data Interface
    USB_Descriptor_Interface_t              CDC_DCI_Interface;
    USB_Descriptor_Endpoint_t               CDC_DataOutEndpoint;
    USB_Descriptor_Endpoint_t               CDC_DataInEndpoint;
} USB_Descriptor_CDC_Configuration_t;

enum CDCInterfaceDescriptors_t {
    INTERFACE_ID_CDC_CCI = 0, /**< CDC CCI interface descriptor ID */
    INTERFACE_ID_CDC_DCI = 1, /**< CDC DCI interface descriptor ID */
};

enum Configurations_t {
    CONFIGURATION_NONE  = 0,
    CONFIGURATION_CDC   = 1,
};

enum StringDescriptors_t {
    STRING_ID_Language      = 0, /**< Supported Languages string descriptor ID (must be zero) */
    STRING_ID_Manufacturer  = 1, /**< Manufacturer string ID */
    STRING_ID_Product       = 2, /**< Product string ID */
};

uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint16_t wIndex,
                                    const void** const DescriptorAddress)
                                    ATTR_WARN_UNUSED_RESULT ATTR_NON_NULL_PTR_ARG(3);
