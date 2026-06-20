#include "keymap.h"

uint32_t layer_state = 0x01; /* layer 0 always active */

void keymap_init(void)
{
    layer_state = 0x01;
}

uint16_t keymap_get_keycode(uint8_t key)
{
    if (key >= NUM_KEYS) return KC_NONE;
    for (int8_t l = NUM_LAYERS - 1; l >= 0; l--)
    {
        if (!(layer_state & (1U << l))) continue;
        uint16_t kc = eeconfig_ram.keymap[l][key];
        if (kc != KC_TRANS) return kc;
    }
    return KC_NONE;
}

void keymap_layer_on(uint8_t layer)
{
    if (layer >= NUM_LAYERS) return;
    layer_state |= (1U << layer);
}

void keymap_layer_off(uint8_t layer)
{
    if (layer == 0 || layer >= NUM_LAYERS) return;
    layer_state &= ~(1U << layer);
}

void keymap_layer_toggle(uint8_t layer)
{
    if (layer == 0 || layer >= NUM_LAYERS) return;
    layer_state ^= (1U << layer);
}

const key_actuation_t *keymap_get_actuation(uint8_t key)
{
    return &eeconfig_ram.actuation_map[key];
}

void keymap_set_actuation(uint8_t key, key_actuation_t cfg)
{
    if (key >= NUM_KEYS) return;
    eeconfig_ram.actuation_map[key] = cfg;
}
