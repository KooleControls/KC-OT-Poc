#include "LinkManager.h"
#include "BoardContext.h"
#include "StruxProvider.h"
#include "CommandManager.h"
#include "Connection.h"
#include "Platform.h"
#include "esp_log.h"
#include <cstring>

namespace {

// The UART is authenticated by being a wire: the only thing at the other end of it is
// the gateway this board is plugged into. So it gates nothing and never sees `auth` --
// the same position the relay pipe takes for its own reason, and a policy difference
// rather than a structural one (see the gateway's AuthGate).
class LinkGate final : public ConnectionAuth
{
public:
    bool Allows(const char*) const { return true; }
    void authenticate(const char*) override {}
    bool isAuthed() const override { return true; }
};

const char* PhaseName(ConnectionState::Phase phase)
{
    switch (phase)
    {
    case ConnectionState::Phase::Handshake: return "handshake";
    case ConnectionState::Phase::Ready:     return "ready";
    case ConnectionState::Phase::Failed:    return "failed";
    }
    return "unknown";
}

} // namespace

CommandEntry LinkManager::statusCommand_{
    "link status", &InvokeCommand<&LinkManager::Cmd_Status>,
    "Report the serial link to the gateway: handshake phase, which half of the "
    "channel ids this board allocates from, and frame counters since boot - "
    "corrupt frames point at noise or a baud-rate mismatch."
};

LinkManager::LinkManager(AppProvider& app)
    : app_(app)
{
}

SerialTransport LinkManager::Transport()
{
    return SerialTransport(app_.getBoard().GetGatewaySerial(), decoder_,
                           encoded_, sizeof(encoded_), INBOUND_WINDOW, counters_);
}

void LinkManager::Init()
{
    app_.getStrux().getCommandManager().Register(this, { &statusCommand_ });

    // Speak first, as both peers must: whichever of the two boards is up first, the
    // other one's hello is the one that settles it.
    SendHandshake();

    ESP_LOGI(TAG, "Initialized");
}

void LinkManager::SendHandshake()
{
    state_.Reset();
    SerialTransport link = Transport();
    protocol::SendHandshake(state_, link);
    lastHandshakeMs_ = Millis();
}

void LinkManager::Poll()
{
    // Retry only while waiting. A board that is Ready answers a fresh handshake from
    // the gateway inside Connection, so retrying from Ready would turn two Ready peers
    // into a pair that re-handshake each other forever.
    if (state_.phase == ConnectionState::Phase::Handshake
        && Millis() - lastHandshakeMs_ >= HANDSHAKE_RETRY_MS)
        SendHandshake();

    SerialTransport link = Transport();
    const uint8_t* frame = nullptr;
    size_t len = 0;
    while (link.Poll(frame, len))
        HandleFrame(frame, len);
}

void LinkManager::HandleFrame(const uint8_t* frame, size_t len)
{
    const uint16_t id    = channel::readU16(frame);
    const uint8_t  flags = frame[2];

    SerialTransport link = Transport();
    LinkGate gate;
    protocol::Connection<CommandManager, LinkGate> connection(
        state_, link, app_.getStrux().getCommandManager(), gate,
        reply_, REPLY_WINDOW, inbound_, sizeof(inbound_));
    connection.OnFrame(id, flags, frame + channel::HEADER_LEN, len - channel::HEADER_LEN);

    // Nothing to reconnect on a wire. A gateway that speaks another version is answered
    // the moment it is reflashed, because its first handshake comes through here.
    if (state_.phase == ConnectionState::Phase::Failed)
        ESP_LOGE(TAG, "handshake failed - waiting for the gateway to restart");
}

bool LinkManager::Push(int32_t& stream, const char* envelope, const void* data, size_t len)
{
    if (!IsReady() || len > PUSH_MAX)
        return false;

    SerialTransport link = Transport();

    // Ours only if it is still the Passive entry we opened. Anything else -- gone
    // after a RESET, or the table cleared by a re-handshake that may have handed this
    // id to the gateway's half -- means opening a fresh one.
    const ChannelTable::Entry* e =
        stream >= 0 ? state_.channels.Find(static_cast<uint16_t>(stream)) : nullptr;
    if (e == nullptr || e->state != ChannelTable::State::Passive)
    {
        stream = -1;
        if (!protocol::OpenPassiveChannel(state_, link, envelope, stream))
            return false;
    }

    uint8_t frame[channel::HEADER_LEN + PUSH_MAX];
    channel::writeHeader(frame, static_cast<uint16_t>(stream), 0);
    memcpy(frame + channel::HEADER_LEN, data, len);
    return link.SendRaw(frame, channel::HEADER_LEN + len);
}

CommandResult LinkManager::Cmd_Status(CommandContext& ctx)
{
    auto resp = ctx.reply.object();
    resp.field("phase", PhaseName(state_.phase));
    if (IsReady())
        resp.field("half", state_.lowHalf ? "low" : "high");

    auto frames = resp.object("frames");
    frames.field("in", counters_.framesIn);
    frames.field("out", counters_.framesOut);
    frames.field("corrupt", counters_.corrupt);
    frames.field("tooLong", counters_.tooLong);
    frames.field("runt", counters_.runt);
    frames.field("sendFailed", counters_.sendFail);
    return CommandResult::Ok;
}
