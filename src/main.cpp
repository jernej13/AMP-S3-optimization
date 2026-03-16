#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "hub75_driver.h"

extern "C" void app_main() {
    HUB75Driver driver;
    vTaskDelay(pdMS_TO_TICKS(3000));

    driver.init();
    driver.start();

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}