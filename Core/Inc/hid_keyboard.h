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
 *     For each is_pressed key use the keycode latched at its press edge
 *     (resolved via keymap_get_keycode() at that moment, held until release).
 *     Modifier keycodes (KC_LCTL–KC_RGUI) set bits in the modifier byte.
 *     Up to 6 non-modifier keycodes fill the keycode array; more than 6 sends
 *     ErrorRollOver (0x01 in every slot) per the boot protocol.
 *     Call socd_resolve() on the keycode array if eeconfig_ram.socd_enabled.
 *     Send via TinyUSB only when the report differs from the last accepted one.
 *
 * hid_keyboard_usb_reset() — call on USB (re)configuration: the host then
 * assumes all keys are up, so the "last sent" report is cleared. */

void hid_keyboard_init(void);
void hid_keyboard_task(void);
void hid_keyboard_usb_reset(void);

#endif /* HID_KEYBOARD_H */
