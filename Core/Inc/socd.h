#ifndef SOCD_H
#define SOCD_H

#include "common.h"

/* SOCD (Simultaneous Opposing Cardinal Directions) keycode-level resolution.
 *
 * Gated on eeconfig_ram.socd_enabled — no-op when disabled.
 * Resolution mode: neutral (both conflicting keycodes removed from the report).
 *
 * Opposing pairs resolved:
 *   KC_LEFT / KC_RIGHT   KC_UP / KC_DOWN   (arrow keys)
 *   KC_A    / KC_D       KC_W  / KC_S      (WASD — matched against resolved keycodes)
 *
 * Called by hid_keyboard_task() after Pass 2 builds the keycode array, before send.
 * Modifies keycodes[] in place and decrements *count for each removed keycode. */

void socd_resolve(uint8_t *keycodes, uint8_t *count);

#endif /* SOCD_H */
