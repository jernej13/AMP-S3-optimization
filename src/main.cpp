#include "driver/gpio.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/gpio_struct.h"

/* HUB75 RGB top */
#define PIN_R1 42
#define PIN_G1 41
#define PIN_B1 40

/* HUB75 RGB bottom */
#define PIN_R2 38
#define PIN_G2 39
#define PIN_B2 37

/* Control signals */
#define PIN_CLK 2
#define PIN_LAT 47
#define PIN_OE 48

/* Address lines */
#define PIN_A 45
#define PIN_B 36
#define PIN_C 21
#define PIN_D 14
#define PIN_E 13

#define PIN_MASK(p) (1ULL << (p))

extern "C" void app_main() {
    esp_wifi_stop();
    esp_wifi_deinit();

    gpio_config_t io = {};
    io.mode = GPIO_MODE_OUTPUT;
    io.pin_bit_mask =
        PIN_MASK(PIN_R1) |
        PIN_MASK(PIN_G1) |
        PIN_MASK(PIN_B1) |
        PIN_MASK(PIN_R2) |
        PIN_MASK(PIN_G2) |
        PIN_MASK(PIN_B2) |
        PIN_MASK(PIN_CLK) |
        PIN_MASK(PIN_LAT) |
        PIN_MASK(PIN_OE) |
        PIN_MASK(PIN_A) |
        PIN_MASK(PIN_B) |
        PIN_MASK(PIN_C) |
        PIN_MASK(PIN_D) |
        PIN_MASK(PIN_E);

    gpio_config(&io);

    /* Select row 0 */
    GPIO.out_w1tc =
        PIN_MASK(PIN_A) |
        PIN_MASK(PIN_B) |
        PIN_MASK(PIN_C) |
        PIN_MASK(PIN_D) |
        PIN_MASK(PIN_E);

    /* Disable LEDs */
    GPIO.out_w1ts = PIN_MASK(PIN_OE);

    const uint64_t rgb_mask =
        PIN_MASK(PIN_R1) |
        PIN_MASK(PIN_G1) |
        PIN_MASK(PIN_B1) |
        PIN_MASK(PIN_R2) |
        PIN_MASK(PIN_G2) |
        PIN_MASK(PIN_B2);

    const uint64_t clk_mask = PIN_MASK(PIN_CLK);
    const uint64_t lat_mask = PIN_MASK(PIN_LAT);

    while (true) {
        GPIO.out_w1ts = PIN_MASK(PIN_OE);

        for (int i = 0; i < 64; i++) {
            GPIO.out_w1ts = rgb_mask;

            GPIO.out_w1ts = clk_mask;
            GPIO.out_w1tc = clk_mask;
        }

        GPIO.out_w1ts = lat_mask;
        GPIO.out_w1tc = lat_mask;

        GPIO.out_w1tc = PIN_MASK(PIN_OE);
    }
}