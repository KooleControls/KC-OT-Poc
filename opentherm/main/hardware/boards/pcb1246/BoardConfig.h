#pragma once

#include "drivers/LpcChip.h"

// ──────────────────────────────────────────────────────────────
// BoardContext configuration — PCB1246, the OpenTherm plug-in board
// (LPC11U68JBD100, Cortex-M0+, 256 KB flash, 32 KB SRAM0). Pin assignments,
// functions and polarities as the KC1246 firmware has them.
// ──────────────────────────────────────────────────────────────

namespace BoardConfig
{
    // ── UART0: the link to the PCB1245 gateway ───────────────
    // RX PIO0_18, TX PIO0_19, both function 1.
    inline constexpr uint32_t GATEWAY_BAUD = 57600;
    inline constexpr uint32_t GATEWAY_RX_IOCON = lpc::IOCON_FUNC1 | lpc::IOCON_PULLUP;
    inline constexpr uint32_t GATEWAY_TX_IOCON = lpc::IOCON_FUNC1;

    // ── OpenTherm, thermostat side (Manchester pair 1) ───────
    // In:  PIO1_30 = SCT0_IN3 (function 2), inverted.
    // Out: PIO0_16 as GPIO (function 0), inverted.
    inline constexpr uint8_t THERMOSTAT_SCT = 0;
    inline constexpr uint32_t THERMOSTAT_RX_IOCON = lpc::IOCON_FUNC2;
    inline constexpr bool THERMOSTAT_RX_INVERTED = true;
    inline constexpr uint32_t THERMOSTAT_TX_IOCON = lpc::IOCON_FUNC0;
    inline constexpr uint8_t THERMOSTAT_TX_PORT = 0;
    inline constexpr uint8_t THERMOSTAT_TX_PIN = 16;
    inline constexpr bool THERMOSTAT_TX_INVERTED = true;

    // ── OpenTherm, boiler side (Manchester pair 2) ───────────
    // In:  PIO2_15 = SCT1_IN3 (function 1), not inverted.
    // Out: PIO0_14 as GPIO (function 1 on this pin), inverted.
    inline constexpr uint8_t BOILER_SCT = 1;
    inline constexpr uint32_t BOILER_RX_IOCON = lpc::IOCON_FUNC1;
    inline constexpr bool BOILER_RX_INVERTED = false;
    inline constexpr uint32_t BOILER_TX_IOCON = lpc::IOCON_FUNC1;
    inline constexpr uint8_t BOILER_TX_PORT = 0;
    inline constexpr uint8_t BOILER_TX_PIN = 14;
    inline constexpr bool BOILER_TX_INVERTED = true;

    // ── Thermostat power switch: PIO1_21 as GPIO, active high ─
    inline constexpr uint32_t THERMOSTAT_POWER_IOCON = lpc::IOCON_FUNC0;
    inline constexpr uint8_t THERMOSTAT_POWER_PORT = 1;
    inline constexpr uint8_t THERMOSTAT_POWER_PIN = 21;

    // ── Watchdog ─────────────────────────────────────────────
    inline constexpr uint32_t WATCHDOG_TIMEOUT_S = 15;
}
