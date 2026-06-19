#ifndef HID_H
#define HID_H

/* HID output stub — implement with TinyUSB.
 *
 * hid_task() should:
 *   1. Iterate key_matrix[i].is_pressed for i in 0..NUM_KEYS-1
 *   2. Look up keymap_get_keycode(i) for pressed keys
 *   3. Build a standard 6KRO boot report or custom NKRO report
 *   4. Send via TinyUSB tud_hid_keyboard_report()
 *
 * A second HID interface (custom report) can receive web configurator
 * commands (SET_REPORT) and reply with keyboard state (GET_REPORT).
 * Access key_matrix[], calib[], eeconfig_ram, and layer_state directly —
 * all are globally accessible from their respective headers. */

void hid_init(void);
void hid_task(void);

#endif /* HID_H */
