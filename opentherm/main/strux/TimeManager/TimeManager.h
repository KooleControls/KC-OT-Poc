#pragma once
#include "StruxProvider.h"
#include "TypedSettings.h"
#include "DateTime.h"
#include <ctime>

// ──────────────────────────────────────────────────────────────
// TimeManager owns the device clock's timezone and whether the clock has been set.
//
// This board has no network, so there is no SNTP: the clock is set from outside,
// by whoever knows the time (the gateway, over the channel), through SetTime().
// The clock itself is the platform's (the chip's RTC here): it keeps running through
// a reset but starts from zero after a power loss, which IsTimeValid() reports.
// ──────────────────────────────────────────────────────────────
class TimeManager
{
    inline static constexpr const char *TAG = "TimeManager";

public:
    explicit TimeManager(StruxProvider &ctx);

    void Init();

    /// Set the device clock to `utc` (seconds since the epoch) and mark it synced.
    void SetTime(std::time_t utc);

    bool IsTimeSynced() const { return synced; }
    bool IsTimeValid() const;

private:
    StruxProvider &strux_;

    volatile bool synced = false;

    void ApplyTimezone();

    // ── Settings (registered with SettingsManager in Init) ──
    inline static StringSetting timezone_{ "time.timezone", "Timezone", "UTC0" };
};
