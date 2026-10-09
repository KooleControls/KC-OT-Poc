#include "SystemManager.h"
#include "SettingsManager.h"
#include "CommandManager.h"
#include "DateTime.h"
#include "esp_log.h"
#include <cstdio>
#include "esp_system.h"
#include <cstring>

SystemManager::SystemManager(StruxProvider& strux)
    : strux_(strux)
{
}

CommandEntry SystemManager::pingCommand_{
    "system ping", &InvokeCommand<&SystemManager::Cmd_Ping>,
    "Check the device is answering. Takes nothing, returns {\"pong\":true}."
};

CommandEntry SystemManager::infoCommand_{
    "system info", &InvokeCommand<&SystemManager::Cmd_Info>,
    "Report this device's runtime state: name, firmware build, chip, clock "
    "and the device's own clock."
};

CommandEntry SystemManager::rebootCommand_{
    "system reboot", &InvokeCommand<&SystemManager::Cmd_Reboot>,
    "Restart the device. The reply is written first, then the device restarts "
    "about half a second later and every connection drops."
};

CommandEntry SystemManager::describeCommand_{
    "system describe", &InvokeCommand<&SystemManager::Cmd_Describe>,
    "What this device IS: its name, firmware, one-line description and its "
    "full instructions - how it is meant to be driven. Pair it with "
    "'help', which is the same question about the commands."
};

void SystemManager::Init()
{
    strux_.getSettingsManager().Register({ &name_ });
    strux_.getCommandManager().Register(this, {
        &pingCommand_,
        &infoCommand_,
        &rebootCommand_,
        &describeCommand_,
    });

    // Logged at boot as well as served by `system info`: the log is there before
    // anything can ask.
    char cpu[48] = {};
    DescribeCpu(cpu, sizeof(cpu));
    ESP_LOGI(TAG, "CPU: %s", cpu);

    ESP_LOGI(TAG, "Initialized");
}

void SystemManager::DescribeCpu(char* out, size_t maxLen)
{
    // CMSIS's name for the core clock, set by the chip's startup code.
    extern uint32_t SystemCoreClock;
    snprintf(out, maxLen, "%lu MHz", static_cast<unsigned long>(SystemCoreClock / 1000000UL));
}

void SystemManager::SetDocumentation(const char* description, const char* instructions)
{
    description_  = description;
    instructions_ = instructions;
}

void SystemManager::GetDeviceName(char* out, size_t maxLen)
{
    name_.Get(out, maxLen);
    if (out[0] == '\0')
        snprintf(out, maxLen, "%s", STRUX_PROJECT_NAME);
}

// ──────────────────────────────────────────────────────────────
// Commands
// ──────────────────────────────────────────────────────────────

CommandResult SystemManager::Cmd_Ping(CommandContext& ctx)
{
    auto resp = ctx.reply.object();
    resp.field("pong", true);
    return CommandResult::Ok;
}

CommandResult SystemManager::Cmd_Info(CommandContext& ctx)
{
    auto resp = ctx.reply.object();

    char deviceName[32] = {};
    GetDeviceName(deviceName, sizeof(deviceName));
    resp.field("name", deviceName);

    resp.field("project", STRUX_PROJECT_NAME);
    resp.field("firmware", STRUX_PROJECT_VER);
    resp.field("date", __DATE__);
    resp.field("time", __TIME__);
    resp.field("chip", STRUX_CHIP);

    char cpu[48] = {};
    DescribeCpu(cpu, sizeof(cpu));
    resp.field("cpu", cpu);


    char deviceTimeStr[32] = "Not synced";
    DateTime now = DateTime::Now();
    if (now.YearLocal() >= 2020)
        now.ToStringLocal(deviceTimeStr, sizeof(deviceTimeStr), "%F %T");
    resp.field("deviceTime", deviceTimeStr);
    return CommandResult::Ok;
}

CommandResult SystemManager::Cmd_Describe(CommandContext& ctx)
{
    char deviceName[48] = {};
    GetDeviceName(deviceName, sizeof(deviceName));

    auto resp = ctx.reply.object();
    resp.field("ok", true);
    resp.field("name", deviceName);
    resp.field("project", STRUX_PROJECT_NAME);
    resp.field("firmware", STRUX_PROJECT_VER);
    resp.field("commit", STRUX_GIT_COMMIT);

    // Absent, not empty, when the application registered nothing: a reader can then
    // tell a device that has no instructions from one whose instructions are blank.
    // The strings stream straight to the transport — nothing here buffers a README.
    if (description_ != nullptr && description_[0] != '\0')
        resp.field("description", description_);
    if (instructions_ != nullptr && instructions_[0] != '\0')
        resp.field("instructions", instructions_);

    return CommandResult::Ok;
}

CommandResult SystemManager::Cmd_Reboot(CommandContext& ctx)
{
    {
        auto resp = ctx.reply.object();
        resp.field("ok", true);
    }   // close the scope BEFORE restarting so the reply is complete

    // TODO: the reply has to have left the UART before this. On the gateway that is a
    // 500 ms task delay; here it wants a flush of whatever transport carried it.
    esp_restart();
}
