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
// OpenSRUDPProtocolPlugin.h
#pragma once

#include <thread>
#include <atomic>
#include <mutex>
#include <string>
#include <cstdint>

#include "OpenSRBuffers.h"
#include "OpenSRHelper.h"
#include "IOpenSRPlugin.h"

#pragma comment(lib, "ws2_32.lib")

class OpenSRUDPProtocolPlugin : public osr::IOpenSRPlugin {
public:
    OpenSRUDPProtocolPlugin();
    ~OpenSRUDPProtocolPlugin() override;

    bool Init(OpenSRContext* context, void* buffersOUT, const wchar_t* pluginPath) override;
    bool Start() override;
    void Stop() override;
    void Shutdown() override;

    void Pause() override {
        m_isPluginPaused = true;
    }

    void Resume() override {
        m_isPluginPaused = false;
        m_pauseCv.notify_one(); // Wake up the thread if it's waiting
    }

    bool IsRunning() const override {
        return m_isPluginRunning;
    }
	
	void GetPluginName(char* buffer, size_t bufferSize) const override {
        strcpy_s(buffer, bufferSize, "Forward UDP Protocol Plugin");
    }
    void GetAuthor(char* buffer, size_t bufferSize) const override {
        strcpy_s(buffer, bufferSize,  "Zappdoc");
    }
    void GetVersion(char* buffer, size_t bufferSize) const override {
        strcpy_s(buffer, bufferSize, "0.0.1");
    }
    void GetLicenseType(char* buffer, size_t bufferSize) const override {
        strcpy_s(buffer, bufferSize, "MIT");
    }
    void GetDescription(char* buffer, size_t bufferSize) const override {
        strcpy_s(buffer, bufferSize,
            "Forward OpenSR telemetry data over UDP by category:\n"
            "PacketHeader + MotionData\n"
            "PacketHeader + SessionData\n"
            "PacketHeader + VehicleData\n"
            "PacketHeader + WheelData\n"
            "PacketHeader + Player (loop on PlayerRecords[car count])\n"
            "Check the OutSimData.h and UDP Client app source code in OpenSR SDK.");
    }
    void GetTargetName(char* buffer, size_t bufferSize) const override {
        strcpy_s(buffer, bufferSize, "");
    }
    void GetTargetProcessName(char* buffer, size_t bufferSize) const override {
        strcpy_s(buffer, bufferSize, "");
    }
	
    void GetPackageName(char* buffer, size_t bufferSize) const override {
        strncpy_s(buffer, bufferSize, "com.zappadoc.outplugin.udpforwarder", _TRUNCATE);
    }

    int GetType() override { return OUT_PLUGIN_TYPE; }

    void GetPluginGroupName(wchar_t* buffer, size_t bufferSize) const override {
        wcscpy_s(buffer, bufferSize, L"");
    }

    void OnContextChanged(osr::OpenSRContextChange reason) override;

    bool IsSettingsReady() override; // sync settings

private:
    std::atomic<bool> m_syncInProgress = false;  // Tells host to wait
    std::atomic<bool> m_syncCompleted = false;   // Tells host settigns sync is finished

    void SenderThread(dataType type, int intervalMs);
    void SendPacket(dataType type);

    bool LoadPluginSettings(const std::wstring path);

    std::thread m_motionThread;
    std::thread m_vehicleStateThread;
    std::thread m_sessionThread;
    std::thread m_wheelThread;
    std::thread m_playerRecordsThread;
    
    std::mutex m_stopMutex;
    std::condition_variable m_stopCv;

    OpenSRContext* m_pContext = nullptr;
    OpenSRBuffersOUT* m_buffersOut = nullptr;
    std::wstring m_pluginPath;

    SOCKET m_udpSocket = INVALID_SOCKET;
    sockaddr_in m_targetAddr = {};

    std::string m_ip = "127.0.0.1";
    uint32_t m_port = 40444;
    int32_t m_motionRate = 1;          // in ms (~60Hz)
    int32_t m_vehicleStateRate = 16;   // in ms
    int32_t m_sessionRate = 100;       // in ms
    int32_t m_wheelRate = 100;      // in ms
    int32_t m_playerRecordsRate = 16; // ms

    uint64_t m_frameId = 0;

    std::atomic<bool> m_isPluginRunning = false;
    std::atomic<bool> m_stopRequested = false;

    std::atomic<bool> m_isPluginPaused = false;
    std::mutex m_pauseMutex;
    std::condition_variable m_pauseCv;

    float m_deviceRate = 1.0f;
};




