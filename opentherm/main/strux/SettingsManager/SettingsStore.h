#pragma once

#include <cstddef>
#include <cstdint>

// ──────────────────────────────────────────────────────────────
// The key-value store behind SettingsManager — what NVS is on the gateway, with the
// same shape: typed values under short keys, a read of the wrong type fails, and
// nothing is durable until Commit().
//
// RAM only for now. This board has no flash file system, and the LPC11U68's EEPROM is
// not driven yet, so Commit() reports failure instead of pretending: a `settings save`
// that says it worked and then forgets at reboot is worse than one that says it can't.
// Making it durable means serialising `entries_` to EEPROM in Commit() and loading
// it back at startup; nothing above this class changes.
// ──────────────────────────────────────────────────────────────
class SettingsStore
{
    static constexpr const char* TAG = "SettingsStore";

public:
    /// Including the terminator. NVS's limit, kept so a key is valid on both boards.
    static constexpr size_t KEY_MAX = 16;
    static constexpr size_t MAX_ENTRIES = 16;
    /// Including the terminator.
    static constexpr size_t STRING_MAX = 64;

    bool GetI32(const char* key, int32_t& out) const;
    bool SetI32(const char* key, int32_t v);
    bool GetU32(const char* key, uint32_t& out) const;
    bool SetU32(const char* key, uint32_t v);
    bool GetU8(const char* key, uint8_t& out) const;
    bool SetU8(const char* key, uint8_t v);
    bool GetString(const char* key, char* out, size_t maxLen) const;
    bool SetString(const char* key, const char* v);

    void EraseAll();
    bool Commit();

private:
    enum class Kind : uint8_t { Empty, I32, U32, U8, String };

    struct Entry
    {
        char key[KEY_MAX];
        Kind kind;
        union
        {
            int32_t i32;
            uint32_t u32;
            uint8_t u8;
            char str[STRING_MAX];
        };
    };

    Entry entries_[MAX_ENTRIES] = {};

    /// The entry for `key`, or null.
    const Entry* Find(const char* key) const;

    /// The entry for `key`, claiming a free one if there is none; null when full or the
    /// key is too long.
    Entry* FindOrClaim(const char* key);
};
