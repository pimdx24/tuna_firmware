#ifndef HID_H
#define HID_H

/* HID coordinator — delegates to four subsystems:
 *
 *   usb.h          TinyUSB device stack init and USB callbacks
 *   hid_keyboard.h standard boot keyboard interface (key reports, layer keys, SOCD)
 *   hid_config.h   vendor HID interface for web configurator (GET/SET eeconfig)
 *   socd.h         SOCD keycode resolution, called from hid_keyboard before send
 *
 * hid_init() initialises all four subsystems.
 * hid_task() drives hid_keyboard_task() and hid_config_task() each 4kHz scan.
 * usb_task() must be called from the main loop (not here) to pump TinyUSB. */

void hid_init(void);
void hid_task(void);

#endif /* HID_H */
