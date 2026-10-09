#pragma once

#include <cstddef>
#include <cstdint>

// ──────────────────────────────────────────────────────────────
// A byte FIFO between one interrupt and the main loop: exactly one side pushes and
// the other pops, so each index has one writer and no lock is needed on a single
// core. N must be a power of two; the buffer holds N - 1 bytes.
// ──────────────────────────────────────────────────────────────
template <size_t N>
class RingBuffer
{
    static_assert(N >= 2 && (N & (N - 1)) == 0, "N must be a power of two");

public:
    bool Push(uint8_t b)
    {
        const size_t next = (head_ + 1) & (N - 1);
        if (next == tail_)
            return false;
        buf_[head_] = b;
        head_ = next;
        return true;
    }

    bool Pop(uint8_t& b)
    {
        if (tail_ == head_)
            return false;
        b = buf_[tail_];
        tail_ = (tail_ + 1) & (N - 1);
        return true;
    }

    size_t Count() const { return (head_ - tail_) & (N - 1); }
    size_t Free() const { return N - 1 - Count(); }
    bool Empty() const { return head_ == tail_; }

private:
    uint8_t buf_[N] = {};
    volatile size_t head_ = 0;   // written by the producer only
    volatile size_t tail_ = 0;   // written by the consumer only
};
