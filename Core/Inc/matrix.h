#ifndef MATRIX_H
#define MATRIX_H

#include "common.h"

typedef enum {
    KEY_DIR_INACTIVE,
    KEY_DIR_DOWN,
    KEY_DIR_UP,
} key_dir_t;

/* Per-key runtime state. Indexed 0–60.
 *
 * adc_acc       EMA state in 12.4 fixed point (16 × adc_filtered). Keeping the
 *               fractional residue lets the filter track steps smaller than
 *               16 ADC counts; a plain (raw + 15*old) >> 4 truncates them away.
 * adc_filtered  EMA-smoothed ADC reading (adc_acc >> 4). Updated every scan at 4kHz.
 * distance      Physical travel 0–255 (0=rest, 255=3.5mm full press).
 *               Valid once calibration data exists for this key.
 * extremum      Peak distance while DIR_DOWN, or valley while DIR_UP.
 *               Used by rapid trigger to detect direction reversals.
 * dir           Current movement direction for the rapid trigger state machine.
 * is_pressed    Key state after actuation/RT logic. Read by HID to build reports. */
typedef struct {
    uint32_t adc_acc;
    uint16_t adc_filtered;
    uint8_t distance;
    uint8_t extremum;
    key_dir_t dir;
    bool is_pressed;
} key_state_t; /* 16 bytes */

extern key_state_t key_matrix[NUM_KEYS];

void matrix_init(void);
void matrix_scan(void);

#endif /* MATRIX_H */
