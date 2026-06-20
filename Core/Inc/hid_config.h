#ifndef HID_CONFIG_H
#define HID_CONFIG_H

/* Vendor HID interface for the web configurator (Interface 1, WebHID API).
 *
 * 64-byte IN/OUT reports. Byte 0 = report ID, byte 1 = sequence (for multi-packet
 * transfers where payload exceeds 63 bytes).
 *
 * Report IDs:
 *   0x01  GET keymap      — stream eeconfig_ram.keymap[NUM_LAYERS][NUM_KEYS]
 *   0x02  SET keymap      — receive keymap, write eeconfig_ram, call eeconfig_save()
 *   0x03  GET actuation   — stream eeconfig_ram.actuation_map[NUM_KEYS]
 *   0x04  SET actuation   — receive actuation_map, write eeconfig_ram, call eeconfig_save()
 *   0x05  GET status      — stream key_matrix[] distances and is_pressed flags (live)
 *   0x06  GET calib       — stream calib[] rest_value and bottom_out_value per key
 *   0x07  SET flags       — set rt_enabled and socd_enabled, call eeconfig_save()
 *   0x08  RESET config    — call eeconfig_reset() (restores all defaults)
 *   0x09  RECALIBRATE     — call calibration_recalibrate() */

void hid_config_init(void);
void hid_config_task(void);

#endif /* HID_CONFIG_H */
