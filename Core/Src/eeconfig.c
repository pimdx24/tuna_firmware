#include "eeconfig.h"
#include "stm32f4xx_hal.h"

#define FLASH_SECTOR FLASH_SECTOR_7
#define FLASH_ADDR   0x08060000U

eeconfig_t eeconfig_ram;

static void set_defaults(void)
{
    eeconfig_ram.magic      = EECONFIG_MAGIC;
    eeconfig_ram.version    = EECONFIG_VERSION;
    eeconfig_ram.calibrated = 0;

    for (uint8_t i = 0; i < NUM_KEYS; i++) {
        eeconfig_ram.rest_value[i]                    = 2048;
        eeconfig_ram.bottom_out_value[i]              = 2048 + 400;
        eeconfig_ram.actuation_map[i].actuation_point = DEFAULT_ACTUATION_POINT;
        eeconfig_ram.actuation_map[i].rt_down         = 0;
        eeconfig_ram.actuation_map[i].rt_up           = 0;
        eeconfig_ram.actuation_map[i].flags           = 0;
    }

    for (uint8_t l = 0; l < NUM_LAYERS; l++)
        for (uint8_t k = 0; k < NUM_KEYS; k++)
            eeconfig_ram.keymap[l][k] = 0xFFFF;  /* KC_TRANS */
}

void eeconfig_init(void)
{
    const eeconfig_t *flash = (const eeconfig_t *)FLASH_ADDR;
    if (flash->magic == EECONFIG_MAGIC && flash->version == EECONFIG_VERSION) {
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
    uint32_t error = 0;
    HAL_FLASHEx_Erase(&erase, &error);

    const uint32_t *src  = (const uint32_t *)&eeconfig_ram;
    uint32_t        addr = FLASH_ADDR;
    for (uint32_t i = 0; i < sizeof(eeconfig_t) / 4; i++) {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, src[i]);
        addr += 4;
    }

    HAL_FLASH_Lock();
}

void eeconfig_reset(void)
{
    set_defaults();
    eeconfig_save();
}
