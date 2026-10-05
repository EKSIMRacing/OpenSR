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

// SettingsDLLTest.h
#pragma once
#include "IPluginSettings.h"

struct MySettings {
    bool updateNeeded = false;
    char outgauge_ip[32];
    int  outgauge_port;
    char motionsim_ip[32];
    int  motionsim_port;
};

enum {
    IDC_EDIT_SINGLE = 1001,
    IDC_EDIT_MULTI,
    IDC_LISTBOX,
    IDC_COMBO,
    IDC_CHECK,
    IDC_RADIO1,
    IDC_RADIO2,
    IDC_SAVE
};

class SettingsDLLTest : public IPluginSettings {
public:
    SettingsDLLTest() {}
    virtual ~SettingsDLLTest() {}

    BOOL Initialize(HWND hWindow, OpenSRContext* context, void* buffersOut, const wchar_t* pluginFolderPath, const uint64_t sharedBuffer) override;
    BOOL Shutdown() override;
    HWND GetHwnd() const { return m_hWnd; }
    BOOL WantsDialogMessages() const override { return TRUE; }

   /* void GetSettingsTabName(char* buffer, size_t bufferSize) const override {
        strcpy_s(buffer, bufferSize, "DLL Test");
    }*/

private:
    void SaveSettings();
    void LoadSettings();


    // The subclassed window procedure for our panel
    static LRESULT CALLBACK SubclassWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    static LRESULT CALLBACK EditSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

    // Message handlers
    void CreateControls();
    void OnPaint();
    void OnCommand(WPARAM wParam, LPARAM lParam);
    void OnSave();
    void OnSaveControls(HWND hWnd);
    void PopulateFromSettings();

    //  Member Variables
    HWND m_hWnd = nullptr; // The HWND provided by the host
    OpenSRContext* m_pContext = nullptr;
    std::wstring m_xmlPath;
    MySettings m_settings;

    //  Child Control Handles
    // We will create and manage our own standard Win32 controls
    HWND m_hOutGaugeIp = nullptr;
    HWND m_hOutGaugePort = nullptr;
    HWND m_hMotionSimIp = nullptr;
    HWND m_hMotionSimPort = nullptr;
    HWND m_hSaveButton = nullptr;

    HWND hEditSingle, hEditMulti, hList, hCombo, hCheck, hRadio1, hRadio2, hSave;
};


