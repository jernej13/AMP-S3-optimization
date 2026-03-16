#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "panel_config.h"

extern "C" void app_main() {
    printf("HUB75 deterministic driver starting...\n");

    while (true) {
        printf("System alive\n");

        printf("Panel size: %dx%d\n", PANEL_WIDTH, PANEL_HEIGHT);
        printf("Scan rows: %d\n", PANEL_SCAN_ROWS);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}