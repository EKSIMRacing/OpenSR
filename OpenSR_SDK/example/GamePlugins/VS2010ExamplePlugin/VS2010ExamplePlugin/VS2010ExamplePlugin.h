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
#ifndef VS2010_EXAMPLE_PLUGIN_H
#define VS2010_EXAMPLE_PLUGIN_H

#include "IOpenSRPlugin.h"
#include "OpenSRBuffers.h" 

#include <windows.h>
#include <process.h> // Required for _beginthreadex
#include <string>

// VS2010 supports nullptr, but just in case of specific settings:
#ifndef nullptr
#define nullptr 0
#endif

#define _TARGET_APP "notepad.exe"

class VS2010ExamplePlugin : public osr::IOpenSRPlugin {
public:
	// random xml settings to show how to load your settings.xml (see LoadSettings function)
	struct PluginSettings {
		int holdInfoDelay;          // id="info-display-delay"
		int deviceUpdateDelay;		// id="device-update-delay"
		int globalBrightness;       // id="global-brightness"
		std::string udpIp;          // id="udp-ip"
		int udpPort;                // id="udp-port"
	};

    VS2010ExamplePlugin();
    ~VS2010ExamplePlugin();

    // Remove 'override' keyword if strict C++98 settings are on, 
    // but VS2010 usually supports it.
    bool Init(OpenSRContext* context, void* inBuf, const wchar_t* pluginPath);
    bool Start();
    void Pause();
    void Resume();
    bool IsRunning() const;
    void Stop();
    void Shutdown();

	uint64_t GetSharedPointer() {
            return 0;
        }

    // Metadata
    void GetPluginName(char* buffer, size_t bufferSize) const {
        strcpy_s(buffer, bufferSize, "VS2010 Example Plugin");
    }
    void GetAuthor(char* buffer, size_t bufferSize) const {
        strcpy_s(buffer, bufferSize, "Your Name");
    }
    void GetVersion(char* buffer, size_t bufferSize) const {
        strcpy_s(buffer, bufferSize, "1.0.0");
    }
    void GetLicenseType(char* buffer, size_t bufferSize) const {
        strcpy_s(buffer, bufferSize, "MIT");
    }
    void GetDescription(char* buffer, size_t bufferSize) const {
        strcpy_s(buffer, bufferSize, "Legacy C++ plugin for OpenSR (WinAPI threading).");
    }
    void GetTargetName(char* buffer, size_t bufferSize) const {
        strcpy_s(buffer, bufferSize, "Notepad");
    }
    void GetTargetProcessName(char* buffer, size_t bufferSize) const {
        strcpy_s(buffer, bufferSize, _TARGET_APP);
    }
    void GetPackageName(char* buffer, size_t bufferSize) const {
        strncpy_s(buffer, bufferSize, "com.example.legacyplugin", _TRUNCATE);
    }
    void GetSettingsTabName(char* buffer, size_t bufferSize) const {
        strcpy_s(buffer, bufferSize, "Options");
    }

    int GetType() { return GAME_PLUGIN_TYPE; }
    
    void GetPluginGroupName(wchar_t* buffer, size_t bufferSize) const {
        wcscpy_s(buffer, bufferSize, L"");
    }

    void OnContextChanged(osr::OpenSRContextChange reason) override;
	
private:
    OpenSRContext* m_pContext;
    OpenSRBuffersIN* m_pBuffersIn;
    std::wstring m_pluginPath;

    // Windows API Threading Resources
    HANDLE m_hThread;
    HANDLE m_hPauseEvent;     // Event to handle Pause/Resume
    CRITICAL_SECTION m_cs;    // To protect shared data

    // Volatile is sufficient for simple flags in older C++ w/ WinAPI
    volatile bool m_isPluginRunning;
    volatile bool m_stopRequested;
    volatile bool m_isPluginPaused;

    // Static thread entry point helper
    static unsigned __stdcall ThreadStaticEntryPoint(void* pThis);
    void WorkerThread();

	PluginSettings m_pluginSettings;
	bool LoadSettings(const std::wstring& path, PluginSettings& plugSettings);
};


#endif // VS2010_EXAMPLE_PLUGIN_H