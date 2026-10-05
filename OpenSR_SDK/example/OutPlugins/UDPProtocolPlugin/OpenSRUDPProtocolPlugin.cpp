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

// OpenSRUDPProtocolPlugin.cpp
/*
    send telemetry data from outsimdata shared struct to UDP 
*/

#include "tinyxml2.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "OpenSRUDPProtocolPlugin.h"
#include "StringUtils.h"

using namespace tinyxml2;

typedef tinyxml2::XMLDocument TinyXMLDocument;

// Mandatory export symbols
extern "C" __declspec(dllexport) osr::IOpenSRPlugin * CreatePlugin() {
    return new OpenSRUDPProtocolPlugin();
}

extern "C" __declspec(dllexport) void DestroyPlugin(osr::IOpenSRPlugin * plugin) {
    if (plugin)
        delete plugin;
}

OpenSRUDPProtocolPlugin::OpenSRUDPProtocolPlugin() {}
OpenSRUDPProtocolPlugin::~OpenSRUDPProtocolPlugin() { Stop(); Shutdown(); }

bool OpenSRUDPProtocolPlugin::LoadPluginSettings(const std::wstring spath) {
    tinyxml2::XMLDocument doc;
    FILE* file = _wfsopen(spath.c_str(), L"rb", _SH_DENYNO);

    if (file) {
        // CASE 1: The file exists
        XMLError error = doc.LoadFile(file);
        fclose(file); // Close the file immediately.

        if (error == XML_SUCCESS) {
            XMLElement* root = doc.FirstChildElement("settings");
            if (root) {
                // Iterate over all <option> elements
                for (XMLElement* optionElem = root->FirstChildElement("option");
                    optionElem;
                    optionElem = optionElem->NextSiblingElement("option"))
                {
                    const char* nameAttr = optionElem->Attribute("id");
                    if (!nameAttr) continue;

                    const char* text = optionElem->GetText();
                    if (!text) continue;

                    if (std::strcmp(nameAttr, "ip") == 0) {
                        m_ip = text;
                    }
                    else if (std::strcmp(nameAttr, "port") == 0) {
                        m_port = static_cast<unsigned>(std::strtoul(text, nullptr, 10));
                    }
                    else if (std::strcmp(nameAttr, "motionRate") == 0) {
                        m_motionRate = std::atoi(text);
                    }
                    else if (std::strcmp(nameAttr, "vehicleStateRate") == 0) {
                        m_vehicleStateRate = std::atoi(text);
                    }
                    else if (std::strcmp(nameAttr, "sessionRate") == 0) {
                        m_sessionRate = std::atoi(text);
                    }
                    else if (std::strcmp(nameAttr, "wheelsRate") == 0) {
                        m_wheelRate = std::atoi(text);
                    }
                    else if (std::strcmp(nameAttr, "playerRecordsRate") == 0) {
                        m_playerRecordsRate = std::atoi(text);
                    }
                    else if (std::strcmp(nameAttr, "pluginRate") == 0) {
                        m_deviceRate = std::atof(text);
                    }
                }

                std::cout << "[OpenSR][UDP] Loaded settings from "
                    << std::string(spath.begin(), spath.end()) << std::endl;
                return true;
            }
            else {
                std::cerr << "[OpenSR][UDP] Settings file malformed (missing <settings>). Using defaults." << std::endl;
            }
        }
        else {
            std::cerr << "[OpenSR][UDP] Failed to parse settings file. Using defaults." << std::endl;
        }
    }
    else {
        // CASE 2: File does not exist -> create a new one with defaults
        std::cout << "[OpenSR][UDP] Settings file not found. Creating new one with defaults." << std::endl;

        doc.NewDeclaration("xml version=\"1.0\" encoding=\"UTF-8\"");
        XMLElement* root = doc.NewElement("settings");
        doc.InsertFirstChild(root);
        root->SetAttribute("type", "unique");

        auto createOption = [&](const char* name, const char* id, const char* type, auto value) {
            XMLElement* elem = doc.NewElement("option");
            elem->SetAttribute("name", name);
            elem->SetAttribute("id", id);
            elem->SetAttribute("type", type);
            elem->SetText(value);
            root->InsertEndChild(elem);
        };

        auto createOptionSlider = [&](const char* name, const char* id, const char* range, auto value) {
            XMLElement* elem = doc.NewElement("option");
            elem->SetAttribute("name", name);
            elem->SetAttribute("id", id);
            elem->SetAttribute("type", "slider");
            elem->SetAttribute("range", range);
            elem->SetText(value);
            root->InsertEndChild(elem);
        };

        createOption("ip", "ip", "text", m_ip.c_str());
        createOption("port", "port", "number", m_port);

        createOptionSlider("Motion Data Rate (ms)", "motionRate", "1,150", m_motionRate);
        createOptionSlider("Vehicle Data Rate (ms)", "vehicleStateRate", "1,150", m_vehicleStateRate);
        createOptionSlider("Session Data Rate (ms)", "sessionRate", "1,150", m_sessionRate);
        createOptionSlider("Wheels Data Rate (ms)", "wheelsRate", "1,150", m_wheelRate);
        createOptionSlider("Players Data Rate (ms)", "playerRecordsRate", "1,150", m_playerRecordsRate);

        FILE* fp = _wfsopen(spath.c_str(), L"wb", _SH_DENYNO);
        if (fp) {
            if (doc.SaveFile(fp) != XML_SUCCESS) {
                std::cerr << "[OpenSR][UDP] CRITICAL: Failed to create default settings file!" << std::endl;
            }
            fclose(fp);

            return true;
        }
        else {
            std::wcerr << L"[OpenSR][UDP] Failed to save settings file: " << spath << std::endl;
        }
    }
    return false;
}

bool OpenSRUDPProtocolPlugin::Init(OpenSRContext* context, void* buffersOUT, const wchar_t* pluginPath) {
    m_pContext = context;
    m_buffersOut = static_cast<OpenSRBuffersOUT*>(buffersOUT);
    m_pluginPath = pluginPath;
    // check params
    if (!m_pContext || !m_buffersOut || m_pluginPath.empty())
        return false;
   /* m_udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_udpSocket == INVALID_SOCKET) {
        std::cerr << "[OpenSR][UDP] Failed to create UDP socket." << std::endl;
        return false;
    }*/
   
    return true;
}

bool OpenSRUDPProtocolPlugin::Start() {
    // Reset the stop flag so threads can run again
    {
        std::lock_guard<std::mutex> lock(m_stopMutex);
        m_stopRequested = false;
    }

    // Create socket here if it was closed (Lazy Initialization)
    if (m_udpSocket == INVALID_SOCKET) {
        m_udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_udpSocket == INVALID_SOCKET) {
            std::cerr << "[OpenSR][UDP] Failed to create UDP socket." << std::endl;
            return false;
        }
    }

    // Load Settings
    std::wstring spath = StringUtils::createWString(m_pContext->osrDocFolder, L"\\", m_pluginPath, L"\\settings.xml");

    // Note: Removed the check for INVALID_SOCKET here because we handled it above
    if (LoadPluginSettings(spath)) {
        m_targetAddr.sin_family = AF_INET;
        m_targetAddr.sin_port = htons(m_port);

        if (inet_pton(AF_INET, m_ip.c_str(), &m_targetAddr.sin_addr) <= 0) {
            std::cerr << "Invalid IP address: " << m_ip << std::endl;
            return false;
        }
    }

    m_isPluginRunning = true;
    m_motionThread = std::thread(&OpenSRUDPProtocolPlugin::SenderThread, this, dataType::dtMotionData, m_motionRate);
    m_vehicleStateThread = std::thread(&OpenSRUDPProtocolPlugin::SenderThread, this, dataType::dtVehicleStateData, m_vehicleStateRate);
    m_sessionThread = std::thread(&OpenSRUDPProtocolPlugin::SenderThread, this, dataType::dtSessionData, m_sessionRate);
    m_wheelThread = std::thread(&OpenSRUDPProtocolPlugin::SenderThread, this, dataType::dtWheelData, m_wheelRate);
    m_playerRecordsThread = std::thread(&OpenSRUDPProtocolPlugin::SenderThread, this, dataType::dtPlayersData, m_playerRecordsRate);

    return true;
}

void OpenSRUDPProtocolPlugin::Stop() {
    // 1. Set the flag to true
    // We use a lock to ensure the change is visible to all threads before the notification.
    {
        std::lock_guard<std::mutex> lock(m_stopMutex);
        m_stopRequested = true;
    }

    // 2. Notify all threads that are waiting on the condition variable
    m_stopCv.notify_all();

    if (m_motionThread.joinable()) m_motionThread.join();
    if (m_vehicleStateThread.joinable()) m_vehicleStateThread.join();
    if (m_sessionThread.joinable()) m_sessionThread.join();
    if (m_wheelThread.joinable()) m_wheelThread.join();
    if (m_playerRecordsThread.joinable()) m_playerRecordsThread.join();
}

void OpenSRUDPProtocolPlugin::Shutdown() {
    if (m_udpSocket != INVALID_SOCKET) {
        closesocket(m_udpSocket);
        m_udpSocket = INVALID_SOCKET;
    }
    m_isPluginRunning = false;
}


bool OpenSRUDPProtocolPlugin::IsSettingsReady() {
    // 1. If already synced, return true immediately (don't re-sync on every UI build)
    if (m_syncCompleted) return true;

    // 2. If syncing is in progress, return false (host will keep waiting)
    if (m_syncInProgress) return false;

    // 3. Safety check: If motion is active, sync is unsafe. Assume XML is ready.
   /* if (something in progress) {
        m_syncCompleted = true;
        return true;
    }*/

    // 4. Start the sync in the background and tell host to wait
    m_syncInProgress = true;
    m_syncCompleted = false;

    // Load Settings
    std::wstring spath = StringUtils::createWString(m_pContext->osrDocFolder, L"\\", m_pluginPath, L"\\settings.xml");

    std::thread([this, spath] {
        // do any init stuff with settings here

        // Signal host xml settings is ready
        m_syncCompleted = true;
        m_syncInProgress = false;
    }).detach();

    return false;
}

void OpenSRUDPProtocolPlugin::OnContextChanged(osr::OpenSRContextChange reason) {
    if (reason == osr::OpenSRContextChange::SettingsChanged) {
        std::wcout << "[FORWARD UDP PLUGIN] Settings need to be reloaded.\n";

        // 1. Stop threads
        Stop();

        // 2. Explicitly close the socket. 
        // This ensures the OS releases the handle and any bound ports/states.
        if (m_udpSocket != INVALID_SOCKET) {
            closesocket(m_udpSocket);
            m_udpSocket = INVALID_SOCKET;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Small buffer

        // 3. Start will now recreate the socket with new settings and fresh flags
        Start();
    }
    
}

void OpenSRUDPProtocolPlugin::SenderThread(dataType type, int intervalMs) {
    m_isPluginRunning = true;
    while (!m_stopRequested && m_pContext && m_pContext->isAppRunning ) {
        // Pause Handling
        {
            std::unique_lock<std::mutex> lock(m_pauseMutex);
            // Wait while m_isPaused is true. This efficiently blocks the thread
            // without using CPU cycles, until Resume() is called.
            m_pauseCv.wait(lock, [this] { return !m_isPluginPaused; });
        }

        // Check for stop request *after* waking from pause, before doing work
        if (m_stopRequested) {
            break;
        }

        if(m_buffersOut->outSimDataOUT && m_buffersOut->outSimDataOUT->mPacketHeader.reportAvailable)
            SendPacket(type);

        std::unique_lock<std::mutex> lock(m_stopMutex);
        m_stopCv.wait_for(lock, std::chrono::milliseconds(intervalMs), [this] {
            // This predicate tells the wait to unblock immediately if m_stopRequested is true
            return m_stopRequested.load();
            });
        //std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
    }
    m_isPluginRunning = false;
}

void OpenSRUDPProtocolPlugin::SendPacket(dataType type) {
    if (!m_pContext) return;
    PacketHeader_t header;
    memcpy(&header, &m_buffersOut->outSimDataOUT->mPacketHeader, sizeof(PacketHeader_t));
    header.packetType = static_cast<uint8_t>(type);

    char buffer[2048] = {};
    memcpy(buffer, &header, sizeof(header));
    size_t dataSize = sizeof(header);

    OutSimData* outG = m_buffersOut->outSimDataOUT;

    switch (type) {
    case dataType::dtMotionData:
        memcpy(buffer + dataSize, &outG->mMotionData, sizeof(outG->mMotionData));
        dataSize += sizeof(outG->mMotionData);
        break;
    case dataType::dtVehicleStateData:
        memcpy(buffer + dataSize, &outG->mVehicleData, sizeof(outG->mVehicleData));
        dataSize += sizeof(outG->mVehicleData);
        break;
    case dataType::dtSessionData:
        memcpy(buffer + dataSize, &outG->mSessionData, sizeof(outG->mSessionData));
        dataSize += sizeof(outG->mSessionData);
        break;
    case dataType::dtWheelData:
        memcpy(buffer + dataSize, &outG->mWheelData, sizeof(outG->mWheelData));
        dataSize += sizeof(outG->mWheelData);
        break;
    case dataType::dtPlayersData: {
        const PlayerRecords_t* records = &outG->mPlayers;
        if (!records) return;

        for (int i = 0; i < records->carCount; ++i) {
            PacketHeader_t perCarHeader;
            memcpy(&perCarHeader, &outG->mPacketHeader, sizeof(PacketHeader_t));
            perCarHeader.packetType = static_cast<uint8_t>(dataType::dtPlayersData);
            perCarHeader.packetIndex = static_cast<uint8_t>(i);

            char carBuffer[1024] = {};
            memcpy(carBuffer, &perCarHeader, sizeof(PacketHeader_t));
            memcpy(carBuffer + sizeof(PacketHeader_t), &records->player[i], sizeof(PlayerInfo_t));

            size_t totalSize = sizeof(PacketHeader_t) + sizeof(PlayerInfo_t);
            sendto(m_udpSocket, carBuffer, static_cast<int>(totalSize), 0, (sockaddr*)&m_targetAddr, sizeof(m_targetAddr));
        }
        return; // already sent all packets
    }
    default:
        return;
    }

    sendto(m_udpSocket, buffer, static_cast<int>(dataSize), 0, (sockaddr*)&m_targetAddr, sizeof(m_targetAddr));
}
