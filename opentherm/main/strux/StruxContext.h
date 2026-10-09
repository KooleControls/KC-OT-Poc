#pragma once
#include "StruxProvider.h"
#include "CommandManager/CommandManager.h"
#include "SettingsManager/SettingsManager.h"
#include "SystemManager/SystemManager.h"
#include "TimeManager/TimeManager.h"

// The framework layer's context: owns every Strux manager and answers StruxProvider.
//
// Init() carries the ORDER, and that is the point of it. The order has real constraints
// in it — a manager registering a setting needs SettingsManager up first — and while it
// lived in main.cpp every fork owned a copy of it. A fork that pulls a new Strux manager
// now gets its position too, instead of having to be told.
class StruxContext : public StruxProvider
{
public:
    StruxContext() = default;
    ~StruxContext() = default;
    StruxContext(const StruxContext&) = delete;
    StruxContext& operator=(const StruxContext&) = delete;

    /// Bring the framework up. Call after the board and before the application: the
    /// application registers into these managers, so they must exist and be ready first.
    void Init()
    {
        settingsManager_.Init();
        systemManager_.Init();
        timeManager_.Init();
        commandManager_.Init();
    }

    CommandManager& getCommandManager() override { return commandManager_; }
    SettingsManager& getSettingsManager() override { return settingsManager_; }
    SystemManager& getSystemManager() override { return systemManager_; }
    TimeManager& getTimeManager() override { return timeManager_; }

private:
    SettingsManager settingsManager_{*this};
    SystemManager systemManager_{*this};
    TimeManager timeManager_{*this};
    CommandManager commandManager_{*this};
};
