#pragma once

#include "Stream.h"
#include <cstdint>

// ──────────────────────────────────────────────────────────────
// Role interface: a serial line. A byte Stream that never blocks — read() returns
// what has arrived, write() queues what fits — plus the line's speed.
// ──────────────────────────────────────────────────────────────

class SerialPort : public Stream
{
public:
    virtual void SetBaudRate(uint32_t baud) = 0;
};
