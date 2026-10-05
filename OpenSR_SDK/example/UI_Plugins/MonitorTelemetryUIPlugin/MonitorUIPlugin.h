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

// MonitorUIPlugin.h
// UI DLL version of OpenSRMonitorPlugin
// Implements IPluginUI and renders inside the HWND provided by the host.

#include "IPluginUI.h"
#include "TrackRenderer.h"
#include "OpenSRBuffers.h"
#include "OpenSRHelper.h"
#include "StringUtils.h"
#include "TelemetryDumper.h"

#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <gdiplus.h>

using namespace Gdiplus;

class MonitorUIPlugin : public IPluginUI {
public:
    struct PluginSettings {
        int deviceRate = 200;
    };

    MonitorUIPlugin();
    virtual ~MonitorUIPlugin();

    // IPluginSettings
    BOOL Initialize(HWND hWindow, OpenSRContext* context, void* outBuf, const wchar_t* pluginFolderPath, const uint64_t sharedBuffer) override;
    BOOL Shutdown() override;
    BOOL WantsDialogMessages() const override { return TRUE; }
   

    HWND GetHwnd() const { return m_hWnd; }

    // Button handlers (same names as original)
    void OnDump(HWND ctrl);
    void OnSelectDump(HWND ctrl);

private:
    // UI and rendering
    void CreateControls();
    void PopulateInitialState();
    void RenderTelemetry(HDC hdc);
    std::vector<std::pair<std::wstring, std::wstring>> telemetry;
    // Thread loop for periodic invalidation & pause handling
    void RenderThreadFunc();

    PluginSettings m_pluginSettings;
    bool LoadPluginSettings(const std::wstring& filepath, PluginSettings& s);

    // Subclass proc (to intercept WM_PAINT/WM_COMMAND/etc.)
    static LRESULT CALLBACK SubclassWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
        UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    static LRESULT CALLBACK ButtonSubclassWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
        UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    // Member data
    HWND m_hWnd = nullptr; // provided by host in Initialize
    OpenSRContext* m_pContext = nullptr;
    OpenSRBuffersOUT* m_pBuffersOut = nullptr;

    // Child controls
    HWND m_hDumpButton = nullptr;
    HWND m_hSelectDumpButton = nullptr;

    bool m_isDumpButtonHovered = false;
    bool m_isSelectDumpButtonHovered = false;

    // Telemetry & rendering
    TrackRenderer m_trackRenderer;
    TelemetryDumper dumper;

    // Threading
    std::thread renderThread_;
    std::atomic<bool> m_isRunning{ false };
    std::atomic<bool> m_stopRequested{ false };
    std::atomic<bool> m_isPaused{ false };
    std::mutex m_pauseMutex;
    std::condition_variable m_pauseCv;

    // GDI+
    ULONG_PTR gdiplusToken_;

    std::wstring m_pluginPath;
    // plugin folder path (if needed)
    std::wstring m_pluginFolderPath;
    std::wstring m_mapPath;

    std::condition_variable m_sleepCv;
    std::mutex m_sleepMutex;
};
