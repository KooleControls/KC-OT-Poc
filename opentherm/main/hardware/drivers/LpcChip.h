#pragma once

// ──────────────────────────────────────────────────────────────
// The LPC11U6x device header plus the bit names the drivers share. The header
// (third_party/LPC11U6x) gives the registers; the bits below are from UM10732.
// ──────────────────────────────────────────────────────────────

#include "LPC11U6x.h"
#include <cstdint>

namespace lpc
{
    // SYSCON->SYSAHBCLKCTRL: clock enables
    inline constexpr uint32_t CLK_GPIO   = 1u << 6;
    inline constexpr uint32_t CLK_CT32B1 = 1u << 10;
    inline constexpr uint32_t CLK_USART0 = 1u << 12;
    inline constexpr uint32_t CLK_WWDT   = 1u << 15;
    inline constexpr uint32_t CLK_IOCON  = 1u << 16;
    inline constexpr uint32_t CLK_RTC    = 1u << 30;
    inline constexpr uint32_t CLK_SCT0_1 = 1u << 31;

    // SYSCON->PRESETCTRL: a 1 releases the peripheral from reset
    inline constexpr uint32_t RST_SCT0 = 1u << 9;
    inline constexpr uint32_t RST_SCT1 = 1u << 10;

    // SYSCON->PDRUNCFG: a 1 powers the block down
    inline constexpr uint32_t PD_WDTOSC = 1u << 6;

    // IOCON pin settings
    inline constexpr uint32_t IOCON_FUNC0 = 0x0;
    inline constexpr uint32_t IOCON_FUNC1 = 0x1;
    inline constexpr uint32_t IOCON_FUNC2 = 0x2;
    inline constexpr uint32_t IOCON_PULLUP = 0x2 << 3;
    inline constexpr uint32_t IOCON_DIGITAL = 0x1 << 7;

    inline void EnableClock(uint32_t bits) { LPC_SYSCON->SYSAHBCLKCTRL |= bits; }
}
