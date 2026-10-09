#pragma once

#include "Transport.h"
#include "ChannelProtocol.h"
#include "SerialFraming.h"
#include "interfaces/SerialPort.h"
#include "Platform.h"
#include <cstring>

// Transport over a serial line (the third implementation -- see Transport.h): channel
// frames in SerialFraming's COBS + CRC, over a SerialPort that never blocks.
//
// The line is read in two places, and they are the same read. Between requests the
// main loop calls Poll(), which takes whatever bytes have arrived and returns as soon
// as there are no more. Mid-request a handler's Channel calls RecvChunk(), which is the
// same loop waiting for its own next chunk -- the only place on this board anything
// waits, and it waits only for the second chunk of a request, which the commands this
// board has never send.
//
// Owns no buffers. The decoder's and the encoder's are LinkManager's, as members: one
// link, one of each.
class SerialTransport final : public Transport
{
    // A chunk of the same request that has not arrived in this long is not coming.
    // Well inside the watchdog, which nothing feeds while this waits.
    static constexpr uint32_t RECV_TIMEOUT_MS = 2000;

    // The TX ring takes a frame in pieces; the UART interrupt drains it at the line's
    // speed. A frame that cannot be queued in this long means the UART has stopped.
    static constexpr uint32_t SEND_TIMEOUT_MS = 500;

    SerialPort&         port_;
    SerialFrameDecoder& decoder_;
    uint8_t*            txBuf_;
    size_t              txCap_;
    size_t              inboundLimit_;

public:
    struct Counters
    {
        uint32_t framesIn  = 0;
        uint32_t framesOut = 0;
        uint32_t corrupt   = 0;   // failed CRC or COBS: noise, a dropped byte
        uint32_t tooLong   = 0;   // bigger than the decoder's buffer
        uint32_t runt      = 0;   // checked, but shorter than a channel header
        uint32_t sendFail  = 0;   // could not be queued in SEND_TIMEOUT_MS
    };

    /// `txBuf` holds `txCap` bytes and must fit EncodedMax() of the largest frame sent.
    /// `inboundLimit` is the payload the decoder's buffer holds, for diagnostics only.
    SerialTransport(SerialPort& port, SerialFrameDecoder& decoder,
                    uint8_t* txBuf, size_t txCap, size_t inboundLimit, Counters& counters)
        : port_(port), decoder_(decoder), txBuf_(txBuf), txCap_(txCap),
          inboundLimit_(inboundLimit), counters_(counters) {}

    size_t InboundLimit() const override { return inboundLimit_; }

    bool SendRaw(const uint8_t* frame, size_t len) override
    {
        if (serial_framing::EncodedMax(len) > txCap_)
        {
            counters_.sendFail++;
            return false;
        }

        const size_t n = serial_framing::Encode(frame, len, txBuf_);
        size_t sent = 0;
        const uint32_t start = Millis();
        while (sent < n)
        {
            sent += port_.write(txBuf_ + sent, n - sent);
            if (sent < n && Millis() - start > SEND_TIMEOUT_MS)
            {
                counters_.sendFail++;
                return false;
            }
        }
        counters_.framesOut++;
        return true;
    }

    /// The next complete frame, if the bytes that have arrived finish one; never
    /// waits. `frame` is the whole channel frame, header included, and stays valid
    /// until the line is read again.
    bool Poll(const uint8_t*& frame, size_t& len)
    {
        uint8_t b;
        while (port_.read(&b, 1) == 1)
        {
            switch (decoder_.Push(b))
            {
            case SerialFrameDecoder::Result::None:
                break;
            case SerialFrameDecoder::Result::Corrupt:
                counters_.corrupt++;
                break;
            case SerialFrameDecoder::Result::TooLong:
                counters_.tooLong++;
                break;
            case SerialFrameDecoder::Result::Frame:
                if (decoder_.Length() < channel::HEADER_LEN)
                {
                    counters_.runt++;
                    break;
                }
                counters_.framesIn++;
                frame = decoder_.Data();
                len = decoder_.Length();
                return true;
            }
        }
        return false;
    }

    int RecvChunk(uint8_t* buf, size_t cap, uint16_t* sid, uint8_t* flags) override
    {
        // tooLong only grows inside Poll, so a change across one call is a chunk this
        // link could not take -- consumed and discarded, the link still in step.
        const uint32_t start = Millis();
        for (;;)
        {
            const uint32_t tooLong = counters_.tooLong;
            const uint8_t* frame = nullptr;
            size_t len = 0;
            if (Poll(frame, len))
            {
                if (len > cap) return RECV_TOO_LONG;
                memcpy(buf, frame, len);
                *sid   = channel::readU16(buf);
                *flags = buf[2];
                return static_cast<int>(len - channel::HEADER_LEN);
            }
            if (counters_.tooLong != tooLong) return RECV_TOO_LONG;
            if (Millis() - start > RECV_TIMEOUT_MS) return RECV_EOF;
        }
    }

private:
    Counters& counters_;
};
