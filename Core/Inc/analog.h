#ifndef ANALOG_H
#define ANALOG_H

#include "common.h"

/* Set by TIM2 ISR at 4kHz. Cleared by analog_task() after each scan.
 * 4 scans per 1ms USB frame, evenly spaced, partially cancels 1kHz
 * periodic noise from USB SOF activity on VDDA. */
extern volatile uint8_t scan_ready;

void     analog_init(void);
bool     analog_task(void);      /* full MUX scan if scan_ready; returns true on completion */
uint16_t analog_read(uint8_t key);

#endif /* ANALOG_H */
