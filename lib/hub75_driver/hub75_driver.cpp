#include "hub75_driver.h"

#include <stdio.h>

#include <esp_log.h>
#include <esp_heap_caps.h>

#include <soc/lcd_cam_struct.h>
#include <esp_private/gdma.h>
#include <hal/dma_types.h>

#include "panel_config.h"
#include "dma_builder.h"

static const char* TAG= "hub75_driver";

HUB75Driver::HUB75Driver() {
    frame_buffer= nullptr;
}

esp_err_t HUB75Driver::init() {
    ESP_LOGI(TAG, "Initializing HUB75 driver");

    esp_err_t err;

    err= init_lcd();
    if (err != ESP_OK) return err;

    err= init_buffers();
    if (err != ESP_OK) return err;

    err= init_dma();
    if (err != ESP_OK) return err;

    ESP_LOGI(TAG, "HUB75 driver ready");

    return ESP_OK;
}

esp_err_t HUB75Driver::init_lcd() {
    LCD_CAM.lcd_user.lcd_dout= 1;

    return lcd.init();
}

esp_err_t HUB75Driver::init_buffers() {
    /*
    Allocate DMA capable memory
    */

    size_t buffer_size= 4096;

    frame_buffer= (uint8_t*)heap_caps_malloc(buffer_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

    if (!frame_buffer) {
        ESP_LOGE(TAG, "Framebuffer allocation failed");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Framebuffer allocated (%d bytes)", buffer_size);

    return ESP_OK;
}

esp_err_t HUB75Driver::init_dma() {
    ESP_LOGI(TAG, "Building DMA stream");

    esp_err_t err= builder.build();
    if (err != ESP_OK) return err;

    uint16_t* buffer= builder.get_buffer();
    size_t buffer_size= builder.get_buffer_size();

    /*
    ------------------------------------------------
    Create DMA descriptors
    ------------------------------------------------
    */

    const size_t MAX_DMA_LEN= 4092;

    dma_desc_count= (buffer_size + MAX_DMA_LEN - 1) / MAX_DMA_LEN;

    dma_desc= (lldesc_t*)heap_caps_malloc(sizeof(lldesc_t) * dma_desc_count,
                                          MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

    if (!dma_desc) {
        ESP_LOGE(TAG, "DMA descriptor allocation failed");
        return ESP_ERR_NO_MEM;
    }

    uint8_t* data_ptr= (uint8_t*)buffer;

    size_t remaining= buffer_size;

    for (size_t i= 0; i < dma_desc_count; i++) {

        size_t chunk= remaining;
        if (chunk > MAX_DMA_LEN) chunk= MAX_DMA_LEN;

        dma_desc[i].size= chunk;
        dma_desc[i].length= chunk;
        dma_desc[i].owner= 1;

        dma_desc[i].buf= data_ptr;

        data_ptr+= chunk;
        remaining-= chunk;

        if (i < dma_desc_count - 1)
            dma_desc[i].qe.stqe_next= &dma_desc[i + 1];
        else
            dma_desc[i].qe.stqe_next= &dma_desc[0];
    }

    ESP_LOGI(TAG, "DMA descriptors created (%d)", dma_desc_count);

    /*
    ------------------------------------------------
    Attach descriptors to GDMA
    ------------------------------------------------
    */

    gdma_channel_handle_t chan= lcd.get_dma_channel();

    gdma_start(chan, (intptr_t)&dma_desc[0]);

    ESP_LOGI(TAG, "GDMA transfer started");

    return ESP_OK;
}

esp_err_t HUB75Driver::start() {

    ESP_LOGI(TAG, "Starting LCD engine");

    /* latch config into hardware */
    LCD_CAM.lcd_user.lcd_update= 1;

    /* start LCD TX */
    LCD_CAM.lcd_user.lcd_start= 1;

    return ESP_OK;
}

void HUB75Driver::stop() {
    LCD_CAM.lcd_user.lcd_start= 0;

    ESP_LOGI(TAG, "Panel refresh stopped");
}