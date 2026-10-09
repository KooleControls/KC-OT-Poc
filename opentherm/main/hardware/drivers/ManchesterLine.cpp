#include "ManchesterLine.h"

namespace {

constexpr uint32_t EV_TIMEOUT = 1u << 0;   // match 0: no edge for IDLE_US
constexpr uint32_t EV_FALL    = 1u << 1;   // CTIN_3 falling, captures into CAP1
constexpr uint32_t EV_RISE    = 1u << 2;   // CTIN_3 rising, captures into CAP2

// Tolerance on a half-bit: a third of it either way, as the KC1246 used.
constexpr uint32_t MARGIN_US = ManchesterLine::HALF_BIT_US / 3;
constexpr uint32_t HALF_MIN = ManchesterLine::HALF_BIT_US - MARGIN_US;
constexpr uint32_t HALF_MAX = ManchesterLine::HALF_BIT_US + MARGIN_US;
constexpr uint32_t FULL_MIN = 2 * ManchesterLine::HALF_BIT_US - MARGIN_US;
constexpr uint32_t FULL_MAX = 2 * ManchesterLine::HALF_BIT_US + MARGIN_US;

constexpr uint8_t HALVES_PER_FRAME = 2 * ManchesterLine::FRAME_BITS;

// One line per SCT; the transmit timer serves all of them.
ManchesterLine* lines[2] = {};

/// How many half-bits a level that lasted `us` is: 1, 2, or 0 when it is neither.
uint8_t HalfBits(uint32_t us)
{
    if (us >= HALF_MIN && us <= HALF_MAX)
        return 1;
    if (us >= FULL_MIN && us <= FULL_MAX)
        return 2;
    return 0;
}

void TxTimerInit()
{
    static bool done = false;
    if (done)
        return;
    done = true;

    lpc::EnableClock(lpc::CLK_CT32B1);
    LPC_CT32B1->TCR = 2;                                       // hold in reset
    LPC_CT32B1->PR = SystemCoreClock / 1000000 - 1;            // count microseconds
    LPC_CT32B1->MR0 = ManchesterLine::HALF_BIT_US - 1;
    LPC_CT32B1->MCR = 3;                                       // interrupt and restart on MR0
    LPC_CT32B1->IR = 0x1F;
    NVIC_EnableIRQ(CT32B1_IRQn);
}

void TxTimerStart()
{
    if ((LPC_CT32B1->TCR & 1) == 0)
    {
        LPC_CT32B1->TCR = 2;
        LPC_CT32B1->TCR = 1;
    }
}

} // namespace

void ManchesterLine::Init()
{
    lpc::EnableClock(lpc::CLK_GPIO | lpc::CLK_IOCON | lpc::CLK_SCT0_1);
    lines[config_.sct] = this;

    // Output: idle before it becomes an output, so the line never glitches.
    TxWrite(0);
    *config_.txIocon = config_.txIoconValue;
    LPC_GPIO_PORT->DIR[config_.txPort] |= 1u << config_.txPin;
    TxTimerInit();

    // Input: the SCT counts microseconds from the last edge. Both edges and the idle
    // timeout restart the count (LIMIT), so each capture is the length of one level.
    LPC_SYSCON->PRESETCTRL |= config_.sct == 0 ? lpc::RST_SCT0 : lpc::RST_SCT1;
    *config_.rxIocon = config_.rxIoconValue;

    LPC_SCT0_Type* sct = Sct();
    sct->CTRL |= (SystemCoreClock / 1000000 - 1) << 5;        // prescaler: 1 MHz
    sct->REGMODE = (1 << 1) | (1 << 2);                         // registers 1 and 2 capture

    sct->MATCH0 = IDLE_US;
    sct->MATCHREL0 = IDLE_US;
    sct->EV0_STATE = 1;
    sct->EV0_CTRL = (0 << 0) | (1 << 12) | (1 << 14);           // match 0 only

    sct->EV1_STATE = 1;
    sct->EV1_CTRL = (3 << 6) | (2 << 10) | (2 << 12) | (1 << 14);   // CTIN_3, falling edge

    sct->EV2_STATE = 1;
    sct->EV2_CTRL = (3 << 6) | (1 << 10) | (2 << 12) | (1 << 14);   // CTIN_3, rising edge

    sct->CAPCTRL1 = EV_FALL;
    sct->CAPCTRL2 = EV_RISE;
    sct->LIMIT = EV_TIMEOUT | EV_FALL | EV_RISE;
    sct->EVEN = EV_TIMEOUT | EV_FALL | EV_RISE;
    sct->EVFLAG = EV_TIMEOUT | EV_FALL | EV_RISE;

    NVIC_EnableIRQ(SCT0_1_IRQn);
    sct->CTRL &= ~(1u << 2);                                    // run
}

// ──────────────────────────────────────────────────────────────
// Receive
// ──────────────────────────────────────────────────────────────

bool ManchesterLine::TryReceive(uint32_t& frame)
{
    if (!frameReady_)
        return false;

    __disable_irq();
    frame = frame_;
    frameReady_ = false;
    __enable_irq();
    return true;
}

void ManchesterLine::OnSctInterrupt()
{
    LPC_SCT0_Type* sct = Sct();
    const uint32_t flags = sct->EVFLAG;

    // A falling edge ends a physical high, a rising edge a physical low; the inversion
    // turns that into the logical level.
    if (flags & EV_FALL)
    {
        sct->EVFLAG = EV_FALL;
        OnEdge(config_.rxInverted ? 0 : 1, sct->CAP1 & 0xFFFF);
    }
    if (flags & EV_RISE)
    {
        sct->EVFLAG = EV_RISE;
        OnEdge(config_.rxInverted ? 1 : 0, sct->CAP2 & 0xFFFF);
    }
    if (flags & EV_TIMEOUT)
    {
        sct->EVFLAG = EV_TIMEOUT;
        OnIdle();
    }
}

void ManchesterLine::OnEdge(uint8_t levelBefore, uint32_t us)
{
    if (rxWaitIdle_)
        return;

    if (!rxActive_)
    {
        // The line idles low, so a frame starts with the rise into its start bit. What
        // came before that edge is idle time, not a half-bit.
        if (levelBefore == 0)
        {
            rxActive_ = true;
            rxHalf_ = -1;
            rxBits_ = 0;
            rxAcc_ = 0;
        }
        return;
    }

    const uint8_t halves = HalfBits(us);
    if (halves == 0)
    {
        Fail();
        return;
    }
    for (uint8_t i = 0; i < halves; i++)
    {
        if (!AddHalf(levelBefore))
        {
            Fail();
            return;
        }
    }
}

void ManchesterLine::OnIdle()
{
    if (rxActive_)
    {
        // The stop bit is high-then-low and the line stays low after it, so its second
        // half never ends in an edge: it is the idle itself.
        if (rxHalf_ == 1)
            AddHalf(0);

        const bool start = (rxAcc_ >> (FRAME_BITS - 1)) & 1;
        const bool stop = rxAcc_ & 1;
        if (rxBits_ == FRAME_BITS && start && stop)
        {
            if (frameReady_)
                overruns_ = overruns_ + 1;
            frame_ = static_cast<uint32_t>(rxAcc_ >> 1);
            frameReady_ = true;
        }
        else
        {
            errors_ = errors_ + 1;
        }
    }

    rxActive_ = false;
    rxWaitIdle_ = false;
}

bool ManchesterLine::AddHalf(uint8_t level)
{
    if (rxHalf_ < 0)
    {
        rxHalf_ = static_cast<int8_t>(level);
        return true;
    }

    // Two equal halves are not a bit.
    if (rxHalf_ == level)
        return false;

    rxAcc_ = (rxAcc_ << 1) | static_cast<uint64_t>(rxHalf_);
    rxHalf_ = -1;
    rxBits_++;
    return rxBits_ <= FRAME_BITS;
}

void ManchesterLine::Fail()
{
    errors_ = errors_ + 1;
    rxActive_ = false;
    rxWaitIdle_ = true;
}

// ──────────────────────────────────────────────────────────────
// Send
// ──────────────────────────────────────────────────────────────

bool ManchesterLine::Send(uint32_t frame)
{
    if (txPending_ || txBusy_)
        return false;

    txBits_ = (1ull << (FRAME_BITS - 1)) | (static_cast<uint64_t>(frame) << 1) | 1ull;
    txPending_ = true;
    TxTimerStart();
    return true;
}

bool ManchesterLine::OnTxTick()
{
    if (txPending_)
    {
        txPending_ = false;
        txBusy_ = true;
        txHalf_ = 0;
        TxWrite(TxLevel(0));
        return true;
    }

    if (!txBusy_)
        return false;

    txHalf_++;
    if (txHalf_ < HALVES_PER_FRAME)
    {
        TxWrite(TxLevel(txHalf_));
        return true;
    }

    TxWrite(0);   // idle
    txBusy_ = false;
    return false;
}

uint8_t ManchesterLine::TxLevel(uint8_t half) const
{
    const uint8_t bit = (txBits_ >> (FRAME_BITS - 1 - half / 2)) & 1;
    return (half % 2 == 0) ? bit : !bit;   // 1 = high then low, 0 = low then high
}

void ManchesterLine::TxWrite(uint8_t level)
{
    const bool high = level ^ config_.txInverted;
    if (high)
        LPC_GPIO_PORT->SET[config_.txPort] = 1u << config_.txPin;
    else
        LPC_GPIO_PORT->CLR[config_.txPort] = 1u << config_.txPin;
}

// ──────────────────────────────────────────────────────────────
// Interrupt handlers
// ──────────────────────────────────────────────────────────────

extern "C" void SCT0_1_IRQHandler(void)
{
    for (ManchesterLine* line : lines)
        if (line != nullptr)
            line->OnSctInterrupt();
}

extern "C" void CT32B1_IRQHandler(void)
{
    LPC_CT32B1->IR = 1;

    bool busy = false;
    for (ManchesterLine* line : lines)
        if (line != nullptr)
            busy |= line->OnTxTick();

    if (!busy)
        LPC_CT32B1->TCR = 0;   // nothing left to send: stop ticking
}
