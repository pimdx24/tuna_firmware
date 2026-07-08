#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

/* TinyUSB configuration — STM32F446 OTG_FS, device-only, bare metal.
 * Included by tusb_option.h before anything else in the stack. */

/* Port/RTOS */
#define CFG_TUSB_MCU        OPT_MCU_STM32F4
#define CFG_TUSB_OS         OPT_OS_NONE

/* Device stack on rhport 0 (OTG_FS), full speed only */
#define CFG_TUD_ENABLED     1
#define CFG_TUD_MAX_SPEED   OPT_MODE_FULL_SPEED

/* No special RAM section; USB buffers only need word alignment */
#define CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_ALIGN  __attribute__ ((aligned(4)))

#define CFG_TUD_ENDPOINT0_SIZE 64

/* Class drivers: two HID interfaces (see usb_descriptors.c)
 *   instance 0 — boot keyboard
 *   instance 1 — vendor/raw HID for the web configurator */
#define CFG_TUD_HID            2

/* Endpoint buffer must fit the largest report: 64-byte vendor reports */
#define CFG_TUD_HID_EP_BUFSIZE 64

#endif /* TUSB_CONFIG_H */
