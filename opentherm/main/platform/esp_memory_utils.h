#pragma once
// ESP-IDF's "is this pointer in flash" check, for the code shared with the gateway.

#include <cstdint>

/// True when `p` points into the image's flash — where string literals live.
/// The range comes from the linker script (__flash_start / __flash_end).
bool esp_ptr_in_drom(const void* p);
