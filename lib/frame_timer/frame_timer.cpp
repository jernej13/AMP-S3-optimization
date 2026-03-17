/*
============================================================
frame_timer.cpp

Implementation of the system master timing source.

Architecture Goal
------------------------------------------------------------
This timer will ultimately drive the full rendering pipeline:

    TIMER
      ↓
    GDMA TRIGGER
      ↓
    LCD PARALLEL CLOCK
      ↓
    HUB75 DATA SHIFT

For now we only validate timer precision by toggling
a GPIO pin which can be measured on an oscilloscope.

============================================================
*/

#include "frame_timer.h"

#include <driver/gptimer.h>
#include <driver/gpio.h>

#include "panel_config.h"
#include "debug_config.h"
#include "esp_attr.h"

static const char* TAG= "frame_timer";

/*
============================================================
Compile-time Timing Calculation
============================================================

panel_config defines frequency in kHz.

Example:
    TIMER_FREQUENCY = 5000 (kHz)

Converted to Hz for GPTimer configuration.

============================================================
*/

#define TIMER_FREQ_HZ (TIMER_FREQUENCY * 1000ULL)

/*
GPTimer base clock on ESP32-S3 = 80 MHz.
We use a divider so that each timer tick = 1 event.
*/

#define GPTIMER_BASE_CLK 80000000ULL

/*
Compute divider at compile time.

Example for 5 MHz:
    divider = 80MHz / 5MHz = 16
*/

#define TIMER_DIVIDER (GPTIMER_BASE_CLK / TIMER_FREQ_HZ)

/*
Validate compile-time divider.

If divider becomes invalid the build fails.
*/

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
Debug test GPIO.

We toggle CLK so timing can be verified on oscilloscope.
*/

#define TEST_GPIO ((gpio_num_t)PIN_CLK)

/*
============================================================
Timer Interrupt Callback
============================================================
This executes on every timer event.

Currently:
    Toggle debug GPIO

Later:
    Trigger GDMA transfer

============================================================
*/

static bool IRAM_ATTR timer_callback(gptimer_handle_t timer,
                                     const gptimer_alarm_event_data_t* edata,
                                     void* user_ctx) {

#if DEBUG
    gpio_set_level(TEST_GPIO, !gpio_get_level(TEST_GPIO));
#endif

    return true;
}

/*
============================================================
frame_timer_init()

Configure hardware timer and debug GPIO.

============================================================
*/

void frame_timer_init(void) {

    DBG_LOG(TAG, "Initializing frame timer");

    DBG_LOG(TAG, "Target frequency: %llu Hz", TIMER_FREQ_HZ);
    DBG_LOG(TAG, "Timer divider: %llu", TIMER_DIVIDER);

    /*
    --------------------------------------------------------
    Configure debug GPIO
    --------------------------------------------------------
    */

#if DEBUG

    DBG_LOG(TAG, "Configuring test GPIO");

    gpio_config_t io= {};
    io.mode= GPIO_MODE_OUTPUT;
    io.pin_bit_mask= (1ULL << TEST_GPIO);

    gpio_config(&io);

#endif

    /*
    --------------------------------------------------------
    Configure GPTimer
    --------------------------------------------------------
    */

    DBG_LOG(TAG, "Creating GPTimer instance");

    gptimer_config_t config= {};
    config.clk_src= GPTIMER_CLK_SRC_DEFAULT;
    config.direction= GPTIMER_COUNT_UP;

    /*
    Resolution determines timer tick precision.

    resolution_hz = base_clock / divider
    */

    config.resolution_hz= TIMER_FREQ_HZ;

    ESP_ERROR_CHECK(gptimer_new_timer(&config, &timer));

    /*
    --------------------------------------------------------
    Configure periodic alarm
    --------------------------------------------------------
    */

    DBG_LOG(TAG, "Configuring periodic alarm");

    gptimer_alarm_config_t alarm= {};
    alarm.reload_count= 0;
    alarm.alarm_count= 1;
    alarm.flags.auto_reload_on_alarm= true;

    ESP_ERROR_CHECK(gptimer_set_alarm_action(timer, &alarm));

    /*
    --------------------------------------------------------
    Register callback
    --------------------------------------------------------
    */

    DBG_LOG(TAG, "Registering timer callback");

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

Start generating timer events.

============================================================
*/

void frame_timer_start(void) {

    DBG_LOG(TAG, "Starting frame timer");

    ESP_ERROR_CHECK(gptimer_start(timer));
}

/*
============================================================
frame_timer_stop()

Stop timer operation.

============================================================
*/

void frame_timer_stop(void) {

    DBG_LOG(TAG, "Stopping frame timer");

    ESP_ERROR_CHECK(gptimer_stop(timer));
}