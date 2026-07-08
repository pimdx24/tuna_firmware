#include "hid.h"
#include "usb.h"
#include "hid_keyboard.h"
#include "hid_config.h"

void hid_init(void)
{
    usb_init();
    hid_keyboard_init();
    hid_config_init();
}

void hid_task(void)
{
    hid_keyboard_task();
    hid_config_task();
}
