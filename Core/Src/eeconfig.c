#include "eeconfig.h"
#include "keymap.h"
#include "main.h"
#include "stm32f4xx_hal.h"

#define FLASH_SECTOR FLASH_SECTOR_7
#define FLASH_ADDR   0x08060000U

eeconfig_t eeconfig_ram;

/* Default keymaps:
 *   Layer 0: standard 60% ANSI QWERTY — the base keymap, normal actuation.
 *   Layer 1: Fn momentary overlay — F-keys, navigation, arrows.
 *   Layer 2: user-defined layer 2 (all KC_TRANS, configure via web UI).
 *   Layer 3: user-defined layer 3 (all KC_TRANS, configure via web UI).
 *
 * Key index = HV sensor number - 1. Layout numbered left-to-right, top-to-bottom:
 *   Row 0 (idx 0–13):  ESC 1-0 - = BKSP
 *   Row 1 (idx 14–27): TAB Q-P [ ] \
 *   Row 2 (idx 28–40): CAPS A-L ; ' ENTER
 *   Row 3 (idx 41–52): LSFT Z-/ RSFT
 *   Row 4 (idx 53–60): LCTL LGUI LALT SPC RALT APP RCTL FN */
static void set_default_keymap(void)
{
    static const uint16_t layer0[NUM_KEYS] = {
        /* Row 0  ESC    1      2      3      4      5      6      7      8      9      0      -       =       BKSP  */
        KC_ESC,  KC_1,  KC_2,  KC_3,  KC_4,  KC_5,  KC_6,  KC_7,  KC_8,  KC_9,  KC_0,  KC_MINS, KC_EQL, KC_BKSP,
        /* Row 1  TAB    Q      W      E      R      T      Y      U      I      O      P      [       ]       \     */
        KC_TAB,  KC_Q,  KC_W,  KC_E,  KC_R,  KC_T,  KC_Y,  KC_U,  KC_I,  KC_O,  KC_P,  KC_LBRC, KC_RBRC, KC_BSLS,
        /* Row 2  CAPS   A      S      D      F      G      H      J      K      L      ;      '       ENTER         */
        KC_CAPS, KC_A,  KC_S,  KC_D,  KC_F,  KC_G,  KC_H,  KC_J,  KC_K,  KC_L,  KC_SCLN, KC_QUOT, KC_ENTER,
        /* Row 3  LSFT   Z      X      C      V      B      N      M      ,      .      /      RSFT                  */
        KC_LSFT, KC_Z,  KC_X,  KC_C,  KC_V,  KC_B,  KC_N,  KC_M,  KC_COMM, KC_DOT, KC_SLSH, KC_RSFT,
        /* Row 4  LCTL   LGUI   LALT   SPC    RALT   APP    RCTL   FN                                               */
        KC_LCTL, KC_LGUI, KC_LALT, KC_SPC, KC_RALT, KC_APP, KC_RCTL, KC_MO(1),
    };
    static const uint16_t layer1[NUM_KEYS] = {
        /* Row 0  `      F1     F2     F3     F4     F5     F6     F7     F8     F9     F10    F11     F12     DEL  */
        KC_GRV,  KC_F1, KC_F2, KC_F3, KC_F4, KC_F5, KC_F6, KC_F7, KC_F8, KC_F9, KC_F10, KC_F11, KC_F12, KC_DEL,
        /* Row 1  _      INS    UP     HOME   PGUP   _      _      _      _      _      PSCR   SCRL    PAUS    _   */
        KC_TRANS, KC_INS, KC_UP, KC_HOME, KC_PGUP, KC_TRANS, KC_TRANS, KC_TRANS, KC_TRANS, KC_TRANS, KC_PSCR, KC_SCRL, KC_PAUS, KC_TRANS,
        /* Row 2  _      LEFT   DOWN   RGHT   END    _      _      _      _      _      _      _       _           */
        KC_TRANS, KC_LEFT, KC_DOWN, KC_RGHT, KC_END, KC_TRANS, KC_TRANS, KC_TRANS, KC_TRANS, KC_TRANS, KC_TRANS, KC_TRANS, KC_TRANS,
        /* Row 3  _      _      _      _      _      _      _      _      _      PGDN   _      _                   */
        KC_TRANS, KC_TRANS, KC_TRANS, KC_TRANS, KC_TRANS, KC_TRANS, KC_TRANS, KC_TRANS, KC_TRANS, KC_PGDN, KC_TRANS, KC_TRANS,
        /* Row 4  all transparent */
        KC_TRANS, KC_TRANS, KC_TRANS, KC_TRANS, KC_TRANS, KC_TRANS, KC_TRANS, KC_TRANS,
    };

    for (uint8_t k = 0; k < NUM_KEYS; k++)
    {
        eeconfig_ram.keymap[0][k] = layer0[k];
        eeconfig_ram.keymap[1][k] = layer1[k];
        eeconfig_ram.keymap[2][k] = KC_TRANS;
        eeconfig_ram.keymap[3][k] = KC_TRANS;
    }
}

static void set_defaults(void)
{
    eeconfig_ram.magic = EECONFIG_MAGIC;
    eeconfig_ram.version = EECONFIG_VERSION;
    eeconfig_ram.calibrated = 0;
    eeconfig_ram.rt_enabled = 0;
    eeconfig_ram.socd_enabled = 0;

    for (uint8_t i = 0; i < NUM_KEYS; i++)
    {
        eeconfig_ram.rest_value[i] = 2048;
        eeconfig_ram.bottom_out_value[i] = 2048 + 400;
        eeconfig_ram.actuation_map[i].actuation_point = DEFAULT_ACTUATION_POINT;
        eeconfig_ram.actuation_map[i].rt_down = 0;
        eeconfig_ram.actuation_map[i].rt_up = 0;
        eeconfig_ram.actuation_map[i].flags = 0;
    }

    set_default_keymap();
}

void eeconfig_init(void)
{
    const eeconfig_t *flash = (const eeconfig_t *)FLASH_ADDR;
    if (flash->magic == EECONFIG_MAGIC && flash->version == EECONFIG_VERSION)
    {
        eeconfig_ram = *flash;
        return;
    }
    eeconfig_reset();
}

void eeconfig_save(void)
{
    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef erase = {
        .TypeErase    = FLASH_TYPEERASE_SECTORS,
        .VoltageRange = FLASH_VOLTAGE_RANGE_3,
        .Sector       = FLASH_SECTOR,
        .NbSectors    = 1,
    };
    uint32_t error = 0xFFFFFFFFU;
    if (HAL_FLASHEx_Erase(&erase, &error) != HAL_OK || error != 0xFFFFFFFFU)
    {
        HAL_FLASH_Lock();
        Error_Handler();
        return;
    }

    const uint32_t *src = (const uint32_t *)&eeconfig_ram;
    uint32_t addr = FLASH_ADDR;
    for (uint32_t i = 0; i < sizeof(eeconfig_t) / 4; i++)
    {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, src[i]) != HAL_OK)
        {
            HAL_FLASH_Lock();
            Error_Handler();
            return;
        }
        addr += 4;
    }

    HAL_FLASH_Lock();
}

void eeconfig_reset(void)
{
    set_defaults();
    eeconfig_save();
}
