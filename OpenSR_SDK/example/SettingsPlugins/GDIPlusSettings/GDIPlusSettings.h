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
#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <string>
#include "OpenSRContext.h"
#include "IPluginSettings.h"

#pragma comment(lib, "Gdiplus.lib")

class GDIPlusSettings : public IPluginSettings {
public:
    GDIPlusSettings();
    ~GDIPlusSettings();

    // Called by host
    BOOL Initialize(HWND hWindow, OpenSRContext* context, void* buffersOut, const wchar_t* pluginFolderPath, const uint64_t sharedBuffer);
    BOOL Shutdown();

    BOOL WantsDialogMessages() const override {
        return TRUE; // Host will call IsDialogMessage(hChild_, ...)
    }
private:
    HWND m_hWnd = nullptr;
    OpenSRContext* m_pContext = nullptr;
    std::wstring m_pluginFolderPath;

    // Child controls
    HWND m_hLabel = nullptr;
    HWND m_hEdit = nullptr;
    HWND m_hEdit2 = nullptr;

    // GDI+ token
    ULONG_PTR m_gdiplusToken = 0;

    // Subclassing
    static LRESULT CALLBACK SubclassWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
        UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

    void OnPaint();
    void OnSize(UINT width, UINT height);
    // Canvas size (fixed)
    const int m_canvasWidth = 1024;
    const int m_canvasHeight = 800;
};

