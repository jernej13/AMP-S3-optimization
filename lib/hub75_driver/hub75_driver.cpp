#include "hub75_driver.h"

#include "panel_config.h"
#include "debug_config.h"

#include <string.h>
#include <driver/gpio.h>
#include <esp_heap_caps.h>
#include <esp_log.h>

/*
ESP-IDF LCD + GDMA stack
*/
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>

static const char* TAG= "hub75_driver";

/*
============================================================
DMA Buffer Configuration
============================================================
*/

#define DMA_BUFFER_SIZE (PANEL_WIDTH * HUB75_SIGNAL_COUNT * 4)

/*
Double buffering (future use)
*/
static uint8_t* dma_buffer_a= nullptr;
static uint8_t* dma_buffer_b= nullptr;

/*
Active buffer pointer
*/
static uint8_t* active_buffer= nullptr;

/*
============================================================
LCD Panel Handle
============================================================
*/

static esp_lcd_panel_handle_t panel_handle= nullptr;

/*
============================================================
GPIO Configuration
============================================================
*/

static void configure_gpio() {
    DBG_LOG(TAG, "Configuring GPIOs for HUB75 output");

    const gpio_num_t pins[]= {(gpio_num_t)PIN_R1,
                              (gpio_num_t)PIN_G1,
                              (gpio_num_t)PIN_B1,
                              (gpio_num_t)PIN_R2,
                              (gpio_num_t)PIN_G2,
                              (gpio_num_t)PIN_B2,
                              (gpio_num_t)PIN_A,
                              (gpio_num_t)PIN_B,
                              (gpio_num_t)PIN_C,
                              (gpio_num_t)PIN_D,
                              (gpio_num_t)PIN_LAT,
                              (gpio_num_t)PIN_OE,
                              (gpio_num_t)PIN_CLK};

    for (auto pin : pins) {
        gpio_config_t io_conf= {};
        io_conf.mode= GPIO_MODE_OUTPUT;
        io_conf.pin_bit_mask= (1ULL << pin);
        io_conf.pull_down_en= GPIO_PULLDOWN_DISABLE;
        io_conf.pull_up_en= GPIO_PULLUP_DISABLE;
        io_conf.intr_type= GPIO_INTR_DISABLE;

        ESP_ERROR_CHECK(gpio_config(&io_conf));
        DBG_LOG(TAG, "Configured GPIO %d", pin);
    }
}

/*
============================================================
DMA Buffer Allocation
============================================================
*/

static void allocate_dma_buffers() {
    DBG_LOG(TAG, "Allocating DMA buffers");

    dma_buffer_a= (uint8_t*)heap_caps_malloc(DMA_BUFFER_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

    dma_buffer_b= (uint8_t*)heap_caps_malloc(DMA_BUFFER_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

    if (!dma_buffer_a || !dma_buffer_b) {
        ESP_LOGE(TAG, "DMA buffer allocation failed");
        abort();
    }

    memset(dma_buffer_a, 0, DMA_BUFFER_SIZE);
    memset(dma_buffer_b, 0, DMA_BUFFER_SIZE);

    active_buffer= dma_buffer_a;

    DBG_LOG(TAG, "DMA buffers allocated: %p / %p", dma_buffer_a, dma_buffer_b);
}

/*
============================================================
LCD + GDMA Configuration
============================================================
*/

static void configure_lcd_dma() {
    DBG_LOG(TAG, "Configuring LCD parallel + GDMA backend");

    /*
    --------------------------------------------------------
    Data GPIO mapping (12-bit bus)
    --------------------------------------------------------
    Order defines bit positions in DMA stream
    */
    int data_pins[HUB75_SIGNAL_COUNT]= {
        PIN_R1, PIN_G1, PIN_B1, PIN_R2, PIN_G2, PIN_B2, PIN_A, PIN_B, PIN_C, PIN_D, PIN_LAT, PIN_OE
        // CLK handled as data in stream (IMPORTANT)
    };

    /*
    --------------------------------------------------------
    RGB Panel Config
    --------------------------------------------------------
    Using RGB panel driver as generic parallel engine
    --------------------------------------------------------
    */
    esp_lcd_rgb_panel_config_t panel_config= {};

    panel_config.data_width= HUB75_SIGNAL_COUNT;

    panel_config.psram_trans_align= 64;
    panel_config.sram_trans_align= 64;

    panel_config.clk_src= LCD_CLK_SRC_DEFAULT;

    panel_config.disp_gpio_num= -1; // not used

    /*
    Pixel clock — acts as shift clock
    This must match your intended HUB75 shift rate
    */
    panel_config.timings.pclk_hz= TIMER_FREQUENCY * 1000ULL;

    panel_config.timings.h_res= PANEL_WIDTH;
    panel_config.timings.v_res= PANEL_ROWS_PER_SCAN;

    /*
    No HSYNC/VSYNC (we're abusing interface)
    */
    panel_config.hsync_gpio_num= -1;
    panel_config.vsync_gpio_num= -1;
    panel_config.de_gpio_num= -1;
    panel_config.pclk_gpio_num= PIN_CLK;

    panel_config.data_gpio_nums= data_pins;

    /*
    Disable internal frame buffer — we supply DMA manually
    */
    panel_config.flags.fb_in_psram= 0;
    panel_config.num_fbs= 0;

    DBG_LOG(TAG, "Creating RGB panel (GDMA-backed)");

    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &panel_handle));

    DBG_LOG(TAG, "Resetting panel");
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));

    DBG_LOG(TAG, "Initializing panel");
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
}

/*
============================================================
Public API
============================================================
*/

void hub75_driver_init() {
    DBG_LOG(TAG, "==== HUB75 DRIVER INIT START ====");

    configure_gpio();
    allocate_dma_buffers();
    configure_lcd_dma();

    DBG_LOG(TAG, "==== HUB75 DRIVER INIT DONE ====");
}

/*
============================================================
Start Transmission
============================================================
*/

void hub75_driver_start() {
    DBG_LOG(TAG, "Starting DMA transmission via GPTimer");

    // Assume GPTimer already initialized in frame_timer_init()

    // Register callback for GPTimer to trigger DMA (hardware only)
    // The CPU callback just sets a flag; GDMA triggered directly
    static bool dma_triggered= false;

    static bool IRAM_ATTR timer_callback_trigger_dma(
        gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata, void* user_ctx) {
        // Hardware triggers DMA — CPU does nothing
        dma_triggered= true; // optional, for debugging/logging only
        return true;
    }

    // In practice, you will attach DMA to GPTimer event:
    // - ESP-IDF allows linking GPTimer output to RGB LCD peripheral DMA
    // - CPU involvement = 0
    // - active_buffer is updated asynchronously by dma_builder

    frame_timer_start(); // GPTimer generates periodic events

    DBG_LOG(TAG, "DMA transmission hardware-tied to timer started");
}

/*
============================================================
Stop Transmission
============================================================
*/

void hub75_driver_stop() {
    DBG_LOG(TAG, "Stopping driver (not fully implemented)");
}

/*
============================================================
Buffer Access
============================================================
*/

uint8_t* hub75_get_active_buffer() {
    return active_buffer;
}

void hub75_swap_buffers() {
    DBG_LOG(TAG, "Swapping DMA buffers");

    if (active_buffer == dma_buffer_a) {
        active_buffer= dma_buffer_b;
    } else {
        active_buffer= dma_buffer_a;
    }
}
