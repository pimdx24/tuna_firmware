#ifndef USB_H
#define USB_H

/* TinyUSB device stack for STM32F446RET6 (OTG_FS, PLLQ=4 → 48MHz USB clock).
 *
 * Two HID interfaces in the device descriptor:
 *   Interface 0 — boot keyboard (8-byte reports, subclass=1, protocol=1)
 *   Interface 1 — vendor/raw HID for web configurator (64-byte IN/OUT reports)
 *
 * usb_init(): configure OTG_FS peripheral, register TinyUSB descriptors, start device.
 * usb_task(): pumps the TinyUSB stack. Must be called from the main() while(1) loop,
 *             NOT from keyboard_task(), as it must run faster than the 4kHz scan. */

void usb_init(void);
void usb_task(void);

#endif /* USB_H */
