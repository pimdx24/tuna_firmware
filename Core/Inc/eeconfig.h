#ifndef EECONFIG_H
#define EECONFIG_H

#include "common.h"

#define EECONFIG_MAGIC   0x544B4248U /* "TBKH" */
#define EECONFIG_VERSION 0x0005U

#define DEFAULT_ACTUATION_POINT 128U /* ~1.75mm default actuation depth */

/* Per-key actuation configuration.
 *
 * actuation_point  Distance threshold (0–255) for a standard press. Default 128 ≈ 1.75mm.
 * rt_down          Rapid trigger down sensitivity in distance units. 0 = disabled per key.
 * rt_up            Rapid trigger up sensitivity. 0 = same as rt_down.
 * flags            bit0: continuous mode (must return to rest to reset rapid trigger). */
typedef struct {
    uint8_t actuation_point;
    uint8_t rt_down;
    uint8_t rt_up;
    uint8_t flags;
} key_actuation_t; /* 4 bytes */

/* Persistent store — written to flash sector 7 (0x08060000, 128KB).
 *
 * calibrated        Set true by calibration_save(). Tells calibration_init()
 *                   whether flash contains valid rest/bottom_out values.
 * rt_enabled        Global rapid trigger enable. 0=disabled (all keys use standard
 *                   threshold mode regardless of rt_down/rt_up), 1=enabled (per-key
 *                   rt_down/rt_up values apply). Toggled via web configurator.
 * socd_enabled      Global SOCD resolution enable. 0=disabled, 1=neutral mode
 *                   (conflicting opposing-direction keycodes cancel each other).
 *                   Toggled via web configurator.
 * _reserved[2]      Padding to maintain word alignment. Reserved for future flags.
 * rest_value        ADC at rest per key, captured during boot calibration.
 * bottom_out_value  ADC at full press per key, tracked dynamically at runtime.
 * actuation_map     Per-key actuation config (threshold, rapid trigger sensitivity).
 * keymap            Keycodes per layer per key. KC_TRANS (0xFFFF) = fall through.
 *                     [0] base QWERTY — normal actuation
 *                     [1] Fn momentary overlay
 *                     [2] user-defined layer 2 (web configurator)
 *                     [3] user-defined layer 3 (web configurator) */
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t calibrated;
    uint8_t  rt_enabled;
    uint8_t  socd_enabled;
    uint8_t  _reserved[2];
    uint16_t rest_value[NUM_KEYS];
    uint16_t bottom_out_value[NUM_KEYS];
    key_actuation_t actuation_map[NUM_KEYS];
    uint16_t keymap[NUM_LAYERS][NUM_KEYS];
} eeconfig_t;

/* 4+4+4+1+1+2 + 61×2 + 61×2 + 61×4 + 4×61×2 = 992 bytes = 248 words */
_Static_assert(sizeof(eeconfig_t) == 992,
               "eeconfig_t size mismatch — update EECONFIG_VERSION and this assert");
_Static_assert(sizeof(eeconfig_t) % 4 == 0,
               "eeconfig_t must be word-aligned for flash writes");

/* RAM working copy. All modules read/write here.
 * Call eeconfig_save() to flush to flash. */
extern eeconfig_t eeconfig_ram;

void eeconfig_init(void);  /* load from flash or reset to defaults */
void eeconfig_save(void);  /* erase sector 7, write eeconfig_ram — blocks ~1-2s */
void eeconfig_reset(void); /* restore defaults and save */

#endif /* EECONFIG_H */
