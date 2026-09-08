#ifndef CALIBRATION_H
#define CALIBRATION_H

#include "common.h"

/* Initial bottom-out seed: ADC counts above rest assumed for a full 3.5mm press
 * until the key has actually been bottomed out once.
 *
 * This MUST be a LOWER bound of the real swing. calibration_update() only ever
 * raises bottom_out_value (it tracks the deepest reading seen), so a seed above
 * the real swing is never corrected and keys can never reach the actuation
 * point. A seed below the real swing is corrected the first time each key is
 * pressed fully; until then that key reads deeper than it really is (a partial
 * press can register as 255), which is a transient, not a lock-out.
 *
 * Confirm against the GET calib (0x06) readout on the first board: the
 * learned bottom_out - rest of a fully pressed key is the real swing, and this
 * seed should sit comfortably below it. */
#define INITIAL_BOTTOM_OUT_THRESHOLD 150U

/* Boot calibration: 500ms of EMA filtering with all keys at rest.
 * Tracks the minimum filtered ADC per key → rest_value.
 * Minimum tracking means accidental presses during boot are harmless
 * (pressing raises ADC, which does not update the minimum).
 *
 * The first CALIBRATION_SETTLE_MS are excluded from the minimum: the filter
 * is seeded from one raw sample, so until it has settled its output carries
 * that sample's noise (up to the full raw noise amplitude), which would bias
 * rest_value low and let rest noise leak through as non-zero distance.
 * 50ms = 200 samples ≈ 13 filter time constants. */
#define CALIBRATION_DURATION_MS 500U
#define CALIBRATION_SETTLE_MS   50U

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
void calibration_save(void);

/* Load calib[] from eeconfig_ram. Returns false if no valid data in flash. */
bool calibration_load(void);

#endif /* CALIBRATION_H */
