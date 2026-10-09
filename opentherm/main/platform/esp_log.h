#pragma once

// ──────────────────────────────────────────────────────────────
// ESP-IDF's logging macros, for the code shared with the gateway.
//
// Lines go to PlatformLogWrite(), which does nothing until a board provides a strong
// definition (a UART, most likely). Debug and verbose are compiled out.
// ──────────────────────────────────────────────────────────────

#include <cstddef>

enum class LogLevel { Error, Warn, Info };

void PlatformLog(LogLevel level, const char* tag, const char* fmt, ...)
    __attribute__((format(printf, 3, 4)));

/// Where finished lines go, newline included. Weak and empty by default.
void PlatformLogWrite(const char* line, size_t len);

#define ESP_LOGE(tag, fmt, ...) PlatformLog(LogLevel::Error, tag, fmt, ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...) PlatformLog(LogLevel::Warn,  tag, fmt, ##__VA_ARGS__)
#define ESP_LOGI(tag, fmt, ...) PlatformLog(LogLevel::Info,  tag, fmt, ##__VA_ARGS__)
#define ESP_LOGD(tag, fmt, ...) do {} while (0)
#define ESP_LOGV(tag, fmt, ...) do {} while (0)
