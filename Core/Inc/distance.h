#ifndef DISTANCE_H
#define DISTANCE_H

#include "common.h"

/* Linear ADC-to-distance conversion with 0.1mm deadband at both ends.
 *
 * The OH49E sensor output vs. physical distance is slightly logarithmic
 * (larger voltage change near the top of travel). This linear approximation
 * is adequate for threshold-based actuation. For rapid trigger with sub-0.1mm
 * sensitivity, replace with a logarithmic LUT calibrated to measured data.
 * See libhmk tools/distance_lut.py for the curve-fitting approach used with
 * the same sensor (GEON Raw HE + OH49E-S).
 *
 * Deadband: 0.1mm / 3.5mm × 255 ≈ 7 distance units at each end.
 * Readings within 7 units of rest → 0. Within 7 units of bottom → 255.
 * This prevents false triggers from sensor noise at travel limits. */
static inline uint8_t adc_to_distance(uint16_t adc,
                                       uint16_t rest,
                                       uint16_t bottom_out)
{
    if (adc <= rest || rest >= bottom_out) return 0;
    if (adc >= bottom_out) return DISTANCE_MAX;

    uint8_t d = (uint8_t)((uint32_t)(adc - rest) * DISTANCE_MAX
                           / (uint32_t)(bottom_out - rest));

    if (d < DEADBAND)                return 0;
    if (d > DISTANCE_MAX - DEADBAND) return DISTANCE_MAX;
    return d;
}

#endif /* DISTANCE_H */
