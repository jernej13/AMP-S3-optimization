#pragma once

/*
============================================================
LCD Parallel Driver (ESP32-S3)
============================================================

This module configures the ESP32-S3 LCD_CAM peripheral in
parallel (i8080 style) mode so it can drive HUB75 panels.

Responsibilities:

- Enable LCD_CAM peripheral
- Configure LCD clock
- Route GPIO signals to LCD data bus
- Setup GDMA channel for future transfers

This layer DOES NOT handle:

- Framebuffers
- DMA descriptor chains
- HUB75 protocol logic

Those will be implemented in higher layers.

This file only exposes a minimal hardware abstraction.
*/

#include <stdint.h>
#include <esp_err.h>
#include <esp_private/gdma.h>

class LCDParallel {
  public:
    /*
    ============================================================
    Constructor
    ============================================================
    */

    LCDParallel();

    /*
    ============================================================
    Initialize LCD peripheral and GDMA
    ============================================================
    */

    esp_err_t init();

    /*
    ============================================================
    Shutdown peripheral
    ============================================================
    */

    void shutdown();

    /*
    ============================================================
    Access GDMA channel
    ============================================================
    */

    gdma_channel_handle_t get_dma_channel() const;

  private:
    /*
    ============================================================
    Internal setup functions
    ============================================================
    */

    void configure_clock();
    void configure_lcd_mode();
    void configure_gpio();
    esp_err_t setup_gdma();

  private:
    gdma_channel_handle_t dma_channel;
};