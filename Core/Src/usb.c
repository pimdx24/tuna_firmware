#include "usb.h"
#include "tusb.h"
#include "stm32f4xx_hal.h"

/* Descriptors live in usb_descriptors.c. The OTG_FS clocks and PA11/PA12
 * pins are set up by CubeMX (HAL_PCD_MspInit); TinyUSB's dcd soft-resets
 * the core and owns it from usb_init() on — the HAL PCD driver is unused
 * at runtime. */

void usb_init(void)
{
    /* Priority 1: below TIM2 (0) so the 4kHz scan tick stays jitter-free. */
    HAL_NVIC_SetPriority(OTG_FS_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(OTG_FS_IRQn);

    const tusb_rhport_init_t dev_init = {
        .role  = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_FULL,
    };
    tusb_rhport_init(0, &dev_init);
}

void usb_task(void)
{
    tud_task();
}

/* -------------------------------------------------------------------------
 * TinyUSB HID callbacks — shared by both interfaces, routed by instance.
 * ------------------------------------------------------------------------- */

/* Host reads a report via control transfer (GET_REPORT).
 * Rarely used by real hosts; returning 0 STALLs the request. */
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type,
                               uint8_t *buffer, uint16_t reqlen)
{
    (void)instance; (void)report_id; (void)report_type;
    (void)buffer; (void)reqlen;
    return 0;
}

/* Host sends a report.
 * USB_HID_KEYBOARD OUTPUT report = LED state (Caps Lock etc.) — phase 3.
 * USB_HID_CONFIG   OUT report    = configurator command      — phase 5,
 * route to hid_config. */
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const *buffer, uint16_t bufsize)
{
    (void)instance; (void)report_id; (void)report_type;
    (void)buffer; (void)bufsize;
}
