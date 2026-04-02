# LED Matrix DMA Rendering Engine (ESP32-S3)

## Overview

This project implements a **high-performance HUB75 LED matrix driver** for ESP32-S3 using a **fully hardware-driven pipeline**:

```
TIMER → GDMA → LCD PARALLEL → HUB75 PANEL
```

The goal is to eliminate CPU involvement during pixel transmission and achieve **maximum possible refresh rate and signal stability**.

## Core Idea

Traditional HUB75 drivers rely on CPU loops or interrupts to shift pixel data. This approach does not scale to high frequencies.

This project instead builds a **deterministic hardware pipeline**:

1. **Timer (GPTimer)**
   Generates precise periodic events.

2. **GDMA (General DMA)**
   Triggered by the timer to transfer prepared data.

3. **LCD Parallel Peripheral**
   Converts memory into a high-speed parallel signal (RGB + control lines).

4. **HUB75 Panel**
   Receives clocked data and displays it.

---

## Final Architecture

```
                +------------------+      +------------------+
                |   frame_timer    |      |       CPU        |
                |------------------|      |------------------|
                | GPTimer events   |      |frame instructions|
                +--------+---------+      +--------+---------+
                         |                         |
                         v                         v                          
                +------------------+      +------------------+
                |   HUB75 Driver   |      |   dma_builder    |
                |------------------| <--- |------------------| 
                | LCD + GDMA       |      | Frame assembly   |     
                | Clock generation |      |                  |
                +--------+---------+      +------------------+
                         |
                         v
                +------------------+
                |   HUB75 Panel    |
                +------------------+
```

## Module Responsibilities

### 1. `frame_timer`

**Purpose:**
Acts as the **master timing source** for the entire system.

**Responsibilities:**

* Configure GPTimer at compile-time frequency
* Generate periodic events
* (Future) trigger GDMA directly via hardware

**Key Design Choice:**

* No GPIO toggling
* No runtime logic
* Minimal ISR (event placeholder only)

### 2. `dma_builder`

**Purpose:**
Prepares memory buffers for DMA transmission.

**Responsibilities:**

* Convert logical pixel data into HUB75 signal format
* Encode:

  * RGB data
  * Row address lines (A, B, C, D)
  * LAT / OE timing
* Produce DMA-ready buffers

**Key Principle:**

> CPU builds frames, hardware transmits them.


### 3. `hub75_driver`

**Purpose:**
Handles **data transmission to the panel**.


**Responsibilities:**

* Configure LCD parallel peripheral
* Configure GDMA channel
* Link DMA buffers to LCD engine
* Start/stop transmission
* Handle scan sequencing

**Key Role:**
This is the **hardware execution engine**.

### 4. `main.cpp`

**Purpose:**
Defines application-level behavior.

**Responsibilities:**

* Initialize all modules
* Generate visual patterns
* Feed data to `dma_builder`
* Control animation logic


## Project Structure

```
.
├── include
│   ├── debug_config.h     // Compile-time logging control
│   └── panel_config.h     // Hardware configuration (source of truth)
│
├── lib
│   ├── dma_builder        // Frame → DMA buffer conversion
│   ├── frame_timer        // Master timing source
│   └── hub75_driver       // LCD + GDMA transmission engine
│
├── src
│   └── main.cpp           // Application logic
```


## End Goal Demo

A deterministic visual test:

1. A **single row moves downward** across the panel.
2. When it reaches the bottom:

   * It wraps to the top.
   * A **32-bit frame counter** increments.
3. The bottom row displays the counter in **binary across 32 green pixels**.

This validates:

* Row addressing correctness
* Frame timing consistency
* Data integrity across DMA pipeline