#pragma once

#include <stdint.h>
#include <esp_err.h>
#include <esp_private/gdma.h>

class DMABuilder {
  public:
    DMABuilder();

    /*
    Build DMA signal stream
    */

    esp_err_t build();

    /*
    Return pointer to DMA buffer
    */

    uint16_t* get_buffer();

    /*
    Return buffer size
    */

    size_t get_buffer_size();

  private:
    /*
    Encode HUB75 protocol
    */

    void encode_row(int row);
    void encode_pixels();
    void encode_latch();
    void encode_oe();

  private:
    uint16_t* dma_buffer;

    size_t buffer_size;

    size_t write_index;
};