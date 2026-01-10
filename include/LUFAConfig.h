#ifndef _LUFA_CONFIG_H_
#define _LUFA_CONFIG_H_

    /* General USB Driver Related Tokens: */
    #define ORDERED_EP_CONFIG
    #define USE_STATIC_OPTIONS              (USB_DEVICE_OPT_FULLSPEED | USB_OPT_REG_ENABLED | USB_OPT_AUTO_PLL)
    #define USB_DEVICE_ONLY

    /* USB Device Mode Driver Related Tokens: */
    #define USE_EEPROM_DESCRIPTORS
    #define FIXED_CONTROL_ENDPOINT_SIZE     32
    #define DEVICE_STATE_AS_GPIOR           0
    #define FIXED_NUM_CONFIGURATIONS        1
    #define INTERRUPT_CONTROL_ENDPOINT
    #define NO_DEVICE_REMOTE_WAKEUP

#endif
