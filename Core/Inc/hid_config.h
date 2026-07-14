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
 *   0x09  RECALIBRATE     — call calibration_recalibrate()
 *
 * GET responses stream N packets with seq 0..N-1 (last zero-padded); the host
 * knows N from the fixed payload size. SET/RESET/RECALIBRATE complete with a
 * single terminal ACK report [cmd, 0xFF, status] (status 0 = ok) once the
 * (blocking) eeconfig_save()/recalibration has finished. Payload byte layouts:
 *   actuation  key_actuation_t[NUM_KEYS]                 (244 B)
 *   keymap     uint16_t[NUM_LAYERS][NUM_KEYS], LE        (488 B)
 *   status     uint8_t distance[NUM_KEYS] + is_pressed bitmap (61 + 8 = 69 B)
 *   calib      uint16_t rest[NUM_KEYS] then bottom[NUM_KEYS], LE (244 B)
 *   flags      uint8_t rt_enabled, uint8_t socd_enabled  (2 B) */

#include <stdint.h>

void hid_config_init(void);
void hid_config_task(void);

/* Feed one 64-byte OUT report from the config interface (called by usb.c's
 * tud_hid_set_report_cb when instance == USB_HID_CONFIG). */
void hid_config_rx(const uint8_t *report, uint16_t len);

#endif /* HID_CONFIG_H */
