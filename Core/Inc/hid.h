#ifndef HID_H
#define HID_H

/* HID output stub — implement with TinyUSB.
 *
 * hid_task() processing order (order matters for same-scan correctness):
 *
 *   Pass 1 — layer keys:
 *     For each key i with is_pressed changed:
 *       kc = keymap_get_keycode(i)
 *       if KC_IS_LAYER_KEY(kc):
 *         if KC_MO: pressed → keymap_layer_on(kc & 0x0F), released → keymap_layer_off(kc & 0x0F)
 *         if KC_TG: on falling edge → keymap_layer_toggle(kc & 0x0F)
 *
 *   Pass 2 — build report:
 *     For each key i with is_pressed == true:
 *       kc = keymap_get_keycode(i)
 *       if KC_IS_MODIFIER(kc): set modifier byte bit (kc - KC_LCTL)
 *       else if kc != KC_NONE && !KC_IS_LAYER_KEY(kc): add to keycode array
 *
 *   Send report only when it differs from the previous one (debounce host).
 *
 * A second HID interface (custom report) handles the web configurator:
 *   SET_REPORT → write keymap/actuation config into eeconfig_ram, call eeconfig_save()
 *   GET_REPORT → return key_matrix[], calib[], eeconfig_ram snapshot
 * All state (key_matrix, calib, eeconfig_ram, layer_state) is globally accessible. */

void hid_init(void);
void hid_task(void);

#endif /* HID_H */
