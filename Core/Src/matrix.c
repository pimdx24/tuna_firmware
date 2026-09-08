#include "matrix.h"
#include "analog.h"
#include "calibration.h"
#include "distance.h"
#include "eeconfig.h"
#include "keymap.h"

/* EMA filter, alpha = 1/16, in 12.4 fixed point.
 *   acc += raw - acc/16        (acc holds 16 × filtered)
 *   filtered = acc >> 4
 * Time constant = 15 samples = 3.75ms at 4kHz.
 * 4 evenly-spaced samples per 1ms USB frame partially cancel 1kHz USB noise.
 * The residue kept in the low four bits of acc means a change of any size is
 * eventually tracked; the earlier (raw + 15*old) >> 4 form could never rise
 * for steps under 16 counts and settled up to 15 counts low. */
#define EMA_SHIFT 4U

static inline uint16_t ema_update(uint32_t *acc, uint16_t raw)
{
    *acc = *acc + raw - (*acc >> EMA_SHIFT);   /* bounded: acc <= 16 * ADC_MAX */
    return (uint16_t)(*acc >> EMA_SHIFT);
}

key_state_t key_matrix[NUM_KEYS];

void matrix_init(void)
{
    for (uint8_t i = 0; i < NUM_KEYS; i++)
    {
        uint16_t raw = analog_read(i);
        key_matrix[i].adc_acc = (uint32_t)raw << EMA_SHIFT;
        key_matrix[i].adc_filtered = raw;
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
        uint16_t filtered = ema_update(&key_matrix[i].adc_acc, analog_read(i));
        key_matrix[i].adc_filtered = filtered;

        calibration_update(i, filtered);

        uint8_t dist = adc_to_distance(filtered,
                                        calib[i].rest_value,
                                        calib[i].bottom_out_value);
        key_matrix[i].distance = dist;

        const key_actuation_t *act = keymap_get_actuation(i);

        if (!eeconfig_ram.rt_enabled || act->rt_down == 0)
        {
            /* Standard mode: simple threshold. Also the path taken when the
             * global rapid trigger switch is off, whatever the per-key config. */
            key_matrix[i].is_pressed = (dist >= act->actuation_point);
            key_matrix[i].dir = KEY_DIR_INACTIVE;
        }
        else
        {
            /* Rapid trigger: fire on configurable delta from directional extremum.
             *
             * Armed at dist >= actuation_point — the same boundary as standard
             * mode. Reset (disarm, release) when the key drops back *below* the
             * actuation point; the strict < pairs with the >= arm so a key
             * resting exactly on the threshold cannot toggle every scan.
             * continuous flag (bit0): key must return fully to rest (dist == 0)
             * to reset; below the actuation point it stays armed and can re-fire. */
            bool reset = (act->flags & 0x01) ? (dist == 0)
                                             : (dist < act->actuation_point);
            uint8_t rt_up = act->rt_up ? act->rt_up : act->rt_down;

            switch (key_matrix[i].dir)
            {
            case KEY_DIR_INACTIVE:
                if (dist >= act->actuation_point)
                {
                    key_matrix[i].extremum = dist;
                    key_matrix[i].dir = KEY_DIR_DOWN;
                    key_matrix[i].is_pressed = true;
                }
                break;

            case KEY_DIR_DOWN:
                if (reset)
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
                if (reset)
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
