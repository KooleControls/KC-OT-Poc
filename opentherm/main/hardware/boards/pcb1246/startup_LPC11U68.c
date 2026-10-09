// GCC startup for the LPC11U68: vector table, Code Read Protection word and reset handler.
// Every handler is a weak default; a driver overrides one by defining it.

#include <stdint.h>
#include "LPC11U6x.h"

// Linker script symbols (LPC11U68.ld).
extern uint32_t __stack_top;
extern uint32_t __data_load_start;
extern uint32_t __data_start;
extern uint32_t __data_end;
extern uint32_t __bss_start;
extern uint32_t __bss_end;

extern void SystemInit(void);
extern void __libc_init_array(void);
extern int main(void);

void Reset_Handler(void);

void Default_Handler(void)
{
    for (;;) {}
}

#define WEAK_DEFAULT __attribute__((weak, alias("Default_Handler")))

void NMI_Handler(void) WEAK_DEFAULT;
void HardFault_Handler(void) WEAK_DEFAULT;
void SVC_Handler(void) WEAK_DEFAULT;
void PendSV_Handler(void) WEAK_DEFAULT;
void SysTick_Handler(void) WEAK_DEFAULT;
void PIN_INT0_IRQHandler(void) WEAK_DEFAULT;
void PIN_INT1_IRQHandler(void) WEAK_DEFAULT;
void PIN_INT2_IRQHandler(void) WEAK_DEFAULT;
void PIN_INT3_IRQHandler(void) WEAK_DEFAULT;
void PIN_INT4_IRQHandler(void) WEAK_DEFAULT;
void PIN_INT5_IRQHandler(void) WEAK_DEFAULT;
void PIN_INT6_IRQHandler(void) WEAK_DEFAULT;
void PIN_INT7_IRQHandler(void) WEAK_DEFAULT;
void GINT0_IRQHandler(void) WEAK_DEFAULT;
void GINT1_IRQHandler(void) WEAK_DEFAULT;
void I2C1_IRQHandler(void) WEAK_DEFAULT;
void USART1_4_IRQHandler(void) WEAK_DEFAULT;
void USART2_3_IRQHandler(void) WEAK_DEFAULT;
void SCT0_1_IRQHandler(void) WEAK_DEFAULT;
void SSP1_IRQHandler(void) WEAK_DEFAULT;
void I2C0_IRQHandler(void) WEAK_DEFAULT;
void CT16B0_IRQHandler(void) WEAK_DEFAULT;
void CT16B1_IRQHandler(void) WEAK_DEFAULT;
void CT32B0_IRQHandler(void) WEAK_DEFAULT;
void CT32B1_IRQHandler(void) WEAK_DEFAULT;
void SSP0_IRQHandler(void) WEAK_DEFAULT;
void USART0_IRQHandler(void) WEAK_DEFAULT;
void USB_IRQHandler(void) WEAK_DEFAULT;
void USB_FIQ_IRQHandler(void) WEAK_DEFAULT;
void ADC_A_IRQHandler(void) WEAK_DEFAULT;
void RTC_IRQHandler(void) WEAK_DEFAULT;
void BOD_WDT_IRQHandler(void) WEAK_DEFAULT;
void FLASH_IRQHandler(void) WEAK_DEFAULT;
void DMA_IRQHandler(void) WEAK_DEFAULT;
void ADC_B_IRQHandler(void) WEAK_DEFAULT;
void USBWAKEUP_IRQHandler(void) WEAK_DEFAULT;

typedef void (*VectorEntry)(void);

// Entry 7 is left 0 here. The boot ROM only starts an image whose first 8 entries sum
// to zero; tools/lpc_checksum.py patches it into the ELF after linking.
__attribute__((section(".isr_vector"), used))
const VectorEntry vectorTable[48] = {
    (VectorEntry)&__stack_top,
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    0, 0, 0, 0, 0, 0, 0,
    SVC_Handler,
    0, 0,
    PendSV_Handler,
    SysTick_Handler,

    PIN_INT0_IRQHandler,
    PIN_INT1_IRQHandler,
    PIN_INT2_IRQHandler,
    PIN_INT3_IRQHandler,
    PIN_INT4_IRQHandler,
    PIN_INT5_IRQHandler,
    PIN_INT6_IRQHandler,
    PIN_INT7_IRQHandler,
    GINT0_IRQHandler,
    GINT1_IRQHandler,
    I2C1_IRQHandler,
    USART1_4_IRQHandler,
    USART2_3_IRQHandler,
    SCT0_1_IRQHandler,
    SSP1_IRQHandler,
    I2C0_IRQHandler,
    CT16B0_IRQHandler,
    CT16B1_IRQHandler,
    CT32B0_IRQHandler,
    CT32B1_IRQHandler,
    SSP0_IRQHandler,
    USART0_IRQHandler,
    USB_IRQHandler,
    USB_FIQ_IRQHandler,
    ADC_A_IRQHandler,
    RTC_IRQHandler,
    BOD_WDT_IRQHandler,
    FLASH_IRQHandler,
    DMA_IRQHandler,
    ADC_B_IRQHandler,
    USBWAKEUP_IRQHandler,
    0,
};

// Any value other than 0xFFFFFFFF here enables read protection or disables ISP, which can
// lock us out of the part. The linker script pins this word to address 0x2FC.
__attribute__((section(".crp"), used))
const uint32_t codeReadProtection = 0xFFFFFFFF;

// Runs before .data and .bss exist, so it must not touch any global variable.
void Reset_Handler(void)
{
    SystemInit();

    uint32_t *src = &__data_load_start;
    for (uint32_t *dst = &__data_start; dst < &__data_end; ++dst)
        *dst = *src++;

    for (uint32_t *dst = &__bss_start; dst < &__bss_end; ++dst)
        *dst = 0;

    __libc_init_array();
    main();

    for (;;) {}
}
