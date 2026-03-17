#pragma once

/* ============================================================
   debug_config.h

   Set DEBUG to 1 to enable verbose per-step logging across
   all driver modules. Set to 0 for production — the compiler
   will eliminate all DBG_LOG calls with zero overhead.
   ============================================================ */

#define DEBUG 1

#if DEBUG
#include <esp_log.h>
#define DBG_LOG(tag, fmt, ...) ESP_LOGI(tag, fmt, ##__VA_ARGS__)
#else
#define DBG_LOG(tag, fmt, ...)                                                                     \
    do {                                                                                           \
    } while (0)
#endif