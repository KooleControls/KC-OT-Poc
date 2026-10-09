#pragma once

#include <cstdint>

// ──────────────────────────────────────────────────────────────
// What ESP-IDF provides on the gateway and this chip has to provide itself: a
// millisecond tick, and a wall clock under libc's time() / settimeofday().
// ──────────────────────────────────────────────────────────────

/// Start the tick and the RTC. First thing in main(), before any layer.
void PlatformInit();

/// Milliseconds since PlatformInit(). Wraps after 49 days; compare with subtraction.
uint32_t Millis();
