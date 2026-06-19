#ifndef KEYMAP_H
#define KEYMAP_H

#include "common.h"
#include "eeconfig.h"

/* Standard USB HID keycodes (usage page 0x07).
 * Extend as needed; web configurator should send raw HID codes directly. */
#define KC_NONE  0x0000U   /* No keycode / empty slot */
#define KC_TRANS 0xFFFFU   /* Transparent: fall through to lower active layer */

/* Basic keycodes for default keymap setup */
#define KC_A     0x0004U
#define KC_B     0x0005U
#define KC_C     0x0006U
#define KC_D     0x0007U
#define KC_E     0x0008U
#define KC_F     0x0009U
#define KC_G     0x000AU
#define KC_H     0x000BU
#define KC_I     0x000CU
#define KC_J     0x000DU
#define KC_K     0x000EU
#define KC_L     0x000FU
#define KC_M     0x0010U
#define KC_N     0x0011U
#define KC_O     0x0012U
#define KC_P     0x0013U
#define KC_Q     0x0014U
#define KC_R     0x0015U
#define KC_S     0x0016U
#define KC_T     0x0017U
#define KC_U     0x0018U
#define KC_V     0x0019U
#define KC_W     0x001AU
#define KC_X     0x001BU
#define KC_Y     0x001CU
#define KC_Z     0x001DU

/* Active layer bitmask. Bit i = layer i active. Layer 0 always on. */
extern uint32_t layer_state;

void     keymap_init(void);

/* Walk active layers from highest to lowest.
 * Returns first non-transparent keycode, or KC_NONE if all layers are transparent. */
uint16_t keymap_get_keycode(uint8_t key);

void     keymap_layer_on(uint8_t layer);
void     keymap_layer_off(uint8_t layer);
void     keymap_layer_toggle(uint8_t layer);

/* Actuation config lives in eeconfig_ram. These provide access without
 * exposing eeconfig internals to higher-level code. */
const key_actuation_t *keymap_get_actuation(uint8_t key);
void                   keymap_set_actuation(uint8_t key, key_actuation_t cfg);

#endif /* KEYMAP_H */
