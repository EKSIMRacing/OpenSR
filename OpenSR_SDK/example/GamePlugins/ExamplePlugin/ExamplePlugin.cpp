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

#include "ExamplePlugin.h"

#include <iostream>
#include <chrono>
#include <thread>
#include "OpenSRHelper.h"
#include "TelemetryPlayer.h"
#include "StringUtils.h"
#include "tinyxml2.h"

using namespace tinyxml2;

// DLL Exported Factory Functions
extern "C" __declspec(dllexport) osr::IOpenSRPlugin * CreatePlugin() {
    return new ExamplePlugin();
}

extern "C" __declspec(dllexport) void DestroyPlugin(osr::IOpenSRPlugin * plugin) {
    if (plugin)
        delete plugin;
}

ExamplePlugin::ExamplePlugin() {}
ExamplePlugin::~ExamplePlugin() {}

bool ExamplePlugin::Init(OpenSRContext* context, void* inBuf, const wchar_t* pluginPath) {
    std::cout << "[ExamplePlugin] Init\n";
    m_pContext = context;
    m_pBuffersIn = static_cast<OpenSRBuffersIN*>(inBuf);
    m_pluginPath = pluginPath;

    
    return true;
}

bool ExamplePlugin::Start() {
    std::cout << "[ExamplePlugin] Start\n";
    if (m_isPluginRunning) {
        return false; // Already running
    }

    m_stopRequested = false;
    m_isPluginPaused = false;
    m_isPluginRunning = true;
    workerThread = std::thread(&ExamplePlugin::WorkerThread, this);
    return true;
}

void ExamplePlugin::Stop() {
    std::cout << "[ExamplePlugin] Stop\n";
    m_stopRequested = true;
    // If the thread is paused, we must resume it so it can check the stop flag
    if (m_isPluginPaused) {
        Resume();
    }
    if (workerThread.joinable())
        workerThread.join();
}

void ExamplePlugin::Shutdown() {
    std::cout << "[ExamplePlugin] Shutdown\n";
}

void ExamplePlugin::WorkerThread() {

  
    OutSimData* data = m_pBuffersIn->outSimDataIN;

   
   data->mPacketHeader.reportAvailable = 1;
   std::wstring path = m_pContext->osrDocFolder;
   path.append(L"\\Map\\demo.lap");
   TelemetryPlayer  my_car(path);
   // Set a constant time step for physics updates
   const double dt = 0.02;
   // Convert to milliseconds for the sleep function
   const auto update_interval = std::chrono::milliseconds(static_cast<long long>(dt * 1000));
   double total_elapsed_time = 0.016;
   while (!m_stopRequested) {
        if (!OpenSRHelper::IsProcessRunningByPIDAndName(m_pContext->targetGamePId, TEXT(_TARGET_APP)) || !m_pContext->isAppRunning) {
            std::cerr << "[ExamplePlugin] Target process is no longer alive, stopping plugin thread." << std::endl;
            break;
        }
 
        // Pause Handling
        {
            std::unique_lock<std::mutex> lock(m_pauseMutex);
            // Wait while m_isPaused is true. This efficiently blocks the thread
            // without using CPU cycles, until Resume() is called.
            m_pauseCv.wait(lock, [this] { return !m_isPluginPaused; });
        }

        my_car.update(total_elapsed_time);

        data->mPacketHeader.paused = 0;
        data->mPacketHeader.playerSlotIndex = 0;

        data->mVehicleData.gear = my_car.getGear();
        data->mVehicleData.maxGear = 6;
        data->mVehicleData.speed = my_car.getSpeedMps();

        data->mVehicleData.rpm = my_car.getRpm();
        data->mVehicleData.maxRpm = 7800.0f;

        data->mSessionData.sessionTime = fmod(total_elapsed_time, 138.0);

        data->mPlayers.carCount = 1;
        data->mMotionData.localAccelX = 0.12;
        data->mPlayers.player[0].lapsCompleted = my_car.getLapCount();
        data->mPlayers.player[0].carSpeed = data->mVehicleData.speed;
        data->mPlayers.player[0].lastLapTime = my_car.getLastLapTime();
        data->mPlayers.player[0].currentLapTime = my_car.getCurrentLapTime();

        data->mPlayers.player[0].worldPositionX = my_car.getWorldX();
        data->mPlayers.player[0].worldPositionY = my_car.getWorldY();
        data->mPlayers.player[0].worldPositionZ = my_car.getWorldZ();


        total_elapsed_time += dt;

        m_pContext->submitFrameCallback(m_pContext->userData);
        std::this_thread::sleep_for(update_interval);
      }
    data->mPacketHeader.reportAvailable = 0;
    m_pContext->submitFrameCallback(m_pContext->userData);
    m_isPluginRunning = false;
}

// callback (event from host app after editing plugin settings
void ExamplePlugin::OnContextChanged(osr::OpenSRContextChange reason) {
    if (m_pContext && reason == osr::OpenSRContextChange::SettingsChanged) {
        if (!m_pluginPath.empty()) {
            std::wstring settingsPath = StringUtils::createWString(m_pContext->osrDocFolder, L"\\", m_pluginPath, "\\settings.xml");
            std::wcout << "[Example Plugin] Profile need to be reloaded.\nProfile Path: " << settingsPath << std::endl;
            // see an example of load settings function below
            if (LoadPluginSettings(settingsPath, m_pluginSettings)) {
                // ...
            }
        }
    }
}

bool ExamplePlugin::LoadPluginSettings(const std::wstring& filepath, PluginSettings& s) {
    tinyxml2::XMLDocument doc;

    FILE* file = _wfsopen(filepath.c_str(), L"rb", _SH_DENYNO);
    if (!file) {
        std::wcerr << L"Failed to open file: " << filepath << std::endl;
        return false;
    }

    tinyxml2::XMLError error = doc.LoadFile(file);
    fclose(file);
    if (error != XML_SUCCESS)
        return false; // File missing or error, defaults remain

    XMLElement* root = doc.FirstChildElement("settings");
    if (!root) return false;

    for (XMLElement* opt = root->FirstChildElement("option"); opt; opt = opt->NextSiblingElement("option")) {
        const char* name = opt->Attribute("name");
        const char* value = opt->GetText();
        if (!name || !value) continue;

        if (strcmp(name, "DATA OUT IP ADDRESS") == 0)
            s.outgauge_ip = value;
        else if (strcmp(name, "DATA OUT PORT") == 0)
            s.outgauge_port = atoi(value);
    }
    return true;
}
