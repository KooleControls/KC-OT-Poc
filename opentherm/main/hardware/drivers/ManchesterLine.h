#pragma once

#include "LpcChip.h"
#include "interfaces/OpenThermLine.h"

// ──────────────────────────────────────────────────────────────
// An OpenTherm line in Manchester code: the modem itself.
//
// On the wire a frame is a start bit, 32 data bits and a stop bit, 1 ms each, and a
// bit is two half-bits: 1 = high then low, 0 = low then high. The line idles low.
// "High" and "low" here are logical; each direction has its own inversion for what the
// board's circuit does to them.
//
// RECEIVE: an SCT times the line. Every edge captures how long the level before it
// lasted, and restarts the count; 12 ms without an edge is the end of a frame. Each
// duration is one or two half-bits of the level that just ended, pairs of half-bits
// are bits, and at the end the frame must be exactly start + 32 + stop. All of it
// runs in the SCT interrupt; the main loop collects finished frames with TryReceive().
//
// SEND: one timer (CT32B1) ticks every half-bit for all lines, so every line's bits are
// on the same grid, and it only runs while some line has something to send. A frame
// handed to Send() starts on the next tick.
// ──────────────────────────────────────────────────────────────

struct ManchesterLineConfig
{
    uint8_t sct;                      // 0 or 1: the SCT whose CTIN_3 is the input pin
    volatile uint32_t* rxIocon;       // the input pin, set to its SCTx_IN3 function
    uint32_t rxIoconValue;
    bool rxInverted;
    volatile uint32_t* txIocon;       // the output pin, set to its GPIO function
    uint32_t txIoconValue;
    uint8_t txPort;
    uint8_t txPin;
    bool txInverted;
};

class ManchesterLine final : public OpenThermLine
{
public:
    static constexpr uint32_t HALF_BIT_US = 500;
    static constexpr uint32_t IDLE_US = 12000;       // no edge this long ends a frame
    static constexpr uint8_t FRAME_BITS = 34;        // start + 32 + stop

    explicit ManchesterLine(const ManchesterLineConfig& config) : config_(config) {}

    ManchesterLine(const ManchesterLine&) = delete;
    ManchesterLine& operator=(const ManchesterLine&) = delete;

    void Init();

    // ── OpenThermLine ────────────────────────────────────────
    bool Send(uint32_t frame) override;
    bool IsSending() const override { return txPending_ || txBusy_; }
    bool TryReceive(uint32_t& frame) override;
    uint32_t Errors() const override { return errors_; }
    uint32_t Overruns() const override { return overruns_; }

    // ── Interrupt side ───────────────────────────────────────
    void OnSctInterrupt();
    /// One half-bit tick. True while this line still has something to send.
    bool OnTxTick();

private:
    const ManchesterLineConfig config_;

    // Receive state: the SCT interrupt's alone, apart from the finished frame.
    bool rxActive_ = false;
    bool rxWaitIdle_ = false;       // after an error, ignore the rest of the frame
    int8_t rxHalf_ = -1;            // the first half of the bit in progress, -1 if none
    uint8_t rxBits_ = 0;
    uint64_t rxAcc_ = 0;

    volatile bool frameReady_ = false;
    volatile uint32_t frame_ = 0;
    volatile uint32_t errors_ = 0;
    volatile uint32_t overruns_ = 0;

    // Send state.
    volatile bool txPending_ = false;
    volatile bool txBusy_ = false;
    uint64_t txBits_ = 0;
    uint8_t txHalf_ = 0;

    LPC_SCT0_Type* Sct() const { return config_.sct == 0 ? LPC_SCT0 : LPC_SCT1; }

    void OnEdge(uint8_t levelBefore, uint32_t us);
    void OnIdle();
    bool AddHalf(uint8_t level);
    void Fail();

    uint8_t TxLevel(uint8_t half) const;
    void TxWrite(uint8_t level);
};
