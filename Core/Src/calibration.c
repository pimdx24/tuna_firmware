#include "calibration.h"
#include "analog.h"
#include "eeconfig.h"
#include "stm32f4xx_hal.h"

key_calib_t calib[NUM_KEYS];

_Static_assert(CALIBRATION_SETTLE_MS < CALIBRATION_DURATION_MS,
               "settle window must leave time to track the minimum");

/* Measure rest_value for every key (all keys assumed at rest) and seed
 * bottom_out_value. Requires TIM2 running (analog_task needs scan_ready). */
static void run_boot_calibration(void)
{
    /* Same 12.4 fixed-point EMA as matrix.c (acc = 16 × filtered) so the
     * rest value is measured with exactly the filter used at runtime. */
    uint32_t acc[NUM_KEYS];
    for (uint8_t i = 0; i < NUM_KEYS; i++)
    {
        acc[i] = (uint32_t)analog_read(i) << 4;
        calib[i].rest_value = ADC_MAX;   /* min-tracked below, after settling */
    }

    uint32_t start = HAL_GetTick();
    while (HAL_GetTick() - start < CALIBRATION_DURATION_MS)
    {
        if (!analog_task()) continue;
        bool settled = (HAL_GetTick() - start) >= CALIBRATION_SETTLE_MS;
        for (uint8_t i = 0; i < NUM_KEYS; i++)
        {
            acc[i] = acc[i] + analog_read(i) - (acc[i] >> 4);
            uint16_t filtered = (uint16_t)(acc[i] >> 4);
            if (settled && filtered < calib[i].rest_value)
                calib[i].rest_value = filtered;
        }
    }

    for (uint8_t i = 0; i < NUM_KEYS; i++)
        calib[i].bottom_out_value = calib[i].rest_value + INITIAL_BOTTOM_OUT_THRESHOLD;
}

void calibration_init(void)
{
    /* Always do at least one scan before returning so analog_read() gives
     * real values when matrix_init() seeds adc_filtered immediately after. */
    while (!analog_task()) {}

    if (calibration_load()) return;

    run_boot_calibration();
    calibration_save();
}

void calibration_recalibrate(void)
{
    while (!analog_task()) {}

    run_boot_calibration();
    calibration_save();
}

void calibration_update(uint8_t key, uint16_t adc_filtered)
{
    if (adc_filtered > calib[key].bottom_out_value + CALIBRATION_EPSILON)
        calib[key].bottom_out_value = adc_filtered;
}

void calibration_save(void)
{
    for (uint8_t i = 0; i < NUM_KEYS; i++)
    {
        eeconfig_ram.rest_value[i] = calib[i].rest_value;
        eeconfig_ram.bottom_out_value[i] = calib[i].bottom_out_value;
    }
    eeconfig_ram.calibrated = 1;
    eeconfig_save();
}

bool calibration_load(void)
{
    if (!eeconfig_ram.calibrated) return false;
    for (uint8_t i = 0; i < NUM_KEYS; i++)
    {
        calib[i].rest_value = eeconfig_ram.rest_value[i];
        calib[i].bottom_out_value = eeconfig_ram.bottom_out_value[i];
    }
    return true;
}
