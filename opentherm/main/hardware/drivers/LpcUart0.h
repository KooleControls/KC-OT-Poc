#pragma once

#include "LpcChip.h"
#include "RingBuffer.h"
#include "interfaces/SerialPort.h"

// ──────────────────────────────────────────────────────────────
// USART0, the LPC11U6x's 16550-style UART, interrupt driven: the RX interrupt moves
// bytes from the FIFO into rx_, the TX interrupt moves them from tx_ into the FIFO.
// read() and write() only touch the ring buffers, so neither ever waits.
//
// 8N1. The pins are the board's (IOCON registers and functions passed in).
// ──────────────────────────────────────────────────────────────
class LpcUart0 final : public SerialPort
{
public:
    static constexpr size_t RX_BUFFER = 256;
    static constexpr size_t TX_BUFFER = 256;

    LpcUart0(volatile uint32_t& rxIocon, uint32_t rxValue,
             volatile uint32_t& txIocon, uint32_t txValue)
        : rxIocon_(rxIocon), rxValue_(rxValue), txIocon_(txIocon), txValue_(txValue) {}

    void Init(uint32_t baud);

    // ── SerialPort ───────────────────────────────────────────
    size_t write(const void* data, size_t size) override;
    size_t read(void* buffer, size_t size) override;
    size_t available() const override { return rx_.Count(); }
    void SetBaudRate(uint32_t baud) override;

    /// Bytes lost because rx_ was full when they arrived.
    uint32_t RxOverruns() const { return rxOverruns_; }

    /// Called from USART0_IRQHandler.
    void OnInterrupt();

private:
    volatile uint32_t& rxIocon_;
    const uint32_t rxValue_;
    volatile uint32_t& txIocon_;
    const uint32_t txValue_;

    RingBuffer<RX_BUFFER> rx_;
    RingBuffer<TX_BUFFER> tx_;
    volatile uint32_t rxOverruns_ = 0;

    void FillTxFifo();
};
