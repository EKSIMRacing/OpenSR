#pragma once

struct SettingsData {
    int reloadNeeded;
    wchar_t pluginPath[MAX_PATH];
    wchar_t text[60];
};