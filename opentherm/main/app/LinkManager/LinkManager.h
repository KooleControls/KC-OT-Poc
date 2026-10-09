#pragma once

#include "AppProvider.h"
#include "CommandEntry.h"
#include "ChannelProtocol.h"
#include "ConnectionState.h"
#include "SerialFraming.h"
#include "SerialTransport.h"
#include <cstddef>
#include <cstdint>

// ──────────────────────────────────────────────────────────────
// The link to the PCB1245 gateway: the Strux channel protocol over UART0.
//
// One Connection, the same one the gateway runs for a browser socket and for the relay
// pipe -- handshake, channel table, command dispatch -- over a SerialTransport instead
// of a socket. So every command this board registers is reachable from the gateway
// exactly as the gateway's own are reachable from a browser, and nothing above the
// transport knows it is a UART.
//
// An application manager rather than a Strux one because a UART is board hardware, and
// only the application layer has the board.
//
// No task: Poll() runs from the main loop, reads what has arrived and runs any command
// that completes, on the main loop's stack. A UART has no "connected" event either, so
// the handshake is resent until the gateway answers it (see Poll()).
// ──────────────────────────────────────────────────────────────
class LinkManager
{
    static constexpr const char* TAG = "LinkManager";

    // Payload per chunk, both directions. Purely local: a reply of any length goes out
    // window by window, and the gateway frames its own chunks to whatever it likes as
    // long as each fits INBOUND_WINDOW here. Every request this board answers fits one.
    static constexpr size_t REPLY_WINDOW   = 256;
    static constexpr size_t INBOUND_WINDOW = 256;

    static constexpr size_t FRAME_MAX = channel::HEADER_LEN + INBOUND_WINDOW;

    // While the gateway has not answered, say hello again this often. Each retry is a
    // fresh handshake -- to a gateway that already settled on the old one it reads as
    // this board restarting, which it answers with its own, and that is the answer
    // this one was waiting for.
    static constexpr uint32_t HANDSHAKE_RETRY_MS = 1000;

    // The largest record Push() frames, on the caller's stack.
    static constexpr size_t PUSH_MAX = 96;

public:
    explicit LinkManager(AppProvider& app);

    LinkManager(const LinkManager&) = delete;
    LinkManager& operator=(const LinkManager&) = delete;
    LinkManager(LinkManager&&) = delete;
    LinkManager& operator=(LinkManager&&) = delete;

    void Init();

    /// Read what has arrived and serve it. From the main loop, every pass.
    void Poll();

    /// Has the handshake settled? Until it has, nothing but the handshake may cross.
    bool IsReady() const { return state_.phase == ConnectionState::Phase::Ready; }

    /// Send `len` bytes on a stream this board pushes on, opening it first if it is not
    /// open: `stream` holds its channel id, -1 for none, and is the caller's so each one
    /// is its own. `envelope` names the stream to the gateway when it opens. False when
    /// the bytes did not go out -- no link yet, or the record is over PUSH_MAX.
    ///
    /// A stream the gateway RESETs is simply reopened by the next push, the same as the
    /// gateway's own log stream to the relay.
    bool Push(int32_t& stream, const char* envelope, const void* data, size_t len);

private:
    AppProvider& app_;

    ConnectionState state_;
    SerialTransport::Counters counters_;
    uint32_t lastHandshakeMs_ = 0;

    // Framing buffers, members rather than the stack: the stack is 4 KB and a command
    // runs on it.
    //   decoded_   what the decoder assembles a frame into; a request's first chunk
    //              is served straight out of it.
    //   inbound_   further chunks of the same request (Channel's continuation buffer).
    //   reply_     a reply chunk, assembled in place behind its header.
    //   encoded_   any frame, COBS-encoded for the wire.
    uint8_t decoded_[FRAME_MAX + serial_framing::CRC_LEN];
    uint8_t inbound_[FRAME_MAX];
    uint8_t reply_[channel::HEADER_LEN + REPLY_WINDOW];
    uint8_t encoded_[serial_framing::EncodedMax(channel::HEADER_LEN + REPLY_WINDOW)];

    SerialFrameDecoder decoder_{ decoded_, sizeof(decoded_) };

    /// The transport over the board's gateway UART. Holds nothing but references to
    /// the members above, so it is built where it is needed, as RelayManager builds
    /// its RelayTransport per frame.
    SerialTransport Transport();

    void SendHandshake();
    void HandleFrame(const uint8_t* frame, size_t len);

    // ── Commands ──
    CommandResult Cmd_Status(CommandContext& ctx);
    static CommandEntry statusCommand_;
};
