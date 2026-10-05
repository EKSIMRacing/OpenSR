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

#include "IOpenSRPlugin.h"
#include "OpenSRBuffers.h"
#include "GameProfileHelper.h"

#include <thread>
#include <mutex>
#include <atomic>

#define _TARGET_APP "notepad.exe"

class ExamplePlugin : public osr::IOpenSRPlugin {
public:
    ExamplePlugin();
    ~ExamplePlugin();

    bool Init(OpenSRContext* context, void* inBuf, const wchar_t* pluginPath) override;
    bool Start() override;
    void Stop() override;
    void Shutdown() override;

    void Pause() override {
        std::lock_guard<std::mutex> lock(m_pauseMutex);
        m_isPluginPaused = true;
    }

    void Resume() override {
        std::lock_guard<std::mutex> lock(m_pauseMutex);
        m_isPluginPaused = false;
        m_pauseCv.notify_one(); // Wake up the thread if it's waiting
    }

    bool IsRunning() const override {
        return m_isPluginRunning;
    }
 
	void GetPluginName(char* buffer, size_t bufferSize) const override {
		strcpy_s(buffer, bufferSize, "Example Game (IN) Plugin");
	}
	void GetAuthor(char* buffer, size_t bufferSize) const override {
		strcpy_s(buffer, bufferSize, "Your Name");
	}
	void GetVersion(char* buffer, size_t bufferSize) const override {
		strcpy_s(buffer, bufferSize, "1.0.0");
	}
	void GetLicenseType(char* buffer, size_t bufferSize) const override {
		strcpy_s(buffer, bufferSize, "MIT");
	}
	void GetDescription(char* buffer, size_t bufferSize) const override {
		strcpy_s(buffer, bufferSize, "A C++ example plugin for OpenSR, source code included in OpenSR SDK.");
	}
    void GetTargetName(char* buffer, size_t bufferSize) const override {
        strcpy_s(buffer, bufferSize, "Notepad");
    }
	void GetTargetProcessName(char* buffer, size_t bufferSize) const override {
		strcpy_s(buffer, bufferSize, _TARGET_APP);
	}
    void GetPackageName(char* buffer, size_t bufferSize) const override {
        strncpy_s(buffer, bufferSize, "com.zappadoc.gameplugin.cplusplusexample", _TRUNCATE);
    }

    void GetSettingsTabName(char* buffer, size_t bufferSize) const override {
        strcpy_s(buffer, bufferSize, "Options");
    }

    int GetType() override { return GAME_PLUGIN_TYPE; }
    
    void GetPluginGroupName(wchar_t* buffer, size_t bufferSize) const override {
        wcscpy_s(buffer, bufferSize, L"");
    }

    void OnContextChanged(osr::OpenSRContextChange reason) override;
	
private:
    OpenSRContext* m_pContext = nullptr;
    OpenSRBuffersIN* m_pBuffersIn = nullptr;
    std::wstring m_pluginPath;

    void WorkerThread();
    std::thread workerThread;

    std::atomic<bool> m_isPluginRunning = false;
    std::atomic<bool> m_stopRequested = false;

    std::atomic<bool> m_isPluginPaused = false;
    std::mutex m_pauseMutex;
    std::condition_variable m_pauseCv;

    // example of plugin settings
    struct PluginSettings {
        std::string outgauge_ip = "127.0.0.1";
        int outgauge_port = 19999;
    };

    PluginSettings m_pluginSettings;

    bool LoadPluginSettings(const std::wstring& filepath, PluginSettings& s);
};




