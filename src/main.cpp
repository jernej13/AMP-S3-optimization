#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "hub75_driver.h"

/*
============================================================
Timer Test Program

This program initializes the frame timer and starts it.

The timer toggles PIN_CLK which can be measured on an
oscilloscope to verify exact timing.

Expected frequency:

    TIMER_FREQUENCY / 2

Because each interrupt toggles the GPIO.

Example:

    TIMER_FREQUENCY = 5000 kHz

Oscilloscope should show:

    2.5 MHz square wave

============================================================
*/

#include "frame_timer.h"
#include "debug_config.h"

static const char* TAG= "main";

extern "C" void app_main(void) {

    DBG_LOG(TAG, "System start");

    /*
    --------------------------------------------------------
    Initialize frame timer
    --------------------------------------------------------
    */

    frame_timer_init();

    /*
    --------------------------------------------------------
    Start timer
    --------------------------------------------------------
    */

    frame_timer_start();

    DBG_LOG(TAG, "Timer running");

    /*
    Idle forever
    */

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}