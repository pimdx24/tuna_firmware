#ifndef KEYMAP_H
#define KEYMAP_H

#include "common.h"
#include "eeconfig.h"

/* Standard USB HID keycodes (usage page 0x07).
 * Values match the USB HID Usage Tables spec directly — pass these to tud_hid_keyboard_report().
 * Modifier keycodes (0xE0-0xE7) are reported in the modifier byte, not the keycode array.
 * Pseudo-keycodes KC_MO/KC_TG (0x5000+) are NOT in the HID spec; the HID layer must
 * intercept them and call keymap_layer_on/off rather than putting them in a report. */
#define KC_NONE  0x0000U
#define KC_TRANS 0xFFFFU   /* transparent: fall through to lower active layer */

/* Letters */
#define KC_A  0x0004U
#define KC_B  0x0005U
#define KC_C  0x0006U
#define KC_D  0x0007U
#define KC_E  0x0008U
#define KC_F  0x0009U
#define KC_G  0x000AU
#define KC_H  0x000BU
#define KC_I  0x000CU
#define KC_J  0x000DU
#define KC_K  0x000EU
#define KC_L  0x000FU
#define KC_M  0x0010U
#define KC_N  0x0011U
#define KC_O  0x0012U
#define KC_P  0x0013U
#define KC_Q  0x0014U
#define KC_R  0x0015U
#define KC_S  0x0016U
#define KC_T  0x0017U
#define KC_U  0x0018U
#define KC_V  0x0019U
#define KC_W  0x001AU
#define KC_X  0x001BU
#define KC_Y  0x001CU
#define KC_Z  0x001DU

/* Number row (unshifted) */
#define KC_1  0x001EU
#define KC_2  0x001FU
#define KC_3  0x0020U
#define KC_4  0x0021U
#define KC_5  0x0022U
#define KC_6  0x0023U
#define KC_7  0x0024U
#define KC_8  0x0025U
#define KC_9  0x0026U
#define KC_0  0x0027U

/* Core editing */
#define KC_ENTER 0x0028U
#define KC_ESC   0x0029U
#define KC_BKSP  0x002AU
#define KC_TAB   0x002BU
#define KC_SPC   0x002CU

/* Symbols (ANSI, unshifted labels) */
#define KC_MINS  0x002DU   /* -  */
#define KC_EQL   0x002EU   /* =  */
#define KC_LBRC  0x002FU   /* [  */
#define KC_RBRC  0x0030U   /* ]  */
#define KC_BSLS  0x0031U   /* \  */
#define KC_SCLN  0x0033U   /* ;  */
#define KC_QUOT  0x0034U   /* '  */
#define KC_GRV   0x0035U   /* `  */
#define KC_COMM  0x0036U   /* ,  */
#define KC_DOT   0x0037U   /* .  */
#define KC_SLSH  0x0038U   /* /  */

/* Lock */
#define KC_CAPS  0x0039U

/* Function keys */
#define KC_F1   0x003AU
#define KC_F2   0x003BU
#define KC_F3   0x003CU
#define KC_F4   0x003DU
#define KC_F5   0x003EU
#define KC_F6   0x003FU
#define KC_F7   0x0040U
#define KC_F8   0x0041U
#define KC_F9   0x0042U
#define KC_F10  0x0043U
#define KC_F11  0x0044U
#define KC_F12  0x0045U

/* Navigation / editing cluster */
#define KC_PSCR  0x0046U   /* Print Screen */
#define KC_SCRL  0x0047U   /* Scroll Lock  */
#define KC_PAUS  0x0048U   /* Pause/Break  */
#define KC_INS   0x0049U
#define KC_HOME  0x004AU
#define KC_PGUP  0x004BU
#define KC_DEL   0x004CU
#define KC_END   0x004DU
#define KC_PGDN  0x004EU
#define KC_RGHT  0x004FU
#define KC_LEFT  0x0050U
#define KC_DOWN  0x0051U
#define KC_UP    0x0052U

/* Modifiers — reported in the HID modifier byte, not the keycode array.
 * HID layer: if (kc >= KC_LCTL && kc <= KC_RGUI) set modifier bit (kc - KC_LCTL). */
#define KC_LCTL  0x00E0U
#define KC_LSFT  0x00E1U
#define KC_LALT  0x00E2U
#define KC_LGUI  0x00E3U
#define KC_RCTL  0x00E4U
#define KC_RSFT  0x00E5U
#define KC_RALT  0x00E6U
#define KC_RGUI  0x00E7U

/* Application/Menu key */
#define KC_APP   0x0065U

/* Pseudo-keycodes for layer control — NOT forwarded to HID.
 * HID layer must intercept these before building the report. */
#define KC_MO(n)  (0x5000U | ((n) & 0x0FU))   /* momentary: on while held */
#define KC_TG(n)  (0x5100U | ((n) & 0x0FU))   /* toggle: stays on/off each press */

#define KC_IS_LAYER_KEY(kc)  (((kc) & 0xFF00U) == 0x5000U || ((kc) & 0xFF00U) == 0x5100U)
#define KC_IS_MODIFIER(kc)   ((kc) >= KC_LCTL && (kc) <= KC_RGUI)

/* Active layer bitmask. Bit i = layer i active. Layer 0 always on. */
extern uint32_t layer_state;

void     keymap_init(void);

/* Walk active layers from highest to lowest.
 * Returns first non-transparent keycode, or KC_NONE if all layers are transparent. */
uint16_t keymap_get_keycode(uint8_t key);

void keymap_layer_on(uint8_t layer);
void keymap_layer_off(uint8_t layer);
void keymap_layer_toggle(uint8_t layer);

/* Actuation config lives in eeconfig_ram. These provide access without
 * exposing eeconfig internals to higher-level code. */
const key_actuation_t *keymap_get_actuation(uint8_t key);
void keymap_set_actuation(uint8_t key, key_actuation_t cfg);

#endif /* KEYMAP_H */
