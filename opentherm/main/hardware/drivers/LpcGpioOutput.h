#pragma once

#include "LpcChip.h"
#include "interfaces/DigitalOutput.h"

// ──────────────────────────────────────────────────────────────
// One LPC GPIO pin as an output role. The board passes the pin's IOCON register
// with the function that makes it a GPIO (FUNC0 on most pins, not all), the port and
// pin, and the polarity.
// ──────────────────────────────────────────────────────────────
class LpcGpioOutput final : public DigitalOutput
{
public:
    LpcGpioOutput(volatile uint32_t& iocon, uint32_t ioconValue, uint8_t port, uint8_t pin,
                  bool activeHigh, bool initiallyOn = false)
        : iocon_(iocon), ioconValue_(ioconValue), port_(port), pin_(pin),
          activeHigh_(activeHigh), on_(initiallyOn) {}

    /// After the GPIO and IOCON clocks are on.
    void Init()
    {
        Write(on_);
        iocon_ = ioconValue_;
        LPC_GPIO_PORT->DIR[port_] |= 1u << pin_;
    }

    void Set(bool on) override
    {
        on_ = on;
        Write(on);
    }

    bool IsOn() const override { return on_; }

private:
    volatile uint32_t& iocon_;
    const uint32_t ioconValue_;
    const uint8_t port_;
    const uint8_t pin_;
    const bool activeHigh_;
    bool on_;

    void Write(bool on)
    {
        if (on == activeHigh_)
            LPC_GPIO_PORT->SET[port_] = 1u << pin_;
        else
            LPC_GPIO_PORT->CLR[port_] = 1u << pin_;
    }
};
