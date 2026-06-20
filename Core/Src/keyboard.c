#include "keyboard.h"
#include "analog.h"
#include "calibration.h"
#include "eeconfig.h"
#include "hid.h"
#include "keymap.h"
#include "matrix.h"
#include "stm32f4xx_hal.h"

extern TIM_HandleTypeDef htim2;

void keyboard_init(void)
{
    analog_init();

    /* Configure TIM2 for 4kHz: 48MHz timer clock / (48 × 250) = 4000Hz.
     * Override CubeMX defaults here so this survives regeneration.
     * PSC=47 → div48; ARR=249 → div250; 48MHz/(48×250)=4000Hz. */
    htim2.Init.Prescaler = 47;
    htim2.Init.Period = 249;
    HAL_TIM_Base_Init(&htim2);
    HAL_TIM_Base_Start_IT(&htim2);

    eeconfig_init();
    calibration_init();   /* blocks ~500ms on first boot for boot calibration */
    keymap_init();
    matrix_init();
    hid_init();
}

void keyboard_task(void)
{
    if (!analog_task()) return;
    matrix_scan();
    hid_task();
}
