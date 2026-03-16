#pragma once

#include <stdint.h>
#include <esp_err.h>

#include "lcd_parallel.h"
#include "dma_builder.h"

#include "soc/lldesc.h"

class HUB75Driver {
  public:
    /*
    ============================================================
    Constructor
    ============================================================
    */

    HUB75Driver();

    /*
    ============================================================
    Initialize driver
    ============================================================
    */

    esp_err_t init();

    /*
    ============================================================
    Start panel refresh
    ============================================================
    */

    esp_err_t start();

    /*
    ============================================================
    Stop panel refresh
    ============================================================
    */

    void stop();

  private:
    /*
    ============================================================
    Subsystem initialization
    ============================================================
    */

    esp_err_t init_lcd();
    esp_err_t init_dma();
    esp_err_t init_buffers();

  private:
    /*
    LCD hardware interface
    */

    LCDParallel lcd;

    /*
    DMA stream builder
    */

    DMABuilder builder;

    /*
    Framebuffer pointer
    (future double buffering)
    */

    uint8_t* frame_buffer;

    /*
    DMA descriptor chain
    */

    lldesc_t* dma_desc;

    size_t dma_desc_count;
};