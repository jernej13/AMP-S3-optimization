#pragma once

#include <stdint.h>

/*
============================================================
HUB75 Driver Public API
============================================================
*/

#ifdef __cplusplus
extern "C" {
#endif

/*
Initialize hardware:
- GPIO
- DMA buffers
- LCD + GDMA backend
*/
void hub75_driver_init();

/*
Start DMA transmission (timer-driven)
*/
void hub75_driver_start();

/*
Stop transmission
*/
void hub75_driver_stop();

/*
Get pointer to active DMA buffer
*/
uint8_t* hub75_get_active_buffer();

/*
Swap front/back DMA buffers
*/
void hub75_swap_buffers();

#ifdef __cplusplus
}
#endif