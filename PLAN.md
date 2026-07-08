# TUNA60 HE — Remaining Work Plan

Status of the firmware as of 2026-07-02.

**Done:** analog acquisition (9× mux → ADC at 4kHz), EMA filtering, ADC→distance
conversion, boot + runtime calibration, rapid-trigger state machine, layered
keymap, flash-backed config store (`eeconfig`).

**Not done:** everything between `key_matrix[].is_pressed` and the USB cable.
The board scans keys but cannot type or be configured yet.

## Dependency order

```
Phase 1  rt_enabled gate      (independent, 1 line — do first)
Phase 2  usb.c                (transport — unlocks everything below)
Phase 3  hid.c + hid_keyboard.c  (typing)
Phase 4  socd.c               (game filter, plugs into phase 3)
Phase 5  hid_config.c         (web configurator protocol)
Backlog  distance LUT, extras
```

Nothing in phases 3–5 can be tested until phase 2 enumerates.

---

## Phase 1 — Gate rapid trigger on `rt_enabled`

`Core/Src/matrix.c:42` — TODO in code. RT currently activates for any key with
`rt_down != 0`, ignoring the global `eeconfig_ram.rt_enabled` flag that
`eeconfig.h` documents as the master switch.

- [ ] Change the branch condition in `matrix_scan()`:
      `if (!eeconfig_ram.rt_enabled || act->rt_down == 0)`
- [ ] Add explicit `#include "eeconfig.h"` to matrix.c
- [ ] Remove the two TODO comments in matrix.c (lines 42, 52)
- [ ] Check the RT-off transition: keys mid-`KEY_DIR_DOWN/UP` when the flag is
      cleared at runtime must not stick. The standard branch setting
      `dir = KEY_DIR_INACTIVE` and re-deriving `is_pressed` from the threshold
      on the next scan should cover it — verify once phase 5 can toggle the flag live.

**Milestone:** compiles; RT behavior unchanged when `rt_enabled == 1`.

---

## Phase 2 — USB stack (`Core/Src/usb.c`)

Spec: `Core/Inc/usb.h`. TinyUSB device stack on OTG_FS (PA11/PA12, 48MHz from
PLLQ — clock and pins already configured by CubeMX in `stm32f4xx_hal_msp.c`).
Composite device, two HID interfaces:

| Interface | Purpose | Reports |
|---|---|---|
| 0 | Boot keyboard (subclass 1, protocol 1) | 8-byte IN |
| 1 | Vendor/raw HID for web configurator | 64-byte IN/OUT |

- [x] Vendor TinyUSB source into `Middlewares/tinyusb/` — 0.21.0, pruned to
      device core + HID class + Synopsys DWC2 port (~1MB)
- [x] Add TinyUSB sources + include paths to `CMakeLists.txt` — also added ALL
      application sources (Core/Src/*.c), which were missing from the CMake
      build entirely (`dcd_dwc2.c` + `dwc2_common.c` is the F4 OTG_FS port)
- [x] Write `tusb_config.h` (Core/Inc): `CFG_TUSB_MCU = OPT_MCU_STM32F4`,
      `CFG_TUD_ENABLED = 1`, `CFG_TUD_HID = 2`, FS speed
- [x] Write `usb_descriptors.c` (Core/Src):
  - [x] Device descriptor — dev VID/PID 0xCAFE:0x6060, replace before release
  - [x] Config descriptor with both HID interfaces (kbd EP 0x81; vendor 0x02/0x82)
  - [x] HID report descriptor IF0: `TUD_HID_REPORT_DESC_KEYBOARD`
  - [x] HID report descriptor IF1: `TUD_HID_REPORT_DESC_GENERIC_INOUT(64)`
  - [x] String descriptors — serial derived from the 96-bit MCU UID
- [x] Interrupt wiring in `stm32f4xx_it.c`: `OTG_FS_IRQHandler` →
      `tud_int_handler(0)`, priority 1 (below TIM2). No HAL conflict:
      CubeMX never enabled the OTG_FS NVIC line or generated a handler.
      Kept `MX_USB_OTG_FS_PCD_Init()` — it pulls in the MSP clock/GPIO setup;
      TinyUSB's dcd soft-resets and takes over the core afterwards.
- [x] Implement `usb_init()` → `tusb_rhport_init(0, …)` (explicit 0.21 API);
      `usb_task()` → `tud_task()`
- [x] Call `usb_task()` from `main()`'s `while(1)` before `keyboard_task()`
- [x] Stub the required TinyUSB callbacks — `tud_hid_get_report_cb` /
      `tud_hid_set_report_cb` in usb.c (route by instance in phases 3/5)
- [x] `hid_init()`/`hid_task()` coordinator implemented (pulled forward from
      phase 3 — it is the init path that calls `usb_init()`)

> Note: linker `FLASH LENGTH` stays at the full 512K (team decision). There is
> a latent risk that if firmware ever grows past 0x08060000 (384K), code lands
> in eeconfig's sector 7, which `eeconfig_save()` erases at runtime. Current
> image is 43.6K so this is far off — tracked in Backlog, not enforced by the
> linker.

**Milestone:** board enumerates; Device Manager / `lsusb` shows one device
with two HID interfaces. No typing yet.
*Status: compiles & links clean (arm-gcc 15.2). Enumeration still to be
verified on hardware — flash `build/Debug/tuna_firmware.elf` and check
Device Manager.*

---

## Phase 3 — Boot keyboard (`Core/Src/hid_keyboard.c` + `Core/Src/hid.c`)

Spec: `Core/Inc/hid_keyboard.h` (two-pass design), `Core/Inc/hid.h`.

`hid.c` (trivial coordinator):
- [ ] `hid_init()` → `usb_init(); hid_keyboard_init(); hid_config_init();`
- [ ] `hid_task()` → `hid_keyboard_task(); hid_config_task();`

`hid_keyboard.c` — `hid_keyboard_task()` runs after `matrix_scan()` each tick:

Pass 1 — layer keys (before keycode resolution so `layer_state` is current):
- [ ] Track previous `is_pressed` per key to detect edges
- [ ] `KC_MO(n)`: `keymap_layer_on(n)` on press edge, `_off(n)` on release edge
- [ ] `KC_TG(n)`: `keymap_layer_toggle(n)` on release edge only (per header)
- [ ] Resolve layer keys from a snapshot of `layer_state` taken at pass start
      (avoids a momentary layer hiding its own MO key mid-pass → stuck layer)

Pass 2 — build the 8-byte boot report:
- [ ] For each pressed key: `keymap_get_keycode()`; skip `KC_NONE` and layer keys
- [ ] `KC_IS_MODIFIER(kc)` → set bit `(kc - KC_LCTL)` in modifier byte
- [ ] Else append to 6-slot keycode array
- [ ] Overflow (>6 keys): fill all six slots with 0x01 ErrorRollOver per boot
      protocol spec
- [ ] If `eeconfig_ram.socd_enabled`: call `socd_resolve(keys, &count)`
      (no-op stub until phase 4)
- [ ] Compare against previously sent report; send via
      `tud_hid_n_keyboard_report()` only on change, guarded by `tud_hid_n_ready()`

**Milestone:** board types. QWERTY correct, Fn+arrows/F-keys work, 6 simultaneous
keys register, no stuck keys when holding/releasing Fn mid-press.

---

## Phase 4 — SOCD resolution (`Core/Src/socd.c`)

Spec: `Core/Inc/socd.h`. Neutral mode only: when both keycodes of an opposing
pair are in the outgoing report, remove both. Runs on the final resolved
keycodes, so remapped layouts still work.

- [ ] Early-return when `!eeconfig_ram.socd_enabled`
- [ ] Pairs table: `LEFT/RGHT`, `UP/DOWN`, `A/D`, `W/S`
- [ ] For each pair present on both sides: remove both entries from
      `keycodes[]` (shift tail down), decrement `*count` per removal
- [ ] Confirm call site ordering in hid_keyboard.c: after report build,
      before dedup/send

**Milestone:** SOCD on → holding A+D outputs neither; releasing one resumes
the other within one scan.

---

## Phase 5 — Configurator protocol (`Core/Src/hid_config.c`)

Spec: `Core/Inc/hid_config.h`. Vendor HID on interface 1, WebHID from the
browser. 64-byte reports: byte 0 = report ID, byte 1 = sequence number for
payloads > 63 bytes.

- [ ] RX: route `tud_hid_set_report_cb()` (interface 1) on `buf[0]`
- [ ] TX: chunked send queue — split payload into ≤62-byte chunks, stamp
      sequence in byte 1, drain from `hid_config_task()` when `tud_hid_n_ready()`
- [ ] Multi-packet RX reassembly with sequence validation (needed for SET
      keymap: 4×61×2 = 488 bytes ≈ 8 packets)

Report handlers:
- [ ] `0x01` GET keymap — stream `eeconfig_ram.keymap`
- [ ] `0x02` SET keymap — reassemble → write → `eeconfig_save()`
- [ ] `0x03` GET actuation — stream `actuation_map[]`
- [ ] `0x04` SET actuation — reassemble → write → `eeconfig_save()`
- [ ] `0x05` GET status — stream live `key_matrix[]` distance + is_pressed
      (**implement first** — best bring-up/debug tool, needs no SET path)
- [ ] `0x06` GET calib — stream `calib[]` rest/bottom-out
- [ ] `0x07` SET flags — `rt_enabled`, `socd_enabled` → `eeconfig_save()`
- [ ] `0x08` RESET — `eeconfig_reset()`
- [ ] `0x09` RECALIBRATE — `calibration_recalibrate()`

Constraints:
- [ ] `eeconfig_save()` blocks 1–2s (sector erase) and stalls scanning + USB.
      Save exactly once per SET transaction (after the final packet), never
      per packet. Expect the host to tolerate the stall — document it in the
      protocol notes for the web UI.
- [ ] SET flags turning `rt_enabled` off: verify no stuck keys (phase 1 item)

**Milestone:** browser test page (WebHID) can read live key distances, write a
keymap change that survives replug, trigger recalibration.

---

## Backlog (post-MVP, no code TODOs yet)

- **Logarithmic distance LUT** — `Core/Inc/distance.h` notes the OH49E curve
  is slightly log; linear is fine for thresholds but limits sub-0.1mm RT
  sensitivity. Approach: `libhmk` `tools/distance_lut.py` curve fit.
- **SOCD modes beyond neutral** — last-input-priority needs per-pair
  press-order state; also per-pair configurability over report `0x07`+.
- **`eeconfig` wear** — sector 7 erase per save; fine for a configurator's
  write frequency, revisit only if saves become frequent.
- **Sector 7 code-placement risk** — `eeconfig` lives in flash sector 7
  (0x08060000). The linker still hands code the full 512K, so if the image ever
  grows past 384K it will overlap sector 7 and be corrupted by `eeconfig_save()`.
  Options if it ever gets close: cap linker `FLASH LENGTH` to 384K, or move
  eeconfig to a different sector. Not a concern at the current 43.6K.
- **Suspend/remote wakeup** — USB suspend handling (`tud_suspend_cb`) and
  wake-on-keypress once the basics work.

## Reference

- Header specs are authoritative: `usb.h`, `hid.h`, `hid_keyboard.h`,
  `hid_config.h`, `socd.h` each document their module's contract.
- Similar working design (same sensor + TinyUSB + WebHID configurator):
  https://github.com/peppapighs/libhmk
