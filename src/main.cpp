#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "soc/gpio_struct.h"
#include "soc/gpio_reg.h"

/*
    HUB75 signal lines (example mapping)

    You MUST adjust these if the pin mapping differs.
    These are typical MatrixPortal S3 HUB75 pins.

    Top half RGB
*/
#define PIN_R1 42
#define PIN_G1 41
#define PIN_B1 40

/*
    Bottom half RGB
*/
#define PIN_R2 38
#define PIN_G2 39
#define PIN_B2 37

/*
    HUB75 control signals
*/
#define PIN_CLK 2
#define PIN_LAT 47
#define PIN_OE  48

/*
    Row address pins
    We keep them all LOW so row 0 is selected
*/
#define PIN_A 45
#define PIN_B 36
#define PIN_C 21
#define PIN_D 14
#define PIN_E 13

/*
    Bitmask helper
*/
#define PIN_MASK(p) (1ULL << (p))

extern "C" void app_main()
{

    /*
        --------------------------------------------------
        Disable WiFi and Bluetooth
        --------------------------------------------------
        Removes radio tasks and interrupts
        which improves timing determinism.
    */

    esp_wifi_stop();
    esp_wifi_deinit();

    esp_bt_controller_disable();
    esp_bt_controller_deinit();


    /*
        --------------------------------------------------
        Configure all HUB75 pins as outputs
        --------------------------------------------------
    */

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
        PIN_MASK(PIN_OE)  |
        PIN_MASK(PIN_A)   |
        PIN_MASK(PIN_B)   |
        PIN_MASK(PIN_C)   |
        PIN_MASK(PIN_D)   |
        PIN_MASK(PIN_E);

    gpio_config(&io);


    /*
        --------------------------------------------------
        Select row 0
        --------------------------------------------------
        HUB75 rows are selected by A/B/C/D/E lines.
        Row 0 = all LOW.
    */

    GPIO.out_w1tc =
        (1 << PIN_A) |
        (1 << PIN_B) |
        (1 << PIN_C) |
        (1 << PIN_D) |
        (1 << PIN_E);


    /*
        --------------------------------------------------
        Turn LEDs OFF initially
        --------------------------------------------------
        OE is active LOW.
    */

    GPIO.out_w1ts = (1 << PIN_OE);


    /*
        --------------------------------------------------
        Precompute masks for faster pixel writes
        --------------------------------------------------
    */

    const uint32_t rgb_mask =
        (1 << PIN_R1) |
        (1 << PIN_G1) |
        (1 << PIN_B1) |
        (1 << PIN_R2) |
        (1 << PIN_G2) |
        (1 << PIN_B2);


    const uint32_t clk_mask = (1 << PIN_CLK);
    const uint32_t lat_mask = (1 << PIN_LAT);


    /*
        --------------------------------------------------
        Main render loop
        --------------------------------------------------
        Continuously pushes a full white row to the panel.
        No delays, maximum speed.
    */

    while (true)
    {

        /*
            Disable LED output while shifting pixels
            (prevents visual glitches)
        */
        GPIO.out_w1ts = (1 << PIN_OE);


        /*
            --------------------------------------------------
            Shift 64 pixels into HUB75 registers
            --------------------------------------------------

            Each clock shifts one pixel across the panel.

            Setting all RGB pins HIGH means the pixel color
            will be WHITE.

            This loop is intentionally extremely small so
            the compiler can optimize it heavily.
        */

        for (int i = 0; i < 64; i++)
        {

            /* Set RGB lines HIGH (white pixel) */
            GPIO.out_w1ts = rgb_mask;

            /* CLK HIGH */
            GPIO.out_w1ts = clk_mask;

            /* CLK LOW */
            GPIO.out_w1tc = clk_mask;
        }


        /*
            --------------------------------------------------
            Latch shifted data to LED drivers
            --------------------------------------------------
        */

        GPIO.out_w1ts = lat_mask;
        GPIO.out_w1tc = lat_mask;


        /*
            Enable LED output
        */

        GPIO.out_w1tc = (1 << PIN_OE);

        /*
            At this point the first row should appear white.
            Loop immediately repeats to maintain brightness.
        */
    }
}