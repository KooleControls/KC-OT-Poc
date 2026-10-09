#pragma once

#include "AppProvider.h"
#include "CommandEntry.h"
#include "interfaces/BoardProvider.h"
#include <cstdint>

// ──────────────────────────────────────────────────────────────
// The modem: moves OpenTherm frames between the two lines and the gateway, and does
// nothing else with them.
//
//   line -> gateway   every frame either line receives is pushed, as it arrives, on
//                     one stream this board opens to the gateway:
//                         {"type":"opentherm stream"}
//                     one JSON line per frame:
//                         {"side":"boiler","frame":3221291008,"ms":123456}
//                     `ms` is this board's clock when the frame was collected, so the
//                     gateway can measure reply times without the UART's latency in
//                     them. A line only knows a frame has ended once it has been idle
//                     for ManchesterLine::IDLE_US, so `ms` trails the stop bit by that
//                     much -- the same amount for every frame.
//
//   gateway -> line   `opentherm send side=<thermostat|boiler> frame=<uint32>` starts
//                     one frame on a line, and answers once it has started.
//
// What a frame MEANS -- parity, message type, data id, whether to answer it, what to
// answer -- is the gateway's, all of it. This board does not look inside a frame, so
// a gateway that wants to pass frames through, simulate either end, or sit in the
// middle and rewrite them needs nothing changed here.
// ──────────────────────────────────────────────────────────────
class ModemManager
{
    static constexpr const char* TAG = "ModemManager";

public:
    explicit ModemManager(AppProvider& app);

    ModemManager(const ModemManager&) = delete;
    ModemManager& operator=(const ModemManager&) = delete;
    ModemManager(ModemManager&&) = delete;
    ModemManager& operator=(ModemManager&&) = delete;

    void Init();

    /// Collect what the lines received and push it to the gateway. From the main loop,
    /// every pass: a line holds one finished frame, and the next one overwrites it.
    void Poll();

private:
    static constexpr int SIDES = 2;

    struct LineCounters
    {
        uint32_t received  = 0;   // decoded off the line
        uint32_t forwarded = 0;   // of those, pushed to the gateway
        uint32_t sent      = 0;   // started on the line for the gateway
        uint32_t busy      = 0;   // sends refused: the previous frame was still going out
    };

    AppProvider& app_;
    LineCounters counters_[SIDES];

    /// The stream's channel id, -1 until it is open. LinkManager opens and reopens it.
    int32_t stream_ = -1;

    void Forward(OpenThermSide side, uint32_t frame);

    // ── Commands ──
    CommandResult Cmd_Send(CommandContext& ctx);
    CommandResult Cmd_Status(CommandContext& ctx);

    static CommandEntry sendCommand_;
    static CommandEntry statusCommand_;
};
