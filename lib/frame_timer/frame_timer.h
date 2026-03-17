#pragma once

/*
============================================================
frame_timer.h

High precision frame timing module using ESP32-S3 GPTimer.

This timer will become the master clock of the entire
rendering pipeline:

    TIMER -> GDMA Trigger -> LCD Parallel Clock -> HUB75

Responsibilities:
    • Configure hardware timer
    • Generate deterministic timing events
    • Provide optional debug test output (GPIO toggle)
    • Later: trigger GDMA transfers

The timer frequency is derived at compile time from
panel_config.h

============================================================
*/

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
============================================================
Public API
============================================================
*/

/*
Initialize the frame timer.

This configures the GPTimer peripheral and prepares
the interrupt callback.

Must be called once during system startup before
any DMA or display driver initialization.
*/
void frame_timer_init(void);

/*
Start the timer.

After this call the timer begins generating
periodic events at the configured frequency.
*/
void frame_timer_start(void);

/*
Stop the timer.
*/
void frame_timer_stop(void);

#ifdef __cplusplus
}
#endif