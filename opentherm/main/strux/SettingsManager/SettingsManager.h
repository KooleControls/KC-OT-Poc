#pragma once

#include "StruxProvider.h"
#include "CommandEntry.h"
#include "Setting.h"
#include "TypedSettings.h"
#include "SettingsStore.h"
#include <initializer_list>

class Stream;

// ──────────────────────────────────────────────────────────────
// Schema registry + storage (SettingsStore). Managers own their settings as
// typed inline static members (TypedSettings.h) and register them
// in Init(). Nothing about JSON, UI, or any presentation format
// lives here — converters (e.g. the getSettings/setSetting command
// handlers below) walk the chain via begin()/end().
// ──────────────────────────────────────────────────────────────
class SettingsManager {
    static constexpr const char* TAG = "SettingsManager";

public:
    explicit SettingsManager(StruxProvider& strux);

    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;

    void Init();

    // ── Schema registration ──────────────────────────────────
    // Called from a manager's Init(). Entries MUST have static storage
    // duration (see ~Setting). Heterogeneous leaves register through
    // base pointers; the initializer_list lives on the caller's stack.
    //
    // SettingsManager is the storage link, so registration enforces the
    // store's rules — boot-deterministically.
    void Register(std::initializer_list<Setting*> settings);

    // ── Iteration ────────────────────────────────────────────
    //   for (Setting& s : settingsManager) { ... }
    SettingIterator begin();
    SettingIterator end() { return SettingIterator(nullptr); }

    // ── Persistence ──────────────────────────────────────────
    bool Save();
    /// Erase the store and commit. That's all — defaults resolve
    /// at read, so nothing needs to be written back.
    bool ResetToDefaults();

    // ── Storage primitives (used by the typed leaves via Manager()) ──
    // Return false when the key has no stored value (caller falls back
    // to the entry's default).
    bool ReadI32(const char* key, int32_t& out) const;
    bool WriteI32(const char* key, int32_t v);
    bool ReadU32(const char* key, uint32_t& out) const;
    bool WriteU32(const char* key, uint32_t v);
    bool ReadU8(const char* key, uint8_t& out) const;
    bool WriteU8(const char* key, uint8_t v);
    bool ReadString(const char* key, char* out, size_t maxLen) const;
    bool WriteString(const char* key, const char* v);

private:
    StruxProvider& strux_;
    SettingsStore store_;

    Setting* head_ = nullptr;

    const Setting* Find(const char* key) const;

    // ── WebSocket commands (the JSON converter lives HERE, at the
    //    edge — not in the schema/storage core above) ──────────
    CommandResult Cmd_GetSettings(CommandContext& ctx);
    CommandResult Cmd_SetSetting(CommandContext& ctx);
    CommandResult Cmd_SaveSettings(CommandContext& ctx);

    // Defined in SettingsManager.cpp, beside the handlers and the arguments they read.
    static CommandEntry listCommand_;
    static CommandEntry setCommand_;
    static CommandEntry saveCommand_;
};
