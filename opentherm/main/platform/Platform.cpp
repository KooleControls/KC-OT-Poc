// The implementations behind the ESP-IDF compatibility headers in this folder, and the
// chip services in Platform.h.

#include "Platform.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_memory_utils.h"
#include "esp_random.h"
#include "esp_timer.h"
#include <cstdarg>
#include <cstdio>
#include <sys/time.h>
#include "LPC11U6x.h"

// ── Logging ──────────────────────────────────────────────────

__attribute__((weak)) void PlatformLogWrite(const char*, size_t)
{
}

void PlatformLog(LogLevel level, const char* tag, const char* fmt, ...)
{
    static constexpr char LEVEL_CHAR[] = { 'E', 'W', 'I' };

    char line[128];
    int len = snprintf(line, sizeof(line), "%c %s: ",
                       LEVEL_CHAR[static_cast<int>(level)], tag);

    if (len >= 0 && static_cast<size_t>(len) < sizeof(line) - 1)
    {
        va_list args;
        va_start(args, fmt);
        int body = vsnprintf(line + len, sizeof(line) - 1 - len, fmt, args);
        va_end(args);
        if (body > 0)
            len += body;
    }

    // Truncated lines still end in a newline.
    if (len < 0 || static_cast<size_t>(len) > sizeof(line) - 2)
        len = sizeof(line) - 2;
    line[len++] = '\n';
    line[len] = '\0';

    PlatformLogWrite(line, static_cast<size_t>(len));
}

// ── Restart ──────────────────────────────────────────────────

void esp_restart()
{
    // SCB->AIRCR: VECTKEY | SYSRESETREQ. The same register on every Cortex-M.
    __asm volatile("dsb" ::: "memory");
    *reinterpret_cast<volatile uint32_t*>(0xE000ED0CUL) = 0x05FA0004UL;
    __asm volatile("dsb" ::: "memory");
    for (;;) {}
}

// ── Flash range ──────────────────────────────────────────────

extern "C" const char __flash_start[];
extern "C" const char __flash_end[];

bool esp_ptr_in_drom(const void* p)
{
    auto* c = static_cast<const char*>(p);
    return c >= __flash_start && c < __flash_end;
}

// ── Tick ─────────────────────────────────────────────────────

static volatile uint32_t millis = 0;

extern "C" void SysTick_Handler(void)
{
    millis = millis + 1;
}

uint32_t Millis()
{
    return millis;
}

int64_t esp_timer_get_time()
{
    // The tick, plus how far SysTick has counted down into the current millisecond.
    // Read twice around the counter so a tick that lands in between is not mixed
    // with the count from before it.
    uint32_t ms;
    uint32_t val;
    do
    {
        ms = millis;
        val = SysTick->VAL;
    } while (ms != millis);

    const uint32_t ticksPerUs = SystemCoreClock / 1000000;
    const uint32_t us = (SysTick->LOAD - val) / (ticksPerUs ? ticksPerUs : 1);
    return static_cast<int64_t>(ms) * 1000 + us;
}

// ── Random: see esp_random.h for why this is enough ─────────

static uint32_t rngState = 0;

uint32_t esp_random()
{
    // xorshift32, stirred with the clock on every draw so two draws at different
    // moments differ even from the same state.
    uint32_t x = rngState ^ static_cast<uint32_t>(esp_timer_get_time()) ^ SysTick->VAL;
    if (x == 0)
        x = 0x9E3779B9u;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rngState = x;
    return x;
}

// ── Wall clock: the internal RTC ─────────────────────────────
// A seconds counter on its own 32 kHz oscillator, kept through a reset. libc reads it
// through _gettimeofday(), and settimeofday() sets it. After a power loss it starts
// from 0, which TimeManager::IsTimeValid() reads as "not set".

namespace {

constexpr uint32_t RTC_CTRL_SWRESET = 1u << 0;
constexpr uint32_t RTC_CTRL_OFD     = 1u << 1;   // oscillator failed; write 1 to clear
constexpr uint32_t RTC_CTRL_EN      = 1u << 7;
constexpr uint32_t CLK_RTC          = 1u << 30;

} // namespace

extern "C" int _gettimeofday(struct timeval* tv, void* /*tz*/)
{
    if (tv != nullptr)
    {
        tv->tv_sec = static_cast<time_t>(LPC_RTC->COUNT);
        tv->tv_usec = 0;
    }
    return 0;
}

extern "C" int settimeofday(const struct timeval* tv, const struct timezone* /*tz*/)
{
    if (tv == nullptr)
        return 0;

    // COUNT only takes a write while the RTC is disabled.
    LPC_RTC->CTRL = 0;
    LPC_RTC->COUNT = static_cast<uint32_t>(tv->tv_sec);
    LPC_RTC->CTRL = RTC_CTRL_EN;
    return 0;
}

void PlatformInit()
{
    SysTick_Config(SystemCoreClock / 1000);

    LPC_SYSCON->SYSAHBCLKCTRL |= CLK_RTC;
    uint32_t ctrl = LPC_RTC->CTRL;
    ctrl &= ~RTC_CTRL_SWRESET;
    ctrl |= RTC_CTRL_EN | RTC_CTRL_OFD;   // run, and clear a stale oscillator-fail flag
    LPC_RTC->CTRL = ctrl;
}
