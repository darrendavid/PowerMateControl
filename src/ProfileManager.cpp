#include "ProfileManager.h"
#include <iostream>
#include <windows.h>

// Default to Volume (1); overwritten by LoadProfile() on startup
size_t ProfileManager::currentProfileIndex = 1;

// Static method: Return a reference to a static vector of profile names
const std::vector<std::wstring>& ProfileManager::GetProfileList() {
    static const std::vector<std::wstring> profiles = {L"Scroll", L"Volume"};
    return profiles;
}

// Static method: Return the current profile index
size_t ProfileManager::GetCurrentProfileIndex() {
    return currentProfileIndex;
}

// Static method: Return the name of the current profile
std::wstring ProfileManager::GetCurrentProfileName() {
    const auto& profiles = GetProfileList();
    size_t idx = GetCurrentProfileIndex();
    return (idx < profiles.size()) ? profiles[idx] : L"(Invalid Profile)";
}

// Static method: Set the current profile by index
void ProfileManager::SetCurrentProfile(int index) {
    if (index >= 0 && index < (int)GetProfileList().size()) {
        currentProfileIndex = index;
        std::wcout << L"[Debug] Current Profile set to: " << GetProfileList()[index] << std::endl;
        SaveProfile();
    } else {
        std::wcout << L"[Error] Invalid profile index" << std::endl;
    }
}

void ProfileManager::SaveProfile() {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kRegKey, 0, nullptr,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr) != ERROR_SUCCESS)
        return;
    DWORD value = static_cast<DWORD>(currentProfileIndex);
    RegSetValueExW(hKey, kRegValue, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(value));
    RegCloseKey(hKey);
}

void ProfileManager::LoadProfile() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRegKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return; // No saved profile yet — keep default (Volume)

    DWORD value = 0;
    DWORD size = sizeof(value);
    DWORD type = 0;
    if (RegQueryValueExW(hKey, kRegValue, nullptr, &type, reinterpret_cast<LPBYTE>(&value), &size) == ERROR_SUCCESS
        && type == REG_DWORD
        && value < GetProfileList().size())
    {
        currentProfileIndex = static_cast<size_t>(value);
        std::wcout << L"[Debug] Loaded profile from registry: " << GetProfileList()[currentProfileIndex] << std::endl;
    }
    RegCloseKey(hKey);
}