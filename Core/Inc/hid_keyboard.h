#ifndef HID_KEYBOARD_H
#define HID_KEYBOARD_H

/* Standard boot keyboard HID interface (Interface 0).
 *
 * hid_keyboard_task() runs two passes each 4kHz scan, after matrix_scan():
 *
 *   Pass 1 — layer keys (must precede report build so layer state is current):
 *     Detect is_pressed transitions; for KC_MO call keymap_layer_on/off,
 *     for KC_TG call keymap_layer_toggle on falling edge only.
 *
 *   Pass 2 — build 8-byte boot keyboard report:
 *     For each is_pressed key, resolve keycode via keymap_get_keycode().
 *     Modifier keycodes (KC_LCTL–KC_RGUI) set bits in the modifier byte.
 *     Up to 6 non-modifier keycodes fill the keycode array.
 *     Call socd_resolve() on the keycode array if eeconfig_ram.socd_enabled.
 *     Send via TinyUSB only when the report differs from the previous one. */

void hid_keyboard_init(void);
void hid_keyboard_task(void);

#endif /* HID_KEYBOARD_H */
