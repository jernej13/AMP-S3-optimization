/*
============================================================
frame_timer.cpp

Implementation of the system master timing source.

Architecture Goal
------------------------------------------------------------
This timer drives the full rendering pipeline:

    TIMER
      ↓
    GDMA TRIGGER
      ↓
    LCD PARALLEL CLOCK
      ↓
    HUB75 DATA SHIFT

Currently set up to generate periodic timer events.
No GPIO operations are included — this is ready
for DMA triggering in the next stage.

============================================================
*/

#include "frame_timer.h"

#include <driver/gptimer.h>
#include "panel_config.h"
#include "debug_config.h"
#include "esp_attr.h"

static const char* TAG= "frame_timer";

/*
============================================================
Compile-time Timing Calculation
------------------------------------------------------------
TIMER_FREQUENCY is defined in panel_config.h in kHz.

Example:
    TIMER_FREQUENCY = 5000 (kHz) = 5 MHz

Converted to Hz for GPTimer configuration.
============================================================
*/
#define TIMER_FREQ_HZ (TIMER_FREQUENCY * 1000ULL)

/*
GPTimer base clock on ESP32-S3 = 80 MHz.
The timer divider determines timer tick resolution.
*/
#define GPTIMER_BASE_CLK 80000000ULL

/*
Compute divider at compile time.
Example for 5 MHz: divider = 80MHz / 5MHz = 16
*/
#define TIMER_DIVIDER (GPTIMER_BASE_CLK / TIMER_FREQ_HZ)

#if TIMER_DIVIDER == 0
#error "Timer divider invalid — frequency too high"
#endif

/*
============================================================
Static State
============================================================
*/
static gptimer_handle_t timer= NULL;

/*
============================================================
Timer Callback
------------------------------------------------------------
This executes on every timer event.

Currently empty; later it will trigger DMA transfers.
============================================================
*/
static bool IRAM_ATTR timer_callback(gptimer_handle_t timer,
                                     const gptimer_alarm_event_data_t* edata,
                                     void* user_ctx) {
    // Placeholder for future DMA trigger
    return true;
}

/*
============================================================
frame_timer_init()
------------------------------------------------------------
Configure GPTimer with periodic alarm.
No GPIO operations are included.
============================================================
*/
void frame_timer_init(void) {
    DBG_LOG(TAG, "Initializing frame timer");
    DBG_LOG(TAG, "Target frequency: %llu Hz", TIMER_FREQ_HZ);
    DBG_LOG(TAG, "Timer divider: %llu", TIMER_DIVIDER);

    /*
    --------------------------------------------------------
    Configure GPTimer
    --------------------------------------------------------
    */
    gptimer_config_t config= {};
    config.clk_src= GPTIMER_CLK_SRC_DEFAULT;
    config.direction= GPTIMER_COUNT_UP;
    config.resolution_hz= TIMER_FREQ_HZ;

    ESP_ERROR_CHECK(gptimer_new_timer(&config, &timer));

    /*
    --------------------------------------------------------
    Configure periodic alarm
    --------------------------------------------------------
    */
    gptimer_alarm_config_t alarm= {};
    alarm.reload_count= 0;
    alarm.alarm_count= 500;                 // Set for slower effective rate
    alarm.flags.auto_reload_on_alarm= true; // Periodic repetition

    ESP_ERROR_CHECK(gptimer_set_alarm_action(timer, &alarm));

    /*
    --------------------------------------------------------
    Register callback
    --------------------------------------------------------
    */
    gptimer_event_callbacks_t cbs= {};
    cbs.on_alarm= timer_callback;

    ESP_ERROR_CHECK(gptimer_register_event_callbacks(timer, &cbs, NULL));

    /*
    --------------------------------------------------------
    Enable timer
    --------------------------------------------------------
    */
    DBG_LOG(TAG, "Enabling timer");
    ESP_ERROR_CHECK(gptimer_enable(timer));
}

/*
============================================================
frame_timer_start()
------------------------------------------------------------
Start generating timer events
============================================================
*/
void frame_timer_start(void) {
    DBG_LOG(TAG, "Starting frame timer");
    ESP_ERROR_CHECK(gptimer_start(timer));
}

/*
============================================================
frame_timer_stop()
------------------------------------------------------------
Stop timer operation
============================================================
*/
void frame_timer_stop(void) {
    DBG_LOG(TAG, "Stopping frame timer");
    ESP_ERROR_CHECK(gptimer_stop(timer));
}