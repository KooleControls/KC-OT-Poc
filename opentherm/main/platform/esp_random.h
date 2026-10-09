#pragma once
// ESP-IDF's random source, for the code shared with the gateway.
//
// This chip has no hardware RNG. The one caller is the channel handshake's nonce,
// which only has to differ from the gateway's -- and that side draws from the ESP32's
// RNG -- so a generator stirred with the microsecond clock is enough. It is NOT for
// anything secret.

#include <cstdint>

uint32_t esp_random();
