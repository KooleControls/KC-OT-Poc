#include "TimeManager.h"
#include "SettingsManager/SettingsManager.h"
#include "esp_log.h"
#include <cstdio>
#include <sys/time.h>

TimeManager::TimeManager(StruxProvider &ctx)
    : strux_(ctx)
{
}

void TimeManager::Init()
{
    strux_.getSettingsManager().Register({ &timezone_ });

    ApplyTimezone();

    ESP_LOGI(TAG, "Initialized");
}

void TimeManager::ApplyTimezone()
{
    char tz[64] = {};
    timezone_.Get(tz, sizeof(tz));
    if (tz[0] == '\0')
        snprintf(tz, sizeof(tz), "UTC0");

    setenv("TZ", tz, 1);
    tzset();
    ESP_LOGI(TAG, "Timezone set to: %s", tz);
}

void TimeManager::SetTime(std::time_t utc)
{
    timeval tv = {};
    tv.tv_sec = utc;
    settimeofday(&tv, nullptr);

    synced = true;

    char buf[32];
    DateTime now = DateTime::Now();
    now.ToStringLocal(buf, sizeof(buf), "%F %T");
    ESP_LOGI(TAG, "Time set: %s", buf);
}

bool TimeManager::IsTimeValid() const
{
    time_t now = time(nullptr);
    struct tm t;
    gmtime_r(&now, &t);
    return t.tm_year >= (2020 - 1900);
}
