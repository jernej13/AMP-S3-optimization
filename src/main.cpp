#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_system.h"

extern "C" void app_main(void)
{
    // Disable WiFi
    esp_wifi_stop();
    esp_wifi_deinit();

    // Disable Bluetooth
    esp_bt_controller_disable();
    esp_bt_controller_deinit();

    // HUB75 pins (example — adjust if needed)
    const gpio_num_t R1 = GPIO_NUM_42;
    const gpio_num_t G1 = GPIO_NUM_41;
    const gpio_num_t B1 = GPIO_NUM_40;

    const gpio_num_t R2 = GPIO_NUM_38;
    const gpio_num_t G2 = GPIO_NUM_39;
    const gpio_num_t B2 = GPIO_NUM_37;

    const gpio_num_t CLK = GPIO_NUM_2;
    const gpio_num_t LAT = GPIO_NUM_47;
    const gpio_num_t OE  = GPIO_NUM_48;

    gpio_config_t io_conf = {};
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask =
        (1ULL<<R1) |
        (1ULL<<G1) |
        (1ULL<<B1) |
        (1ULL<<R2) |
        (1ULL<<G2) |
        (1ULL<<B2) |
        (1ULL<<CLK) |
        (1ULL<<LAT) |
        (1ULL<<OE);

    gpio_config(&io_conf);

    // Enable output
    gpio_set_level(OE, 0);

    while(true)
    {
        // Turn all RGB channels on
        gpio_set_level(R1, 1);
        gpio_set_level(G1, 1);
        gpio_set_level(B1, 1);
        gpio_set_level(R2, 1);
        gpio_set_level(G2, 1);
        gpio_set_level(B2, 1);

        // Clock some pixels into the shift registers
        for(int i=0;i<64;i++)
        {
            gpio_set_level(CLK, 1);
            gpio_set_level(CLK, 0);
        }

        // Latch data to outputs
        gpio_set_level(LAT, 1);
        gpio_set_level(LAT, 0);

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}