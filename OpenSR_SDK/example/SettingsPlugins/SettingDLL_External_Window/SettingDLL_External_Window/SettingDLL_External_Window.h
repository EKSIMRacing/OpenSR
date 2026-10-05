/*
#####################################################################
# Part of Open Sim Relay (OpenSR) - Source Code
# Copyright (c) 2025-2026 Zappadoc - All Rights Reserved
#
# This file is part of the OpenSR Plugins SDK.
#
# PURPOSE:
#   Provides shared data structures and interface definitions
#   required to develop third-party OpenSR plugins.
#
# LICENSE:
#   This header is provided under the OpenSR Plugin Interface
#   License. Redistribution is permitted *only* as part of
#   developing plugins for OpenSR, and subject to the terms of
#   the license provided with the SDK.
#
# NOTICE:
# - This code, modules, and libraries are subject to change
#   without notice.
# - By using this code, you agree to the terms and conditions
#   of the OpenSR license provided with the software.
#
# Change History:
#   2025-01-07 : Initial creation by Zappadoc
#   2025-10-01 : Integrated into OpenSR project
#####################################################################
*/
// SettingDLL_External_Window.h
#pragma once

#include "OpenSRContext.h"
#include "IPluginSettings.h"

struct MySettings {
    bool updateNeeded = false;
    char outgauge_ip[32];
    int  outgauge_port;
    char motionsim_ip[32];
    int  motionsim_port;
};

class SettingsDLLExternalDialog : public IPluginSettings {
public:
    SettingsDLLExternalDialog() {}
    virtual ~SettingsDLLExternalDialog() {}

    BOOL Initialize(HWND hWindow, OpenSRContext* context, void* buffersOut, const wchar_t* pluginFolderPath, const uint64_t sharedBuffer) override;
    BOOL Shutdown() override;

private:
    void SaveSettings(HWND hwndDlg);
    void LoadSettings();
    // The subclassed window procedure for our panel
    static LRESULT CALLBACK SubclassWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

    static LRESULT CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void InitDialogValues(HWND hwndDlg);
    void OnPaint(HWND hWnd);
    void OnPaintDlg(HWND hWnd);
    
    void CenterDialogInParent(HWND hDlg);

    HINSTANCE g_hInstance = nullptr;
    OpenSRContext* m_pContext = nullptr;
    std::wstring m_xmlPath;
    MySettings m_settings;
    HWND m_hWnd = nullptr; // The HWND provided by the host
};

//  Exported C Functions

// This function is called by the host application to create an instance of our settings class.
extern "C" __declspec(dllexport) IPluginSettings * CreatePluginSettings() {
    // We simply create a 'new' instance of our implementation class.
    return new SettingsDLLExternalDialog();
}

// This function is called by the host application to destroy the instance.
extern "C" __declspec(dllexport) void DestroyPluginSettings(IPluginSettings * pSettings) {
    // We cast it back to the concrete type and 'delete' it.
    if (pSettings) {
        delete static_cast<SettingsDLLExternalDialog*>(pSettings);
    }
}
