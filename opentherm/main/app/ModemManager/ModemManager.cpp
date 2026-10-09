#include "ModemManager.h"
#include "BoardContext.h"
#include "StruxProvider.h"
#include "CommandManager.h"
#include "LinkManager.h"
#include "Platform.h"
#include "esp_log.h"
#include <cstdio>
#include <cstring>

namespace {

constexpr const char* STREAM_ENVELOPE = "{\"type\":\"opentherm stream\"}\n";

constexpr OpenThermSide SIDE_LIST[] = { OpenThermSide::Thermostat, OpenThermSide::Boiler };

const char* SideName(OpenThermSide side)
{
    switch (side)
    {
    case OpenThermSide::Thermostat: return "thermostat";
    case OpenThermSide::Boiler:     return "boiler";
    }
    return "unknown";
}

bool ParseSide(const char* name, OpenThermSide& out)
{
    for (OpenThermSide side : SIDE_LIST)
        if (strcmp(name, SideName(side)) == 0)
        {
            out = side;
            return true;
        }
    return false;
}

constexpr CommandArg<const char*> sideArg{
    "side", "Which line: 'thermostat' (the room thermostat; this board plays the "
            "boiler towards it) or 'boiler' (this board plays the thermostat).",
    10 };

constexpr CommandArg<uint32_t> frameArg{
    "frame", "The 32 frame bits exactly as they go on the line, parity bit included "
             "- nothing is added or checked. Decimal or 0x-prefixed hex." };

} // namespace

CommandEntry ModemManager::sendCommand_{
    "opentherm send", &InvokeCommand<&ModemManager::Cmd_Send>,
    "Start one frame on an OpenTherm line. Answers once it has started; it takes "
    "34 ms on the line, and a send while the previous one is still going out "
    "answers ok=false.",
    { &sideArg, &frameArg }
};

CommandEntry ModemManager::statusCommand_{
    "opentherm status", &InvokeCommand<&ModemManager::Cmd_Status>,
    "Per-line frame counters since boot: received, forwarded to the gateway, sent, "
    "sends refused as busy, and the line's own decode errors and overruns."
};

ModemManager::ModemManager(AppProvider& app)
    : app_(app)
{
}

void ModemManager::Init()
{
    app_.getStrux().getCommandManager().Register(this, { &sendCommand_, &statusCommand_ });
    ESP_LOGI(TAG, "Initialized");
}

void ModemManager::Poll()
{
    BoardContext& board = app_.getBoard();
    for (OpenThermSide side : SIDE_LIST)
    {
        uint32_t frame = 0;
        if (board.GetOpenThermLine(side).TryReceive(frame))
            Forward(side, frame);
    }
}

void ModemManager::Forward(OpenThermSide side, uint32_t frame)
{
    LineCounters& c = counters_[static_cast<int>(side)];
    c.received++;

    char record[64];
    const int n = snprintf(record, sizeof(record),
                           "{\"side\":\"%s\",\"frame\":%lu,\"ms\":%lu}\n",
                           SideName(side), static_cast<unsigned long>(frame),
                           static_cast<unsigned long>(Millis()));
    if (n <= 0 || static_cast<size_t>(n) >= sizeof(record))
        return;

    // Not queued when the link is down: a frame is only worth anything to a gateway
    // that is there to answer it within the line's timing, and a backlog replayed
    // later would be answers to questions nobody is still asking.
    if (app_.getLinkManager().Push(stream_, STREAM_ENVELOPE, record, static_cast<size_t>(n)))
        c.forwarded++;
}

// ──────────────────────────────────────────────────────────────
// Commands
// ──────────────────────────────────────────────────────────────

CommandResult ModemManager::Cmd_Send(CommandContext& ctx)
{
    auto resp = ctx.reply.object();

    // Which names exist is meaning, not form: the framework only knows `side` is a
    // string. So an unknown one is a reply, not a refusal.
    OpenThermSide side;
    if (!ParseSide(ctx.arg(sideArg), side))
    {
        resp.field("ok", false);
        resp.field("error", "unknown side");
        return CommandResult::Ok;
    }

    LineCounters& c = counters_[static_cast<int>(side)];
    if (!app_.getBoard().GetOpenThermLine(side).Send(ctx.arg(frameArg)))
    {
        c.busy++;
        resp.field("ok", false);
        resp.field("error", "busy");
        return CommandResult::Ok;
    }

    c.sent++;
    resp.field("ok", true);
    return CommandResult::Ok;
}

CommandResult ModemManager::Cmd_Status(CommandContext& ctx)
{
    BoardContext& board = app_.getBoard();
    auto resp = ctx.reply.object();

    for (OpenThermSide side : SIDE_LIST)
    {
        const LineCounters& c = counters_[static_cast<int>(side)];
        const OpenThermLine& line = board.GetOpenThermLine(side);

        auto o = resp.object(SideName(side));
        o.field("received", c.received);
        o.field("forwarded", c.forwarded);
        o.field("sent", c.sent);
        o.field("busy", c.busy);
        o.field("errors", line.Errors());
        o.field("overruns", line.Overruns());
        o.field("sending", line.IsSending());
    }
    return CommandResult::Ok;
}
