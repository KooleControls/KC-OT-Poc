#pragma once
// ESP-IDF's microsecond clock, for the code shared with the gateway.

#include <cstdint>

/// Microseconds since PlatformInit(). Built on the millisecond tick, so it wraps when
/// Millis() does, after 49 days.
int64_t esp_timer_get_time();
