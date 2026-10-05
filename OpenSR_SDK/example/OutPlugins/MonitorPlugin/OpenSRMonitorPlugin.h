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
// OpenSRMonitorPlugin.h
#pragma once

// comment to use only the user interface from ui.dll that do exactly the same thing but inside the OpenSR Dashboard.
// (see MonitorTelemetryUIPlugin source code)
#define EXTERNAL_TELEM_WINDOW

#ifdef EXTERNAL_TELEM_WINDOW
#include "TrackRenderer.h"
#include "TelemetryDumper.h"

#include <thread>
#include <mutex>
#endif

#include "IOpenSRPlugin.h"
#include "OpenSRBuffers.h"
#include "GameProfileHelper.h"
#include "OpenSRHelper.h"
#include "StringUtils.h"

#include <atomic>


class OpenSRMonitorPlugin : public osr::IOpenSRPlugin {
public:
    struct PluginSettings {
        bool showExternalMonitorWindow = true;
        int deviceRate = 200; // ms
    };

    OpenSRMonitorPlugin();
    ~OpenSRMonitorPlugin() override;

    bool Init(OpenSRContext* context, void* outBuf, const wchar_t* pluginPath) override;
    bool Start() override;
    void Stop() override;
    void Shutdown() override;

    void Pause() override {
        m_isPluginPaused = true;
    }

    void Resume() override {
        m_isPluginPaused = false;
#ifdef EXTERNAL_TELEM_WINDOW
        m_pauseCv.notify_one(); // Wake up the thread if it's waiting
#endif
    }

    bool IsRunning() const override {
        return m_isPluginRunning;
    }

    void GetPluginName(char* buffer, size_t bufferSize) const override {
        strcpy_s(buffer, bufferSize, "Monitor Telemetry (Example)");
    }
    void GetAuthor(char* buffer, size_t bufferSize) const override {
        strcpy_s(buffer, bufferSize, "OpenSR Team");
    }
    void GetVersion(char* buffer, size_t bufferSize) const override {
        strcpy_s(buffer, bufferSize, "0.1 alpha");
    }
    void GetLicenseType(char* buffer, size_t bufferSize) const override {
        strcpy_s(buffer, bufferSize, "MIT");
    }
    void GetDescription(char* buffer, size_t bufferSize) const override {
        strncpy_s(buffer, bufferSize, "Example plugin that displays live telemetry data in a debug/monitoring window.\nIncludes a companion UI module (MonitorUIPlugin) that shows the same data inside the OpenSR dashboard.\nFull source code is provided in the OpenSR SDK.", _TRUNCATE);
    }
    void GetTargetName(char* buffer, size_t bufferSize) const override {
        strcpy_s(buffer, bufferSize, "");
    }
    void GetTargetProcessName(char* buffer, size_t bufferSize) const override {
        strcpy_s(buffer, bufferSize, "");
    }
    void GetPackageName(char* buffer, size_t bufferSize) const {
        strncpy_s(buffer, bufferSize, "com.zappadoc.outplugin.monitortelemetry", _TRUNCATE);
    }

    void GetUITabName(char* buffer, size_t bufferSize) const override {
        strcpy_s(buffer, bufferSize, "Monitor Telemetry"); // plugin UI custom tab name
    }

   /* void GetSettingsTabName(char* buffer, size_t bufferSize) const override {
        strcpy_s(buffer, bufferSize, "My Settings"); // not used here
    }*/
  
    int GetType() override { return OUT_PLUGIN_TYPE; }

    void GetPluginGroupName(wchar_t* buffer, size_t bufferSize) const override {
        wcscpy_s(buffer, bufferSize, L"");
    }

    void OnContextChanged(osr::OpenSRContextChange reason) override;

#ifdef EXTERNAL_TELEM_WINDOW
    HINSTANCE g_hInstance = nullptr;
    void OnDump(HWND ctrl);
    void OnSelectDump(HWND ctrl);
#endif

private:

    OpenSRContext* m_pContext;
    OpenSRBuffersOUT* m_pBuffersOut = nullptr;
    std::wstring m_pluginPath;
    std::wstring m_mapPath;
    HWND m_hWnd;
    std::atomic<bool> m_isPluginRunning = false;
    std::atomic<bool> m_isPluginPaused = false;

    // manage game profile
    GameProfileHelper m_profile;
    std::atomic<bool> m_stopRequested = false;

#ifdef EXTERNAL_TELEM_WINDOW
    PluginSettings m_pluginSettings;
    bool m_showExternalW = true;
    bool LoadPluginSettings(const std::wstring& filepath, PluginSettings& s);

    void WindowThreadFunc();
    std::thread windowThread_;
    HWND m_hDumpButton = nullptr;
    HWND m_hSelectDumpButton = nullptr;

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void RenderTelemetry(HDC hdc);
    std::vector<std::pair<std::wstring, std::wstring>> telemetry;
    std::mutex m_pauseMutex;
    std::condition_variable m_pauseCv;

    TrackRenderer m_trackRenderer;

    TelemetryDumper dumper;

    ULONG_PTR gdiplusToken_;

#endif
};
