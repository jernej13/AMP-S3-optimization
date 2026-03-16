#include "dma_builder.h"

#include <esp_log.h>
#include <esp_heap_caps.h>

#include "panel_config.h"

static const char* TAG= "dma_builder";

DMABuilder::DMABuilder() {
    dma_buffer= nullptr;
    buffer_size= 0;
    write_index= 0;
}

esp_err_t DMABuilder::build() {
    /*
    Allocate DMA memory
    */

    buffer_size= 8192;

    dma_buffer= (uint16_t*)heap_caps_malloc(buffer_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

    if (!dma_buffer) return ESP_ERR_NO_MEM;

    write_index= 0;

    /*
    Build signal stream
    */

    for (int row= 0; row < PANEL_SCAN_ROWS; row++) {
        encode_row(row);
    }

    ESP_LOGI(TAG, "DMA stream built (%d bytes)", buffer_size);

    return ESP_OK;
}

uint16_t* DMABuilder::get_buffer() {
    return dma_buffer;
}

size_t DMABuilder::get_buffer_size() {
    return buffer_size;
}

void DMABuilder::encode_row(int row) {
    /*
    Set row address
    */

    uint16_t row_bits= row << 6;

    /*
    Shift 32 pixels
    */

    for (int i= 0; i < PANEL_WIDTH; i++) {
        uint16_t word= row_bits;

        /*
        simple pattern
        */

        word|= (1 << 0); // R1
        word|= (1 << 3); // R2

        dma_buffer[write_index++]= word;
    }

    /*
    Latch
    */

    uint16_t latch= row_bits | (1 << 11);

    dma_buffer[write_index++]= latch;

    /*
    Enable row (OE low)
    */

    for (int i= 0; i < 50; i++) {
        uint16_t oe_word= row_bits;

        dma_buffer[write_index++]= oe_word;
    }

    /*
    Disable row (OE high)
    */

    uint16_t disable= row_bits | (1 << 12);

    dma_buffer[write_index++]= disable;
}