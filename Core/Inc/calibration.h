#ifndef CALIBRATION_H
#define CALIBRATION_H

#include "common.h"

/* Initial bottom-out threshold: ADC counts above rest representing a full 3.5mm press.
 * Tune after first hardware measurement. The dynamic tracker in calibration_update()
 * corrects this upward as keys are pressed during normal use. */
#define INITIAL_BOTTOM_OUT_THRESHOLD 400U

/* Boot calibration: 500ms of EMA filtering with all keys at rest.
 * Tracks the minimum filtered ADC per key → rest_value.
 * Minimum tracking means accidental presses during boot are harmless
 * (pressing raises ADC, which does not update the minimum). */
#define CALIBRATION_DURATION_MS 500U

/* Minimum ADC change above current bottom_out to trigger a dynamic update.
 * Prevents noise from incrementally walking bottom_out upward. */
#define CALIBRATION_EPSILON 5U

typedef struct {
    uint16_t rest_value;
    uint16_t bottom_out_value;
} key_calib_t;

/* Per-key calibration data. Read by matrix.c and distance.h every scan. */
extern key_calib_t calib[NUM_KEYS];

/* Run 500ms boot calibration (if no flash data), or load from flash.
 * Call after eeconfig_init() and after TIM2 has been started. */
void calibration_init(void);

/* Reset calibration data and re-run boot calibration.
 * Call with all keys at rest. Saves to flash on completion. */
void calibration_recalibrate(void);

/* Called by matrix_scan() every scan. Updates bottom_out_value if the key
 * was pressed deeper than the current tracked maximum. */
void calibration_update(uint8_t key, uint16_t adc_filtered);

/* Write calib[] into eeconfig_ram and call eeconfig_save(). Blocks ~1-2s. */
bool calibration_save(void);

/* Load calib[] from eeconfig_ram. Returns false if no valid data in flash. */
bool calibration_load(void);

#endif /* CALIBRATION_H */
