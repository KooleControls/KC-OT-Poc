#pragma once

#include <cstdint>

// ──────────────────────────────────────────────────────────────
// Role interface: one OpenTherm line, at the level this board works at — whole
// frames in and out, nothing about what they mean.
//
// A frame is the 32 data bits; the start and stop bits around them are the line's
// business and never appear here. Parity, message types, data IDs: all of that is
// the gateway's.
// ──────────────────────────────────────────────────────────────

class OpenThermLine
{
public:
    /// Start sending `frame`. False while the previous one is still going out.
    virtual bool Send(uint32_t frame) = 0;

    virtual bool IsSending() const = 0;

    /// The frame that arrived since the last call, if one did. A frame that was not
    /// collected before the next one arrived is lost and counted in Overruns().
    virtual bool TryReceive(uint32_t& frame) = 0;

    /// Frames that did not decode: bad timing, bad Manchester, missing start/stop bit.
    virtual uint32_t Errors() const = 0;

    virtual uint32_t Overruns() const = 0;

    virtual ~OpenThermLine() = default;
};
