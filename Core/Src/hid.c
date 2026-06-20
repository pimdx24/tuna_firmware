#include "hid.h"
#include "usb.h"
#include "hid_keyboard.h"
#include "hid_config.h"

/* Coordinator stub — not yet implemented.
 * hid_init(): call usb_init(), hid_keyboard_init(), hid_config_init().
 * hid_task(): call hid_keyboard_task(), hid_config_task(). */

void hid_init(void) {}
void hid_task(void) {}
