#include "socd.h"
#include "eeconfig.h"
#include "keymap.h"
#include <string.h>

/* Neutral SOCD: when both keycodes of an opposing pair are in the outgoing
 * report, drop both. Runs on the final resolved 8-bit HID usages so it works
 * for remapped layouts too (whatever ends up as A/D, W/S, arrows). */

static const uint8_t socd_pairs[][2] = {
    { (uint8_t)KC_LEFT, (uint8_t)KC_RGHT },
    { (uint8_t)KC_UP,   (uint8_t)KC_DOWN },
    { (uint8_t)KC_A,    (uint8_t)KC_D    },
    { (uint8_t)KC_W,    (uint8_t)KC_S    },
};

static bool contains(const uint8_t *keycodes, uint8_t count, uint8_t kc)
{
    for (uint8_t i = 0; i < count; i++)
        if (keycodes[i] == kc) return true;
    return false;
}

/* Remove the first occurrence of kc, shift the tail down, zero the freed slot
 * so the array stays a valid zero-padded 6-slot report. */
static void remove_kc(uint8_t *keycodes, uint8_t *count, uint8_t kc)
{
    for (uint8_t i = 0; i < *count; i++)
    {
        if (keycodes[i] != kc) continue;
        memmove(&keycodes[i], &keycodes[i + 1], (size_t)(*count - i - 1));
        (*count)--;
        keycodes[*count] = 0;
        return;
    }
}

void socd_resolve(uint8_t *keycodes, uint8_t *count)
{
    if (!eeconfig_ram.socd_enabled) return;

    for (uint8_t p = 0; p < M_ARRAY_SIZE(socd_pairs); p++)
    {
        uint8_t a = socd_pairs[p][0];
        uint8_t b = socd_pairs[p][1];
        if (contains(keycodes, *count, a) && contains(keycodes, *count, b))
        {
            remove_kc(keycodes, count, a);
            remove_kc(keycodes, count, b);
        }
    }
}
