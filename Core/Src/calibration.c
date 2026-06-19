#include "calibration.h"
#include "analog.h"
#include "eeconfig.h"
#include "stm32f4xx_hal.h"

key_calib_t calib[NUM_KEYS];

static void run_boot_calibration(void)
{
    uint16_t ema[NUM_KEYS];
    for (uint8_t i = 0; i < NUM_KEYS; i++)
        ema[i] = analog_read(i);

    uint32_t start = HAL_GetTick();
    while (HAL_GetTick() - start < CALIBRATION_DURATION_MS) {
        analog_task();
        for (uint8_t i = 0; i < NUM_KEYS; i++) {
            ema[i] = (uint16_t)(((uint32_t)analog_read(i) + (uint32_t)ema[i] * 15U) >> 4);
            if (ema[i] < calib[i].rest_value)
                calib[i].rest_value = ema[i];
        }
    }

    for (uint8_t i = 0; i < NUM_KEYS; i++)
        calib[i].bottom_out_value = calib[i].rest_value + INITIAL_BOTTOM_OUT_THRESHOLD;
}

void calibration_init(void)
{
    if (calibration_load()) return;

    /* Block until the first scan is available so rest values are seeded
     * from real ADC readings, not the zero-initialised adc_values buffer. */
    while (!analog_task()) {}

    for (uint8_t i = 0; i < NUM_KEYS; i++) {
        calib[i].rest_value       = analog_read(i);
        calib[i].bottom_out_value = calib[i].rest_value + INITIAL_BOTTOM_OUT_THRESHOLD;
    }

    run_boot_calibration();
    calibration_save();
}

void calibration_recalibrate(void)
{
    while (!analog_task()) {}

    for (uint8_t i = 0; i < NUM_KEYS; i++) {
        calib[i].rest_value       = analog_read(i);
        calib[i].bottom_out_value = calib[i].rest_value + INITIAL_BOTTOM_OUT_THRESHOLD;
    }

    run_boot_calibration();
    calibration_save();
}

void calibration_update(uint8_t key, uint16_t adc_filtered)
{
    if (adc_filtered > calib[key].bottom_out_value + CALIBRATION_EPSILON)
        calib[key].bottom_out_value = adc_filtered;
}

bool calibration_save(void)
{
    for (uint8_t i = 0; i < NUM_KEYS; i++) {
        eeconfig_ram.rest_value[i]       = calib[i].rest_value;
        eeconfig_ram.bottom_out_value[i] = calib[i].bottom_out_value;
    }
    eeconfig_ram.calibrated = 1;
    eeconfig_save();
    return true;
}

bool calibration_load(void)
{
    if (!eeconfig_ram.calibrated) return false;
    for (uint8_t i = 0; i < NUM_KEYS; i++) {
        calib[i].rest_value       = eeconfig_ram.rest_value[i];
        calib[i].bottom_out_value = eeconfig_ram.bottom_out_value[i];
    }
    return true;
}
