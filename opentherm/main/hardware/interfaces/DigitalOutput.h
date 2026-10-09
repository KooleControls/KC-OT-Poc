#pragma once

// ──────────────────────────────────────────────────────────────
// Role interface: a control line that is on or off — a power switch, an enable.
// Polarity is the driver's business: on means on, whatever level that is on the pin.
// ──────────────────────────────────────────────────────────────

class DigitalOutput
{
public:
    virtual void Set(bool on) = 0;
    virtual bool IsOn() const = 0;
    virtual ~DigitalOutput() = default;
};
