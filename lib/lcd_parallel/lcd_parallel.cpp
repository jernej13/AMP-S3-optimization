#include "lcd_parallel.h"

#include <stdio.h>

#include "panel_config.h"

#include <esp_log.h>
#include <esp_rom_gpio.h>
#include <esp_rom_sys.h>

#include <driver/gpio.h>

#include <esp_private/periph_ctrl.h>
#include <soc/lcd_cam_struct.h>
#include <soc/gpio_sig_map.h>

static const char* TAG= "lcd_parallel";

/*
============================================================
Constructor
============================================================
*/

LCDParallel::LCDParallel() {
    dma_channel= nullptr;
}

/*
============================================================
Initialize LCD parallel engine
============================================================
*/

esp_err_t LCDParallel::init() {
    ESP_LOGI(TAG, "Initializing LCD_CAM parallel interface");

    /*
    --------------------------------------------------------
    Enable LCD_CAM peripheral
    --------------------------------------------------------
    */

    periph_module_enable(PERIPH_LCD_CAM_MODULE);
    periph_module_reset(PERIPH_LCD_CAM_MODULE);

    /*
    Reset LCD bus
    */

    LCD_CAM.lcd_user.lcd_reset= 1;
    esp_rom_delay_us(1000);

    configure_clock();
    configure_lcd_mode();
    configure_gpio();

    /*
    Setup GDMA channel
    */

    esp_err_t err= setup_gdma();

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "GDMA setup failed");
        return err;
    }

    ESP_LOGI(TAG, "LCD parallel initialized");

    return ESP_OK;
}

/*
============================================================
Configure LCD clock
============================================================

Clock source: PLL_F160M

Pixel clock = 160 MHz / divider
*/

void LCDParallel::configure_clock() {
    const uint32_t target_freq= 20000000; // 20 MHz safe default
    const uint32_t pll_freq= 160000000;

    uint32_t divider= pll_freq / target_freq;

    LCD_CAM.lcd_clock.lcd_clk_sel= 3; // PLL_F160M
    LCD_CAM.lcd_clock.lcd_clkm_div_num= divider;

    LCD_CAM.lcd_clock.lcd_clkm_div_a= 1;
    LCD_CAM.lcd_clock.lcd_clkm_div_b= 0;

    LCD_CAM.lcd_clock.lcd_clkcnt_n= 1;
    LCD_CAM.lcd_clock.lcd_clk_equ_sysclk= 1;

    ESP_LOGI(TAG, "LCD clock configured (divider=%lu)", divider);
}

/*
============================================================
Configure LCD peripheral mode
============================================================
*/

void LCDParallel::configure_lcd_mode() {
    /*
    Use i8080 style parallel mode
    */

    LCD_CAM.lcd_ctrl.lcd_rgb_mode_en= 0;

    LCD_CAM.lcd_user.lcd_always_out_en= 1;

    /*
    16-bit parallel bus
    */

    LCD_CAM.lcd_user.lcd_2byte_en= 1;

    /*
    Normal bit order
    */

    LCD_CAM.lcd_user.lcd_bit_order= 0;
    LCD_CAM.lcd_user.lcd_8bits_order= 0;

    /*
    Enable output
    */

    LCD_CAM.lcd_user.lcd_dout= 1;

    /*
    Disable start for now
    */

    LCD_CAM.lcd_user.lcd_start= 0;
}

/*
============================================================
Configure GPIO routing
============================================================

Maps HUB75 signals onto the LCD data bus.

Data lines:

D0  R1
D1  G1
D2  B1
D3  R2
D4  G2
D5  B2
D6  A
D7  B
D8  C
D9  D
D10 unused
D11 LAT
D12 OE
*/

void LCDParallel::configure_gpio() {
    int data_pins[16]= {PIN_R1,
                        PIN_G1,
                        PIN_B1,

                        PIN_R2,
                        PIN_G2,
                        PIN_B2,

                        PIN_A,
                        PIN_B,
                        PIN_C,
                        PIN_D,

                        -1,

                        PIN_LAT,
                        PIN_OE,

                        -1,
                        -1,
                        -1};

    for (int i= 0; i < 16; i++) {
        if (data_pins[i] < 0) continue;

        gpio_set_direction((gpio_num_t)data_pins[i], GPIO_MODE_OUTPUT);

        esp_rom_gpio_connect_out_signal(data_pins[i], LCD_DATA_OUT0_IDX + i, false, false);
    }

    /*
    Clock output
    */

    gpio_set_direction((gpio_num_t)PIN_CLK, GPIO_MODE_OUTPUT);

    esp_rom_gpio_connect_out_signal(PIN_CLK, LCD_PCLK_IDX, false, false);

    ESP_LOGI(TAG, "GPIO matrix configured");
}

/*
============================================================
Setup GDMA channel
============================================================
*/

esp_err_t LCDParallel::setup_gdma() {
    ESP_LOGI(TAG, "Allocating GDMA channel");

    gdma_channel_alloc_config_t config= {.sibling_chan= NULL,
                                         .direction= GDMA_CHANNEL_DIRECTION_TX};

    esp_err_t err= gdma_new_channel(&config, &dma_channel);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "GDMA allocation failed");
        return err;
    }

    /*
    Connect GDMA to LCD peripheral
    */

    gdma_connect(dma_channel, GDMA_MAKE_TRIGGER(GDMA_TRIG_PERIPH_LCD, 0));

    ESP_LOGI(TAG, "GDMA channel ready");

    return ESP_OK;
}

/*
============================================================
Shutdown peripheral
============================================================
*/

void LCDParallel::shutdown() {
    if (dma_channel) {
        gdma_disconnect(dma_channel);
        gdma_del_channel(dma_channel);
        dma_channel= nullptr;
    }

    periph_module_disable(PERIPH_LCD_CAM_MODULE);

    ESP_LOGI(TAG, "LCD parallel shutdown");
}

/*
============================================================
Return GDMA channel
============================================================
*/

gdma_channel_handle_t LCDParallel::get_dma_channel() const {
    return dma_channel;
}