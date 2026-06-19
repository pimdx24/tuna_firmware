#ifndef EECONFIG_H
#define EECONFIG_H

#include "common.h"

#define EECONFIG_MAGIC   0x544B4248U   /* "TBKH" */
#define EECONFIG_VERSION 0x0001U

#define DEFAULT_ACTUATION_POINT 128U   /* ~1.75mm default actuation depth */

/* Per-key actuation configuration.
 *
 * actuation_point  Distance threshold (0–255) to register a press in standard mode.
 *                  Default 128 ≈ 1.75mm. Configurable per key via web configurator.
 * rt_down          Rapid trigger down sensitivity in distance units. 0 = disabled.
 * rt_up            Rapid trigger up sensitivity. 0 = same as rt_down.
 * flags            bit0: continuous mode (must return to rest to reset rapid trigger). */
typedef struct {
    uint8_t actuation_point;
    uint8_t rt_down;
    uint8_t rt_up;
    uint8_t flags;
} key_actuation_t;

/* Persistent store — written to flash sector 7 (0x08060000, 128KB).
 *
 * calibrated        Set true by calibration_save(). Tells calibration_init()
 *                   whether flash contains valid rest/bottom_out values.
 * rest_value        ADC at rest per key, captured during boot calibration.
 * bottom_out_value  ADC at full press per key, tracked dynamically at runtime.
 * actuation_map     Per-key actuation config (threshold, rapid trigger).
 * keymap            Keycodes per layer per key. KC_TRANS (0xFFFF) = fall through.
 *                   Populated by web configurator over HID. */
typedef struct {
    uint32_t        magic;
    uint32_t        version;
    uint32_t        calibrated;
    uint16_t        rest_value[NUM_KEYS];
    uint16_t        bottom_out_value[NUM_KEYS];
    key_actuation_t actuation_map[NUM_KEYS];
    uint16_t        keymap[NUM_LAYERS][NUM_KEYS];
} eeconfig_t;

/* 4+4+4+122+122+244+488 = 988 bytes = 247 words */
_Static_assert(sizeof(eeconfig_t) % 4 == 0,
               "eeconfig_t must be word-aligned for flash writes");

/* RAM working copy. All modules read/write here.
 * Call eeconfig_save() to flush to flash. */
extern eeconfig_t eeconfig_ram;

void eeconfig_init(void);    /* load from flash or reset to defaults */
void eeconfig_save(void);    /* erase sector 7, write eeconfig_ram — blocks ~1-2s */
void eeconfig_reset(void);   /* restore defaults and save */

#endif /* EECONFIG_H */
