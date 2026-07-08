#include "tusb.h"
#include "stm32f4xx.h"   /* UID_BASE for the serial number string */

/* -------------------------------------------------------------------------
 * Device identity
 * 0xCAFE is the TinyUSB test VID — fine for development, but get a real
 * VID/PID (e.g. pid.codes) before distributing hardware.
 * ------------------------------------------------------------------------- */
#define USB_VID  0xCAFEU
#define USB_PID  0x6060U
#define USB_BCD  0x0100U /* device release 1.00 */

enum {
    ITF_NUM_KEYBOARD = 0, /* HID instance 0 — boot keyboard */
    ITF_NUM_CONFIG,       /* HID instance 1 — vendor HID for web configurator */
    ITF_NUM_TOTAL
};

#define EPNUM_KEYBOARD_IN 0x81
#define EPNUM_CONFIG_OUT  0x02
#define EPNUM_CONFIG_IN   0x82

/* String descriptor indices */
enum {
    STRID_LANGID = 0,
    STRID_MANUFACTURER,
    STRID_PRODUCT,
    STRID_SERIAL,
    STRID_ITF_KEYBOARD,
    STRID_ITF_CONFIG,
};

/* -------------------------------------------------------------------------
 * Device descriptor
 * ------------------------------------------------------------------------- */
static const tusb_desc_device_t desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00, /* per-interface class */
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = USB_VID,
    .idProduct          = USB_PID,
    .bcdDevice          = USB_BCD,
    .iManufacturer      = STRID_MANUFACTURER,
    .iProduct           = STRID_PRODUCT,
    .iSerialNumber      = STRID_SERIAL,
    .bNumConfigurations = 0x01,
};

uint8_t const *tud_descriptor_device_cb(void)
{
    return (uint8_t const *)&desc_device;
}

/* -------------------------------------------------------------------------
 * HID report descriptors
 * ------------------------------------------------------------------------- */
static const uint8_t desc_hid_report_keyboard[] = {
    TUD_HID_REPORT_DESC_KEYBOARD()
};

static const uint8_t desc_hid_report_config[] = {
    TUD_HID_REPORT_DESC_GENERIC_INOUT(64)
};

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    return (instance == ITF_NUM_KEYBOARD) ? desc_hid_report_keyboard
                                          : desc_hid_report_config;
}

/* -------------------------------------------------------------------------
 * Configuration descriptor
 * Keyboard: boot protocol, 8-byte IN reports, 1ms polling.
 * Config:   64-byte IN/OUT reports, 1ms polling.
 * ------------------------------------------------------------------------- */
#define CONFIG_TOTAL_LEN \
    (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN + TUD_HID_INOUT_DESC_LEN)

static const uint8_t desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0, 100),

    TUD_HID_DESCRIPTOR(ITF_NUM_KEYBOARD, STRID_ITF_KEYBOARD,
                       HID_ITF_PROTOCOL_KEYBOARD,
                       sizeof(desc_hid_report_keyboard),
                       EPNUM_KEYBOARD_IN, 8, 1),

    TUD_HID_INOUT_DESCRIPTOR(ITF_NUM_CONFIG, STRID_ITF_CONFIG,
                             HID_ITF_PROTOCOL_NONE,
                             sizeof(desc_hid_report_config),
                             EPNUM_CONFIG_OUT, EPNUM_CONFIG_IN, 64, 1),
};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;
    return desc_configuration;
}

/* -------------------------------------------------------------------------
 * String descriptors
 * ------------------------------------------------------------------------- */
static const char *const string_desc_arr[] = {
    [STRID_LANGID]       = NULL, /* handled specially: 0x0409 English (US) */
    [STRID_MANUFACTURER] = "TUNA",
    [STRID_PRODUCT]      = "TUNA60 HE",
    [STRID_SERIAL]       = NULL, /* generated from the MCU unique ID */
    [STRID_ITF_KEYBOARD] = "TUNA60 HE Keyboard",
    [STRID_ITF_CONFIG]   = "TUNA60 HE Config",
};

/* 96-bit MCU unique ID as 24 hex chars, so each board gets a stable,
 * distinct serial and hosts don't mix up per-device settings. */
static uint8_t serial_ascii(char buf[24])
{
    const uint32_t *uid = (const uint32_t *)UID_BASE;
    static const char hex[] = "0123456789ABCDEF";
    for (uint8_t w = 0; w < 3; w++)
        for (uint8_t n = 0; n < 8; n++)
            buf[w * 8 + n] = hex[(uid[w] >> (28 - 4 * n)) & 0xF];
    return 24;
}

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void)langid;
    /* UTF-16 buffer: [0] = header, up to 32 chars payload */
    static uint16_t desc_str[33];
    uint8_t len;

    if (index == STRID_LANGID)
    {
        desc_str[1] = 0x0409;
        len = 1;
    }
    else if (index == STRID_SERIAL)
    {
        char ascii[24];
        len = serial_ascii(ascii);
        for (uint8_t i = 0; i < len; i++)
            desc_str[1 + i] = (uint16_t)ascii[i];
    }
    else if (index < TU_ARRAY_SIZE(string_desc_arr) && string_desc_arr[index])
    {
        const char *s = string_desc_arr[index];
        for (len = 0; s[len] && len < 32; len++)
            desc_str[1 + len] = (uint16_t)s[len];
    }
    else
    {
        return NULL;
    }

    desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * (len + 1)));
    return desc_str;
}
