#pragma once
#include <Arduino.h>

// Leveled, tagged serial logger.
//   LOGE("WIFI", "connect failed: %d", code);
//   LOGI("API",  "got %d emails", n);
// Set min level at build time:  -D LOG_LEVEL=LOG_DEBUG   (default INFO)
// Disable color:                -D LOG_NO_COLOR

#define LOG_NONE  0
#define LOG_ERROR 1
#define LOG_WARN  2
#define LOG_INFO  3
#define LOG_DEBUG 4

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_INFO
#endif

#ifdef LOG_NO_COLOR
#define LOG_C_ERR ""
#define LOG_C_WRN ""
#define LOG_C_INF ""
#define LOG_C_DBG ""
#define LOG_C_RST ""
#else
#define LOG_C_ERR "\033[31m"   // red
#define LOG_C_WRN "\033[33m"   // yellow
#define LOG_C_INF "\033[32m"   // green
#define LOG_C_DBG "\033[90m"   // gray
#define LOG_C_RST "\033[0m"
#endif

// millis -> "  12.345" seconds, right-aligned
inline void log_stamp(char *out, size_t n) {
    uint32_t ms = millis();
    snprintf(out, n, "%6lu.%03lu", (unsigned long)(ms / 1000), (unsigned long)(ms % 1000));
}

#define LOG_PRINT(col, lvl, tag, fmt, ...)                              \
    do {                                                                \
        char _ts[16];                                                   \
        log_stamp(_ts, sizeof(_ts));                                    \
        Serial.printf(col "[%s %s %-5s] " fmt LOG_C_RST "\r\n",         \
                      _ts, tag, lvl, ##__VA_ARGS__);                    \
    } while (0)

#if LOG_LEVEL >= LOG_ERROR
#define LOGE(tag, fmt, ...) LOG_PRINT(LOG_C_ERR, "ERROR", tag, fmt, ##__VA_ARGS__)
#else
#define LOGE(tag, fmt, ...) do {} while (0)
#endif

#if LOG_LEVEL >= LOG_WARN
#define LOGW(tag, fmt, ...) LOG_PRINT(LOG_C_WRN, "WARN", tag, fmt, ##__VA_ARGS__)
#else
#define LOGW(tag, fmt, ...) do {} while (0)
#endif

#if LOG_LEVEL >= LOG_INFO
#define LOGI(tag, fmt, ...) LOG_PRINT(LOG_C_INF, "INFO", tag, fmt, ##__VA_ARGS__)
#else
#define LOGI(tag, fmt, ...) do {} while (0)
#endif

#if LOG_LEVEL >= LOG_DEBUG
#define LOGD(tag, fmt, ...) LOG_PRINT(LOG_C_DBG, "DEBUG", tag, fmt, ##__VA_ARGS__)
#else
#define LOGD(tag, fmt, ...) do {} while (0)
#endif
