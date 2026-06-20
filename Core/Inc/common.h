#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdbool.h>

#define NUM_KEYS     61
#define NO_KEY       0xFF

#define ADC_MAX      4095U    /* 12-bit ADC */
#define DISTANCE_MAX 255U     /* 0–255 travel scale */
#define DEADBAND     7U       /* 0.1mm / 3.5mm × 255 ≈ 7 units */

#define NUM_LAYERS   4

#define M_MIN(a, b) ((a) < (b) ? (a) : (b))
#define M_MAX(a, b) ((a) > (b) ? (a) : (b))
#define M_CLAMP(v, lo, hi) (M_MAX((lo), M_MIN((v), (hi))))
#define M_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#endif /* COMMON_H */
