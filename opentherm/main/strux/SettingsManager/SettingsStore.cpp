#include "SettingsStore.h"
#include "esp_log.h"
#include <cstring>

const SettingsStore::Entry* SettingsStore::Find(const char* key) const
{
    for (const Entry& e : entries_)
        if (e.kind != Kind::Empty && strcmp(e.key, key) == 0)
            return &e;
    return nullptr;
}

SettingsStore::Entry* SettingsStore::FindOrClaim(const char* key)
{
    if (strlen(key) >= KEY_MAX)
        return nullptr;

    if (const Entry* found = Find(key))
        return const_cast<Entry*>(found);

    for (Entry& e : entries_)
    {
        if (e.kind == Kind::Empty)
        {
            strcpy(e.key, key);
            return &e;
        }
    }

    ESP_LOGE(TAG, "Full (%u entries), cannot store '%s'", (unsigned)MAX_ENTRIES, key);
    return nullptr;
}

bool SettingsStore::GetI32(const char* key, int32_t& out) const
{
    const Entry* e = Find(key);
    if (e == nullptr || e->kind != Kind::I32) return false;
    out = e->i32;
    return true;
}

bool SettingsStore::SetI32(const char* key, int32_t v)
{
    Entry* e = FindOrClaim(key);
    if (e == nullptr) return false;
    e->kind = Kind::I32;
    e->i32 = v;
    return true;
}

bool SettingsStore::GetU32(const char* key, uint32_t& out) const
{
    const Entry* e = Find(key);
    if (e == nullptr || e->kind != Kind::U32) return false;
    out = e->u32;
    return true;
}

bool SettingsStore::SetU32(const char* key, uint32_t v)
{
    Entry* e = FindOrClaim(key);
    if (e == nullptr) return false;
    e->kind = Kind::U32;
    e->u32 = v;
    return true;
}

bool SettingsStore::GetU8(const char* key, uint8_t& out) const
{
    const Entry* e = Find(key);
    if (e == nullptr || e->kind != Kind::U8) return false;
    out = e->u8;
    return true;
}

bool SettingsStore::SetU8(const char* key, uint8_t v)
{
    Entry* e = FindOrClaim(key);
    if (e == nullptr) return false;
    e->kind = Kind::U8;
    e->u8 = v;
    return true;
}

bool SettingsStore::GetString(const char* key, char* out, size_t maxLen) const
{
    const Entry* e = Find(key);
    // Like NVS: a buffer too small for the stored value is a failed read, not a
    // truncated one.
    if (e == nullptr || e->kind != Kind::String || strlen(e->str) >= maxLen) return false;
    strcpy(out, e->str);
    return true;
}

bool SettingsStore::SetString(const char* key, const char* v)
{
    if (strlen(v) >= STRING_MAX) return false;

    Entry* e = FindOrClaim(key);
    if (e == nullptr) return false;
    e->kind = Kind::String;
    strcpy(e->str, v);
    return true;
}

void SettingsStore::EraseAll()
{
    for (Entry& e : entries_)
        e.kind = Kind::Empty;
}

bool SettingsStore::Commit()
{
    ESP_LOGW(TAG, "No persistent storage on this board yet; settings last until reboot");
    return false;
}
