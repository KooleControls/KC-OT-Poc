#pragma once

#include "LpcChip.h"

// ──────────────────────────────────────────────────────────────
// The windowed watchdog, clocked from the watchdog oscillator (600 kHz / 8 = 75 kHz,
// then the timer's own fixed /4). Resets the chip when it is not fed in time; once
// started it cannot be stopped.
//
// It keeps running while a debugger has the core halted, so a long stop at a
// breakpoint resets the board.
// ──────────────────────────────────────────────────────────────
class LpcWatchdog
{
public:
    void Init(uint32_t timeoutSeconds)
    {
        constexpr uint32_t FREQSEL_600KHZ = 0x1;
        constexpr uint32_t DIVSEL = 3;                              // / (2 x (1 + 3))
        constexpr uint32_t TICKS_PER_SECOND = 600000 / 8 / 4;
        constexpr uint32_t MOD_WDEN = 0x01;
        constexpr uint32_t MOD_WDRESET = 0x02;

        lpc::EnableClock(lpc::CLK_WWDT);
        LPC_SYSCON->WDTOSCCTRL = (FREQSEL_600KHZ << 5) | DIVSEL;
        LPC_SYSCON->PDRUNCFG &= ~lpc::PD_WDTOSC;                     // power the oscillator

        LPC_WWDT->CLKSEL = 1;                                        // watchdog oscillator
        LPC_WWDT->TC = TICKS_PER_SECOND * timeoutSeconds;
        LPC_WWDT->MOD = MOD_WDEN | MOD_WDRESET;
        Feed();                                                       // starts it
    }

    void Feed()
    {
        __disable_irq();          // the two writes must not be split
        LPC_WWDT->FEED = 0xAA;
        LPC_WWDT->FEED = 0x55;
        __enable_irq();
    }
};
