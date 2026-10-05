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

#include "VS2010ExamplePlugin.h"
#include <iostream>
#include <math.h>   // fmod, sin, cos
#include "OpenSRLegacyHelper.h"

// Note: If you have TelemetryPlayer.h compatible with VS2010, include it here.
#include "TelemetryPlayer.h" 
#include "tinyxml2.h"

using namespace tinyxml2;

// DLL Exported Factory Functions
extern "C" __declspec(dllexport) osr::IOpenSRPlugin * CreatePlugin() {
    return new VS2010ExamplePlugin();
}

extern "C" __declspec(dllexport) void DestroyPlugin(osr::IOpenSRPlugin * plugin) {
    if (plugin) 
		delete plugin;
}

VS2010ExamplePlugin::VS2010ExamplePlugin() 
    : m_pContext(nullptr), m_pBuffersIn(nullptr), 
      m_hThread(NULL), m_hPauseEvent(NULL),
      m_isPluginRunning(false), m_stopRequested(false), m_isPluginPaused(false)
{
    // Initialize Critical Section (Mutex replacement)
    InitializeCriticalSection(&m_cs);
    // Create manual reset event for pausing. Initial state: Signaled (Running)
    m_hPauseEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
}

VS2010ExamplePlugin::~VS2010ExamplePlugin() {
    // Ensure thread is stopped before destruction
    Stop();
    
    DeleteCriticalSection(&m_cs);
    if (m_hPauseEvent) CloseHandle(m_hPauseEvent);
}

bool VS2010ExamplePlugin::Init(OpenSRContext* context, void* inBuf, const wchar_t* pluginPath) {
    // VS2010: Use std::cout sparingly in DLLs, can cause CRT issues if not careful, 
    // but usually fine for console attached apps.
    m_pContext = context;
    // Cast void* to the specific struct type
    m_pBuffersIn = (OpenSRBuffersIN*)inBuf; 
    m_pluginPath = pluginPath;

	if(m_pContext && !m_pluginPath.empty()) {
		// Load Settings
		std::wstring spath = std::wstring(m_pContext->osrDocFolder) + L"\\" + m_pluginPath + L"\\settings.xml";
		if (!LoadSettings(spath, m_pluginSettings)) {
			std::cout << "Failed to load settings file." << std::endl;
		} 
	}
    return true;
}

void VS2010ExamplePlugin::OnContextChanged(osr::OpenSRContextChange reason) {
   if (m_pContext && reason == osr::OpenSRContextChange::SettingsChanged) {
        if (!m_pluginPath.empty()) {
            std::wstring settingsPath = std::wstring(m_pContext->osrDocFolder) + L"\\" + m_pluginPath + L"\\settings.xml";
            std::wcout << "[LEGACY VS2010 PLUGIN] Profile need to be reloaded.\nProfile Path: " << settingsPath << std::endl;
            if (!LoadSettings(settingsPath, m_pluginSettings)) {
				std::cerr <<  "failed to load settings.\n";
            }
        }
    }
}

bool VS2010ExamplePlugin::Start() {
    if (m_isPluginRunning) {
        return false;
    }

    m_stopRequested = false;
    m_isPluginPaused = false;
    // Ensure the pause event is signaled so thread runs immediately
    SetEvent(m_hPauseEvent); 

    m_isPluginRunning = true;

    // VS2010: Use _beginthreadex instead of std::thread
    unsigned threadId;
    m_hThread = (HANDLE)_beginthreadex(NULL, 0, &VS2010ExamplePlugin::ThreadStaticEntryPoint, this, 0, &threadId);

    if (m_hThread == NULL) {
        m_isPluginRunning = false;
        return false;
    }

    return true;
}

void VS2010ExamplePlugin::Stop() {
    m_stopRequested = true;
    
    // Resume if paused so the thread can exit the loop
    Resume();

    if (m_hThread != NULL) {
        // Wait for the thread to finish (join)
        WaitForSingleObject(m_hThread, INFINITE);
        CloseHandle(m_hThread);
        m_hThread = NULL;
    }
    m_isPluginRunning = false;
}

void VS2010ExamplePlugin::Shutdown() {
    // cleanup
}

void VS2010ExamplePlugin::Pause() {
    EnterCriticalSection(&m_cs);
    if (!m_isPluginPaused) {
        m_isPluginPaused = true;
        // Unsignal the event -> Thread will block at WaitForSingleObject
        ResetEvent(m_hPauseEvent); 
    }
    LeaveCriticalSection(&m_cs);
}

void VS2010ExamplePlugin::Resume() {
    EnterCriticalSection(&m_cs);
    if (m_isPluginPaused) {
        m_isPluginPaused = false;
        // Signal the event -> Thread continues
        SetEvent(m_hPauseEvent); 
    }
    LeaveCriticalSection(&m_cs);
}

bool VS2010ExamplePlugin::IsRunning() const {
    return m_isPluginRunning;
}

// Static helper to bounce into the member function
unsigned __stdcall VS2010ExamplePlugin::ThreadStaticEntryPoint(void* pThis) {
    VS2010ExamplePlugin* pPlugin = (VS2010ExamplePlugin*)pThis;
    pPlugin->WorkerThread();
    return 0;
}

void VS2010ExamplePlugin::WorkerThread() {
    
    // Safety check
    if (!m_pBuffersIn || !m_pContext || !m_pBuffersIn->outSimDataIN) {
        m_isPluginRunning = false;
        return;
    }

    OutSimData* data = m_pBuffersIn->outSimDataIN;

    // 1. Indicate that this plugin is reporting data
    data->mPacketHeader.reportAvailable = 1;

    // 2. Setup TelemetryPlayer
    // Construct path: Documents/OpenSR/Map/demo.lap
    std::wstring path = m_pContext->osrDocFolder;
    path.append(L"\\Map\\demo.lap");

    // Initialize the player with the file path
    TelemetryPlayer my_car(path);

    // 3. Timing Setup
    const double dt = 0.02; // 20ms time step
    // VS2010: Convert to DWORD milliseconds for Sleep()
    const DWORD sleep_ms = (DWORD)(dt * 1000.0); 

    double total_elapsed_time = 0.016;

    while (!m_stopRequested) {
        
        // 4. Check if Host Process (Game/Sim) is still alive
        //if (!m_pContext->isAppRunning) {
        //    // Log output in VS2010 usually goes to Output Debug String or console if attached
        //    std::cerr << "[ExamplePlugin] Host context ended. Stopping." << std::endl;
        //    break;
        //}

        if (!OpenSRHelper::IsProcessRunningByPIDAndName(m_pContext->targetGamePId, TEXT(_TARGET_APP)) || !m_pContext->isAppRunning) {
            std::cerr << "[ExamplePlugin] Target process is no longer alive, stopping plugin thread." << std::endl;
            break;
        }
		
        // 5. Handle Pause (WinAPI style)
        // WaitForSingleObject blocks here until m_hPauseEvent is Signaled.
        // If m_isPluginPaused was true, the event was Reset (Red light), and we wait here efficiently.
        WaitForSingleObject(m_hPauseEvent, INFINITE);

        // Check stop again immediately after waking up, in case we were stopped while paused
        if (m_stopRequested) break;

        // 6. Update Physics Engine
        my_car.update(total_elapsed_time);

        // 7. Map Data to OpenSR Buffer
        // Note: Explicit casts to float are good practice for VS2010 strictness
        data->mPacketHeader.paused = 0;
        data->mPacketHeader.playerSlotIndex = 0;

        data->mVehicleData.gear = my_car.getGear();
        data->mVehicleData.maxGear = 6;
        data->mVehicleData.speed = (float)my_car.getSpeedMps();
        data->mVehicleData.rpm = (float)my_car.getRpm();
        data->mVehicleData.maxRpm = 7800.0f;

        // Session logic
        // fmod is available in <math.h> or <cmath>
        data->mSessionData.sessionTime = (float)fmod(total_elapsed_time, 138.0);

        // Player/Motion logic
        data->mPlayers.carCount = 1;
        data->mMotionData.localAccelX = 0.12f; // Dummy G-force for effect

        data->mPlayers.player[0].lapsCompleted = my_car.getLapCount();
        data->mPlayers.player[0].carSpeed = (float)my_car.getSpeedMps();
        data->mPlayers.player[0].lastLapTime = (float)my_car.getLastLapTime();
        data->mPlayers.player[0].currentLapTime = (float)my_car.getCurrentLapTime();

        // World Position
        data->mPlayers.player[0].worldPositionX = (float)my_car.getWorldX();
        data->mPlayers.player[0].worldPositionY = (float)my_car.getWorldY();
        data->mPlayers.player[0].worldPositionZ = (float)my_car.getWorldZ();

        // 8. Advance Time
        total_elapsed_time += dt;

        // 9. Submit Frame to Host
        if (m_pContext->submitFrameCallback) {
            // userData is passed back to the host so it knows context
            m_pContext->submitFrameCallback(m_pContext->userData);
        }

        // 10. Wait for next tick
		Sleep(m_pluginSettings.deviceUpdateDelay);
    }

    // Cleanup
    data->mPacketHeader.reportAvailable = 0;
    
    // Send one final frame to clear state
    if (m_pContext->submitFrameCallback) {
        m_pContext->submitFrameCallback(m_pContext->userData);
    }

    m_isPluginRunning = false;
}

bool VS2010ExamplePlugin::LoadSettings(const std::wstring& path, PluginSettings& plugSettings)
{

    tinyxml2::XMLDocument doc;
    
    // Use _wfsopen for Windows Unicode support
    FILE* file = _wfsopen(path.c_str(), L"rb", _SH_DENYNO);

    if (!file) {
        std::cerr << "Failed to open settings file (File not found or locked)." << std::endl;
        return false;
    }

    // Load the file using the FILE* handle
    XMLError error = doc.LoadFile(file);
    
    // Close the file immediately; we are done reading
    fclose(file); 

    if (error != XML_SUCCESS) {
        std::cerr << "Failed to parse XML structure." << std::endl;
        // Optional: print doc.ErrorStr()
        return false;
    }

    XMLElement* root = doc.FirstChildElement("settings");
    if (root == NULL) {
        return false;
    }

    XMLElement* option = root->FirstChildElement("option");
    while (option != NULL)
    {
        const char* id = option->Attribute("id");

        if (id != NULL)
        {
            if (strcmp(id, "info-display-delay") == 0) {
                option->QueryIntText(&plugSettings.holdInfoDelay);
            }
            else if (strcmp(id, "device-update-delay") == 0) {
                option->QueryIntText(&plugSettings.deviceUpdateDelay);
            }
            else if (strcmp(id, "global-brightness") == 0) {
                option->QueryIntText(&plugSettings.globalBrightness);
            }
            else if (strcmp(id, "udp-ip") == 0) {
                // GetText() can return NULL if the tag is empty: <option ...></option>
                const char* text = option->GetText();
                if (text) {
                    plugSettings.udpIp = text;
                }
            }
            else if (strcmp(id, "udp-port") == 0) {
                option->QueryIntText(&plugSettings.udpPort);
            }
        }

        option = option->NextSiblingElement("option");
    }

    return true;
}

