#include "hid_keyboard.h"
#include "matrix.h"
#include "keymap.h"
#include "socd.h"
#include "eeconfig.h"
#include "usb.h"
#include "tusb.h"
#include <string.h>

/* Boot keyboard HID (interface 0). Two passes per scan, see hid_keyboard.h.
 *
 * Keycodes are latched at the press edge and held until the release edge.
 * Resolving live every scan would retarget a held key whenever the layer
 * stack changes underneath it: press Fn, press the "arrow" key, release Fn
 * first and the same physical key would suddenly report its base-layer
 * letter while still held, then "release" a letter it never pressed. With a
 * latch, whatever went down is exactly what comes up — this also guarantees
 * that MO(n) always turns off the layer it turned on. */

#define REPORT_KEYS     6U
#define KC_ERR_ROLLOVER 0x01U /* boot protocol: all slots = 0x01 on >6 keys */

static bool     prev_pressed[NUM_KEYS];
static uint16_t held_kc[NUM_KEYS];  /* latched keycode while pressed, KC_NONE otherwise */

/* Last report accepted by the USB stack. Only differences are sent. */
static uint8_t last_mod;
static uint8_t last_keys[REPORT_KEYS];

static inline bool is_momentary(uint16_t kc) { return (kc & 0xFF00U) == 0x5000U; }
static inline uint8_t layer_of(uint16_t kc) { return (uint8_t)(kc & 0x0FU); }

static void layer_key_press(uint16_t kc)
{
    if (is_momentary(kc)) keymap_layer_on(layer_of(kc));
    /* KC_TG acts on release only */
}

static void layer_key_release(uint16_t kc)
{
    if (is_momentary(kc)) keymap_layer_off(layer_of(kc));
    else                  keymap_layer_toggle(layer_of(kc));
}

void hid_keyboard_init(void)
{
    memset(prev_pressed, 0, sizeof(prev_pressed));
    for (uint8_t i = 0; i < NUM_KEYS; i++) held_kc[i] = KC_NONE;
    hid_keyboard_usb_reset();
}

void hid_keyboard_usb_reset(void)
{
    /* After (re)enumeration the host assumes every key is up. Forget what
     * was sent so the current state is re-sent on the next difference. */
    last_mod = 0;
    memset(last_keys, 0, sizeof(last_keys));
}

void hid_keyboard_task(void)
{
    /* Pass 1 — layer keys only. Handle their edges before anything else is
     * resolved, so pass 2 sees the final layer state for this scan. */
    for (uint8_t i = 0; i < NUM_KEYS; i++)
    {
        bool now = key_matrix[i].is_pressed;
        if (now == prev_pressed[i]) continue;

        if (now)
        {
            uint16_t kc = keymap_get_keycode(i);
            if (KC_IS_LAYER_KEY(kc))
            {
                held_kc[i] = kc;
                prev_pressed[i] = true;
                layer_key_press(kc);
            }
            /* non-layer press edges are latched in pass 2 */
        }
        else if (KC_IS_LAYER_KEY(held_kc[i]))
        {
            layer_key_release(held_kc[i]);
            held_kc[i] = KC_NONE;
            prev_pressed[i] = false;
        }
    }

    /* Pass 2 — latch the remaining edges and build the 8-byte boot report. */
    uint8_t mod = 0;
    uint8_t keys[REPORT_KEYS] = {0};
    uint8_t count = 0;
    bool overflow = false;

    for (uint8_t i = 0; i < NUM_KEYS; i++)
    {
        bool now = key_matrix[i].is_pressed;
        if (now != prev_pressed[i])
        {
            prev_pressed[i] = now;
            if (now)
            {
                uint16_t kc = keymap_get_keycode(i);
                held_kc[i] = kc;
                /* A layer key exposed by a layer change earlier this scan. */
                if (KC_IS_LAYER_KEY(kc)) layer_key_press(kc);
            }
            else
            {
                held_kc[i] = KC_NONE;
            }
        }
        if (!now) continue;

        uint16_t kc = held_kc[i];
        if (kc == KC_NONE || KC_IS_LAYER_KEY(kc)) continue;

        if (KC_IS_MODIFIER(kc))
            mod |= (uint8_t)(1U << (kc - KC_LCTL));
        else if (kc > 0xFFU)
            continue;                       /* not a boot-protocol usage */
        else if (count < REPORT_KEYS)
            keys[count++] = (uint8_t)kc;
        else
            overflow = true;
    }

    if (overflow)
    {
        memset(keys, KC_ERR_ROLLOVER, sizeof(keys));
        count = REPORT_KEYS;
    }

    if (eeconfig_ram.socd_enabled)
        socd_resolve(keys, &count);

    if (mod == last_mod && memcmp(keys, last_keys, sizeof(keys)) == 0)
        return;

    /* Previous report still in flight (1ms interrupt endpoint) — retry next
     * scan with whatever the state is then. */
    if (!tud_hid_n_ready(USB_HID_KEYBOARD))
        return;

    if (tud_hid_n_keyboard_report(USB_HID_KEYBOARD, 0, mod, keys))
    {
        last_mod = mod;
        memcpy(last_keys, keys, sizeof(keys));
    }
}
