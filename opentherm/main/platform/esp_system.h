#pragma once
// ESP-IDF's restart, for the code shared with the gateway.

// The ESP-IDF headers bring assert() along, and the shared code relies on it.
#include <cassert>

[[noreturn]] void esp_restart();
