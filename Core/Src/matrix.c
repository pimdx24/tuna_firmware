#include "matrix.h"
#include "analog.h"
#include "calibration.h"
#include "distance.h"
#include "keymap.h"

/* EMA filter: new = (raw + old*15) >> 4  (alpha=1/16)
 * Time constant = 15 * 0.25ms = 3.75ms at 4kHz.
 * 4 evenly-spaced samples per 1ms USB frame partially cancel 1kHz USB noise. */
#define EMA(raw, old) ((uint16_t)(((uint32_t)(raw) + ((uint32_t)(old) * 15U)) >> 4))

key_state_t key_matrix[NUM_KEYS];

void matrix_init(void)
{
    for (uint8_t i = 0; i < NUM_KEYS; i++)
    {
        key_matrix[i].adc_filtered = analog_read(i);
        key_matrix[i].distance = 0;
        key_matrix[i].extremum = 0;
        key_matrix[i].dir = KEY_DIR_INACTIVE;
        key_matrix[i].is_pressed = false;
    }
}

void matrix_scan(void)
{
    for (uint8_t i = 0; i < NUM_KEYS; i++)
    {
        uint16_t filtered = EMA(analog_read(i), key_matrix[i].adc_filtered);
        key_matrix[i].adc_filtered = filtered;

        calibration_update(i, filtered);

        uint8_t dist = adc_to_distance(filtered,
                                        calib[i].rest_value,
                                        calib[i].bottom_out_value);
        key_matrix[i].distance = dist;

        const key_actuation_t *act = keymap_get_actuation(i);

        /* TODO: gate RT on eeconfig_ram.rt_enabled — when 0, always take standard branch. */
        if (act->rt_down == 0)
        {
            /* Standard mode: simple threshold */
            key_matrix[i].is_pressed = (dist >= act->actuation_point);
            key_matrix[i].dir = KEY_DIR_INACTIVE;
        }
        else
        {
            /* Rapid trigger: fire on configurable delta from directional extremum.
             * Only reached when eeconfig_ram.rt_enabled == 1 (see gate TODO below).
             * continuous flag (bit0): key must return to rest (dist=0) to reset;
             *                        when clear, returning past actuation_point resets. */
            uint8_t reset_point = (act->flags & 0x01) ? 0 : act->actuation_point;
            uint8_t rt_up = act->rt_up ? act->rt_up : act->rt_down;

            switch (key_matrix[i].dir)
            {
            case KEY_DIR_INACTIVE:
                if (dist > act->actuation_point)
                {
                    key_matrix[i].extremum = dist;
                    key_matrix[i].dir = KEY_DIR_DOWN;
                    key_matrix[i].is_pressed = true;
                }
                break;

            case KEY_DIR_DOWN:
                if (dist <= reset_point)
                {
                    key_matrix[i].extremum = dist;
                    key_matrix[i].dir = KEY_DIR_INACTIVE;
                    key_matrix[i].is_pressed = false;
                }
                else if (key_matrix[i].extremum >= rt_up &&
                         dist <= key_matrix[i].extremum - rt_up)
                {
                    key_matrix[i].extremum = dist;
                    key_matrix[i].dir = KEY_DIR_UP;
                    key_matrix[i].is_pressed = false;
                }
                else if (dist > key_matrix[i].extremum)
                    key_matrix[i].extremum = dist;
                break;

            case KEY_DIR_UP:
                if (dist <= reset_point)
                {
                    key_matrix[i].extremum = dist;
                    key_matrix[i].dir = KEY_DIR_INACTIVE;
                    key_matrix[i].is_pressed = false;
                }
                else if ((uint16_t)key_matrix[i].extremum + act->rt_down <= dist)
                {
                    key_matrix[i].extremum = dist;
                    key_matrix[i].dir = KEY_DIR_DOWN;
                    key_matrix[i].is_pressed = true;
                }
                else if (dist < key_matrix[i].extremum)
                    key_matrix[i].extremum = dist;
                break;
            }
        }
    }
}
