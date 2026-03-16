#include "driver/gpio.h"
#include "soc/gpio_struct.h"
#include "esp_pm.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"

/* ---------------------------
   SPEED CONTROL
--------------------------- */

#define SPEED_FACTOR 0.001f   // 1.0 = fastest, 0.5 = 2x slower


/* ---------------------------
   HUB75 RGB pins
--------------------------- */

#define R1 42
#define G1 41
#define B1 40

#define R2 38
#define G2 39
#define B2 37


/* ---------------------------
   HUB75 control pins
--------------------------- */

#define CLK 2
#define LAT 47
#define OE  14


/* ---------------------------
   Row address pins
--------------------------- */

#define A 45
#define B 36
#define C 48
#define D 35


#define WIDTH 32
#define ROWS 16


/* ---------------------------
   Masks
--------------------------- */

static uint32_t row_mask[ROWS];

static const uint32_t RGB_CLEAR =
      (1UL<<(R1-32))
    | (1UL<<(G1-32))
    | (1UL<<(B1-32))
    | (1UL<<(R2-32))
    | (1UL<<(G2-32))
    | (1UL<<(B2-32));

static const uint32_t WHITE_TOP =
      (1UL<<(R1-32))
    | (1UL<<(G1-32))
    | (1UL<<(B1-32));

static const uint32_t WHITE_BOTTOM =
      (1UL<<(R2-32))
    | (1UL<<(G2-32))
    | (1UL<<(B2-32));

static const uint32_t GREEN_BOTTOM =
      (1UL<<(G2-32));


/* ---------------------------
   Row address masks
--------------------------- */

void build_row_masks()
{
    for(int r=0;r<ROWS;r++)
    {
        uint32_t m=0;

        if(r&1) m |= (1UL<<(A-32));
        if(r&2) m |= (1UL<<(B-32));
        if(r&4) m |= (1UL<<(C-32));
        if(r&8) m |= (1UL<<(D-32));

        row_mask[r]=m;
    }
}


/* ---------------------------
   Display task
--------------------------- */

void display_task(void*)
{

    uint32_t frame_counter = 0;

    int frame_divider = (int)(1.0f / SPEED_FACTOR);
    if(frame_divider < 1) frame_divider = 1;

    int frame_skip = 0;


    while(true)
    {

        for(int row=0; row<ROWS; row++)
        {

            /* disable output */

            GPIO.out_w1ts = (1UL<<OE);

            /* select row */

            GPIO.out1_w1tc.val =
                (1UL<<(A-32))|
                (1UL<<(B-32))|
                (1UL<<(C-32))|
                (1UL<<(D-32));

            GPIO.out1_w1ts.val = row_mask[row];


            for(int col=0; col<WIDTH; col++)
            {

                GPIO.out1_w1tc.val = RGB_CLEAR;

                uint32_t out = WHITE_TOP | WHITE_BOTTOM;


                /* bottom row binary counter */

                if(row == 15)
                {
                    if(frame_counter & (1UL<<col))
                        out = WHITE_TOP | GREEN_BOTTOM;
                    else
                        out = WHITE_TOP;
                }

                GPIO.out1_w1ts.val = out;

                /* clock */

                GPIO.out_w1ts = (1UL<<CLK);
                GPIO.out_w1tc = (1UL<<CLK);
            }


            /* latch */

            GPIO.out1_w1ts.val = (1UL<<(LAT-32));
            GPIO.out1_w1tc.val = (1UL<<(LAT-32));


            /* enable */

            GPIO.out_w1tc = (1UL<<OE);
        }


        frame_skip++;

        if(frame_skip >= frame_divider)
        {
            frame_skip = 0;
            frame_counter++;
        }
    }
}


/* ---------------------------
   Main
--------------------------- */

extern "C" void app_main()
{

    gpio_config_t io={};

    io.mode = GPIO_MODE_OUTPUT;

    io.pin_bit_mask =
        (1ULL<<R1)|(1ULL<<G1)|(1ULL<<B1)|
        (1ULL<<R2)|(1ULL<<G2)|(1ULL<<B2)|
        (1ULL<<CLK)|(1ULL<<LAT)|(1ULL<<OE)|
        (1ULL<<A)|(1ULL<<B)|(1ULL<<C)|(1ULL<<D);

    gpio_config(&io);


    /* CPU locked to 240 MHz */

    esp_pm_config_t pm={
        .max_freq_mhz=240,
        .min_freq_mhz=240,
        .light_sleep_enable=false
    };

    esp_pm_configure(&pm);


    build_row_masks();


    /* disable watchdog */

    esp_task_wdt_deinit();


    /* run display on core 1 */

    xTaskCreatePinnedToCore(
        display_task,
        "display",
        4096,
        NULL,
        configMAX_PRIORITIES-1,
        NULL,
        1);
}