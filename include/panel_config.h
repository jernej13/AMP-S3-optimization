#pragma once

#include <stdint.h>

/*
========================================================
Panel Geometry
========================================================
*/

#define PANEL_WIDTH 32
#define PANEL_HEIGHT 32

/*
HUB75 panels multiplex rows.
A 32x32 panel typically scans 16 rows at a time.
*/

#define PANEL_SCAN_ROWS 16

/*
========================================================
Frame Timing
========================================================
Frame period in microseconds.

Example:
200 us = 5000 FPS internal refresh
*/

#define FRAME_TIME_US 200

/*
========================================================
GPIO Pin Mapping
(MatrixPortal ESP32-S3)
========================================================
*/

#define PIN_R1 42
#define PIN_G1 41
#define PIN_B1 40

#define PIN_R2 38
#define PIN_G2 39
#define PIN_B2 37

#define PIN_CLK 2
#define PIN_LAT 47
#define PIN_OE 14

#define PIN_A 45
#define PIN_B 36
#define PIN_C 48
#define PIN_D 35

/*
========================================================
Derived Constants
========================================================
*/

#define PANEL_PIXELS (PANEL_WIDTH * PANEL_HEIGHT)

/*
Rows driven per scan cycle
*/

#define PANEL_ROWS_PER_SCAN PANEL_SCAN_ROWS

/*
Pixels shifted per row
*/

#define PIXELS_PER_ROW PANEL_WIDTH

/*
========================================================
Signal Count
========================================================

We will drive these signals via the LCD peripheral.

RGB1 (3)
RGB2 (3)
Row address (4)
CLK
LAT
OE

Total: 12 signals
*/

#define HUB75_SIGNAL_COUNT 12