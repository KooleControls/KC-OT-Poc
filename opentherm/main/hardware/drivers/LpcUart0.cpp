#include "LpcUart0.h"

namespace {

// USART0 register bits (UM10732, chapter USART0)
constexpr uint32_t LCR_8N1   = 0x03;
constexpr uint32_t LCR_DLAB  = 0x80;
constexpr uint32_t FCR_ENABLE_AND_CLEAR = 0x07;   // FIFOs on, both reset, RX trigger at 1 byte
constexpr uint32_t IER_RBR   = 0x01;              // RX data available / timeout
constexpr uint32_t IER_THRE  = 0x02;              // TX holding register empty
constexpr uint32_t IER_RLS   = 0x04;              // RX line status
constexpr uint32_t LSR_RDR   = 0x01;
constexpr uint32_t LSR_THRE  = 0x20;
constexpr size_t TX_FIFO     = 16;

LpcUart0* instance = nullptr;

} // namespace

void LpcUart0::Init(uint32_t baud)
{
    instance = this;

    NVIC_DisableIRQ(USART0_IRQn);
    lpc::EnableClock(lpc::CLK_USART0 | lpc::CLK_GPIO | lpc::CLK_IOCON);
    LPC_SYSCON->USART0CLKDIV = 1;   // the UART runs at the core clock

    rxIocon_ = rxValue_;
    txIocon_ = txValue_;

    LPC_USART0->FCR = FCR_ENABLE_AND_CLEAR;
    LPC_USART0->LCR = LCR_8N1;
    SetBaudRate(baud);

    LPC_USART0->IER = IER_RBR | IER_RLS;
    NVIC_EnableIRQ(USART0_IRQn);
}

void LpcUart0::SetBaudRate(uint32_t baud)
{
    // Divisor = clock / (16 x baud), rounded. At 48 MHz that is within 0.2 % of every
    // rate up to 115200, so the fractional divider stays at 1/1.
    const uint32_t clock = SystemCoreClock / LPC_SYSCON->USART0CLKDIV;
    uint32_t div = (clock + 8 * baud) / (16 * baud);
    if (div == 0)
        div = 1;

    LPC_USART0->LCR = LCR_8N1 | LCR_DLAB;
    LPC_USART0->DLL = div & 0xFF;
    LPC_USART0->DLM = (div >> 8) & 0xFF;
    LPC_USART0->LCR = LCR_8N1;
    LPC_USART0->FDR = 0x10;   // MULVAL 1, DIVADDVAL 0
}

size_t LpcUart0::write(const void* data, size_t size)
{
    auto* p = static_cast<const uint8_t*>(data);
    size_t n = 0;
    while (n < size && tx_.Push(p[n]))
        n++;

    // The THRE interrupt drains tx_. Enabling it with the FIFO already empty raises it
    // straight away, which is what starts the transfer.
    if (n > 0)
        LPC_USART0->IER |= IER_THRE;
    return n;
}

size_t LpcUart0::read(void* buffer, size_t size)
{
    auto* p = static_cast<uint8_t*>(buffer);
    size_t n = 0;
    while (n < size && rx_.Pop(p[n]))
        n++;
    return n;
}

void LpcUart0::OnInterrupt()
{
    // Reading IIR acknowledges a THRE interrupt.
    (void)LPC_USART0->IIR;

    // Empty the RX FIFO whatever raised the interrupt; reading LSR also clears a
    // line-status interrupt.
    while (LPC_USART0->LSR & LSR_RDR)
    {
        const uint8_t b = static_cast<uint8_t>(LPC_USART0->RBR);
        if (!rx_.Push(b))
            rxOverruns_ = rxOverruns_ + 1;
    }

    if (LPC_USART0->LSR & LSR_THRE)
        FillTxFifo();
}

void LpcUart0::FillTxFifo()
{
    uint8_t b;
    size_t n = 0;
    while (n < TX_FIFO && tx_.Pop(b))
    {
        LPC_USART0->THR = b;
        n++;
    }
    if (tx_.Empty())
        LPC_USART0->IER &= ~IER_THRE;
}

extern "C" void USART0_IRQHandler(void)
{
    if (instance != nullptr)
        instance->OnInterrupt();
}
