#ifndef KEYBOARD_H
#define KEYBOARD_H

/* Top-level keyboard coordinator.
 *
 * keyboard_init()  Call once from main() after HAL init.
 *                  Starts TIM2 at 4kHz, loads or runs boot calibration,
 *                  and seeds the matrix filter.
 *
 * keyboard_task()  Call in the main loop on every iteration.
 *                  Processes one scan when available (every 250µs),
 *                  then updates key states and drives HID output. */

void keyboard_init(void);
void keyboard_task(void);

#endif /* KEYBOARD_H */
