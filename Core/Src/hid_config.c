#include "hid_config.h"
#include "eeconfig.h"
#include "calibration.h"
#include "matrix.h"
#include "usb.h"
#include "tusb.h"
#include <string.h>

/* Web configurator HID interface (Interface 1). Protocol: see hid_config.h.
 *
 * 64-byte reports, byte 0 = command, byte 1 = sequence. Multi-packet payloads
 * carry 62 bytes/packet. Payload byte layouts mirror the eeconfig_t / calib[]
 * memory so most GET/SET handlers are a straight memcpy (STM32F4 is little-
 * endian, matching the host's little-endian encode/decode).
 *
 * Blocking work (eeconfig_save erases a flash sector, ~1-2s; recalibration)
 * runs from hid_config_task() in the main loop, never in the RX callback. */

#define REPORT_SIZE 64
#define HEADER      2
#define CHUNK       (REPORT_SIZE - HEADER) /* 62 payload bytes per packet */

#define SZ_ACTUATION (NUM_KEYS * 4)                    /* 244 */
#define SZ_KEYMAP    (NUM_LAYERS * NUM_KEYS * 2)       /* 488 */
#define SZ_STATUS    (NUM_KEYS + ((NUM_KEYS + 7) / 8)) /* 61 + 8 = 69 */
#define SZ_CALIB     (NUM_KEYS * 2 * 2)                /* 244 */
#define MAX_PAYLOAD  SZ_KEYMAP

#define SEQ_ACK 0xFFU

enum {
    CMD_GET_KEYMAP    = 0x01,
    CMD_SET_KEYMAP    = 0x02,
    CMD_GET_ACTUATION = 0x03,
    CMD_SET_ACTUATION = 0x04,
    CMD_GET_STATUS    = 0x05,
    CMD_GET_CALIB     = 0x06,
    CMD_SET_FLAGS     = 0x07,
    CMD_RESET         = 0x08,
    CMD_RECALIBRATE   = 0x09,
};

/* Deferred blocking action, run from the task rather than the RX callback. */
typedef enum { ACT_NONE, ACT_SAVE, ACT_RESET, ACT_RECALIB } pending_action_t;

/* Outgoing (device -> host) multi-packet response. */
static uint8_t  tx_buf[MAX_PAYLOAD];
static uint16_t tx_len;  /* payload bytes queued; 0 = idle */
static uint16_t tx_sent; /* payload bytes already sent */
static uint8_t  tx_cmd;

/* Incoming (host -> device) reassembly for SET commands. */
static uint8_t  rx_buf[MAX_PAYLOAD];
static uint16_t rx_expected; /* total payload bytes; 0 = not receiving */
static uint16_t rx_got;
static uint8_t  rx_cmd;

/* Deferred action + terminal ACK. */
static pending_action_t pending;
static bool    ack_pending;
static uint8_t ack_cmd;

void hid_config_init(void)
{
    tx_len = 0;
    tx_sent = 0;
    rx_expected = 0;
    rx_got = 0;
    pending = ACT_NONE;
    ack_pending = false;
}

/* Queue a payload to stream back to the host. */
static void tx_start(uint8_t cmd, const uint8_t *data, uint16_t len)
{
    if (len > MAX_PAYLOAD) len = MAX_PAYLOAD;
    memcpy(tx_buf, data, len);
    tx_cmd = cmd;
    tx_len = len;
    tx_sent = 0;
}

/* GET status (0x05): 61 distance bytes then an 8-byte is_pressed bitmap. */
static void tx_start_status(void)
{
    uint8_t buf[SZ_STATUS];
    memset(buf, 0, sizeof(buf));
    for (uint8_t i = 0; i < NUM_KEYS; i++)
        buf[i] = key_matrix[i].distance;
    for (uint8_t i = 0; i < NUM_KEYS; i++)
        if (key_matrix[i].is_pressed)
            buf[NUM_KEYS + (i >> 3)] |= (uint8_t)(1U << (i & 7));
    tx_start(CMD_GET_STATUS, buf, SZ_STATUS);
}

/* GET calib (0x06): rest_value[61] then bottom_out_value[61], little-endian u16. */
static void tx_start_calib(void)
{
    uint8_t buf[SZ_CALIB];
    for (uint8_t i = 0; i < NUM_KEYS; i++) {
        buf[i * 2]     = (uint8_t)(calib[i].rest_value & 0xFF);
        buf[i * 2 + 1] = (uint8_t)(calib[i].rest_value >> 8);
    }
    uint16_t base = NUM_KEYS * 2;
    for (uint8_t i = 0; i < NUM_KEYS; i++) {
        buf[base + i * 2]     = (uint8_t)(calib[i].bottom_out_value & 0xFF);
        buf[base + i * 2 + 1] = (uint8_t)(calib[i].bottom_out_value >> 8);
    }
    tx_start(CMD_GET_CALIB, buf, SZ_CALIB);
}

/* Payload byte count for a multi-packet SET, or 0 if cmd is not one. */
static uint16_t set_payload_size(uint8_t cmd)
{
    switch (cmd) {
    case CMD_SET_ACTUATION: return SZ_ACTUATION;
    case CMD_SET_KEYMAP:    return SZ_KEYMAP;
    default:                return 0;
    }
}

/* Commit a completed SET payload to eeconfig_ram and schedule the flash save. */
static void apply_set(uint8_t cmd, const uint8_t *payload)
{
    if (cmd == CMD_SET_ACTUATION)
        memcpy(eeconfig_ram.actuation_map, payload, SZ_ACTUATION);
    else if (cmd == CMD_SET_KEYMAP)
        memcpy(eeconfig_ram.keymap, payload, SZ_KEYMAP);
    pending = ACT_SAVE;
    ack_cmd = cmd;
    ack_pending = true;
}

/* Called from tud_hid_set_report_cb for the config interface (instance 1). */
void hid_config_rx(const uint8_t *report, uint16_t len)
{
    if (len < HEADER) return;
    uint8_t cmd = report[0];
    uint8_t seq = report[1];

    uint16_t set_sz = set_payload_size(cmd);
    if (set_sz) {
        /* Multi-packet SET: (re)start on seq 0, accumulate the ordered chunks. */
        if (seq == 0) {
            rx_cmd = cmd;
            rx_expected = set_sz;
            rx_got = 0;
        }
        if (rx_expected == 0 || cmd != rx_cmd) return; /* stray / out of sync */
        uint16_t off = (uint16_t)seq * CHUNK;
        if (off >= rx_expected) return;
        uint16_t n = rx_expected - off;
        if (n > CHUNK) n = CHUNK;
        if ((uint16_t)(len - HEADER) < n) n = (uint16_t)(len - HEADER);
        memcpy(rx_buf + off, report + HEADER, n);
        rx_got = off + n;
        if (rx_got >= rx_expected) {
            apply_set(rx_cmd, rx_buf);
            rx_expected = 0;
        }
        return;
    }

    switch (cmd) {
    case CMD_GET_KEYMAP:
        tx_start(CMD_GET_KEYMAP, (const uint8_t *)eeconfig_ram.keymap, SZ_KEYMAP);
        break;
    case CMD_GET_ACTUATION:
        tx_start(CMD_GET_ACTUATION, (const uint8_t *)eeconfig_ram.actuation_map, SZ_ACTUATION);
        break;
    case CMD_GET_STATUS:
        tx_start_status();
        break;
    case CMD_GET_CALIB:
        tx_start_calib();
        break;
    case CMD_SET_FLAGS:
        if (len >= HEADER + 2) {
            eeconfig_ram.rt_enabled   = report[HEADER + 0] ? 1 : 0;
            eeconfig_ram.socd_enabled = report[HEADER + 1] ? 1 : 0;
            pending = ACT_SAVE;
            ack_cmd = cmd;
            ack_pending = true;
        }
        break;
    case CMD_RESET:
        pending = ACT_RESET;
        ack_cmd = cmd;
        ack_pending = true;
        break;
    case CMD_RECALIBRATE:
        pending = ACT_RECALIB;
        ack_cmd = cmd;
        ack_pending = true;
        break;
    default:
        break;
    }
}

void hid_config_task(void)
{
    /* 1. Run any deferred blocking action (flash erase / recalibration).
     *    Done here, in main-loop context, so the RX callback stays fast. */
    if (pending != ACT_NONE) {
        switch (pending) {
        case ACT_SAVE:    eeconfig_save(); break;             /* erase+write, ~1-2s */
        case ACT_RESET:   eeconfig_reset(); break;            /* restores defaults + saves */
        case ACT_RECALIB: calibration_recalibrate(); break;   /* recalibrates + saves */
        default: break;
        }
        pending = ACT_NONE;
    }

    if (!tud_hid_n_ready(USB_HID_CONFIG)) return;

    /* 2. Terminal ACK for the last SET/RESET/RECALIBRATE. */
    if (ack_pending) {
        uint8_t pkt[REPORT_SIZE];
        memset(pkt, 0, sizeof(pkt));
        pkt[0] = ack_cmd;
        pkt[1] = SEQ_ACK;
        pkt[2] = 0; /* status: ok */
        if (tud_hid_n_report(USB_HID_CONFIG, 0, pkt, REPORT_SIZE))
            ack_pending = false;
        return;
    }

    /* 3. Stream the next chunk of an in-progress GET response. */
    if (tx_len > 0) {
        uint8_t pkt[REPORT_SIZE];
        memset(pkt, 0, sizeof(pkt));
        pkt[0] = tx_cmd;
        pkt[1] = (uint8_t)(tx_sent / CHUNK);
        uint16_t n = tx_len - tx_sent;
        if (n > CHUNK) n = CHUNK;
        memcpy(pkt + HEADER, tx_buf + tx_sent, n);
        if (tud_hid_n_report(USB_HID_CONFIG, 0, pkt, REPORT_SIZE)) {
            tx_sent += n;
            if (tx_sent >= tx_len) {
                tx_len = 0;
                tx_sent = 0;
            }
        }
    }
}
