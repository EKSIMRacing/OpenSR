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
// OpenSRUDPClient.cpp
// Simple UDP client to receive OpenSR UDP telemetry packets and display values.

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <cstdint>
#include <cstring>
#include <thread>

#include "OpenSROutSimData.h"
#pragma comment(lib, "Ws2_32.lib")


// Helper to print current time
std::string nowStr() {
    using namespace std::chrono;
    auto t = system_clock::now();
    std::time_t tt = system_clock::to_time_t(t);
    char buf[64];
    ctime_s(buf, sizeof(buf), &tt);
    // strip newline
    std::string s(buf);
    if (!s.empty() && s.back() == '\n') s.pop_back();
    return s;
}

void handleMotion(const PacketHeader_t& hdr, const MotionData_t& m) {
    std::cout << "[" << nowStr() << "] MotionData frame:" << hdr.frameId
        << " accelX=" << m.localAccelX
        << " velocityX=" << m.velocityX
        << " velocityZ=" << m.velocityZ
        << " reportAvail=" << int(hdr.reportAvailable)
        << std::endl;
}

void handleVehicle(const PacketHeader_t& hdr, const VehicleData_t& v) {
    std::cout << "[" << nowStr() << "] VehicleData frame:" << hdr.frameId
        << " rpm=" << v.rpm
        << " speed(m/s)=" << v.speed
        << " gear=" << int(v.gear)
        << std::endl;
}

void handleSession(const PacketHeader_t& hdr, const SessionData_t& s) {
    // ensure trackName is null-terminated for safety
    std::string track(s.trackName, strnlen(s.trackName, sizeof(s.trackName)));
    std::cout << "[" << nowStr() << "] SessionData frame:" << hdr.frameId
        << " sessionType=" << int(s.sessionType)
        << " trackLength=" << s.trackLength
        << " trackName=\"" << track << "\""
        << std::endl;
}

void handleWheel(const PacketHeader_t& hdr, const WheelData_t& w) {
    // Show wheel 0 summary
    const WheelInfo_t& wheel0 = w.wheels[0];
    std::cout << "[" << nowStr() << "] WheelData frame:" << hdr.frameId
        << " wheel0_speed=" << wheel0.wheelSpeed
        << " tyreTempInner=" << wheel0.tyreTempInner
        << " tyreTempCore=" << wheel0.tyreTempCore
        << std::endl;
}

void handlePlayerRecord(const PacketHeader_t& hdr, const PlayerInfo_t& p) {
    std::string name(p.carName, strnlen(p.carName, sizeof(p.carName)));
    std::cout << "[" << nowStr() << "] PlayerRecord frame:" << hdr.frameId
        << " carInfoIndex=" << int(hdr.playerSlotIndex)
        << " position=" << int(p.position)
        << " bestLapTime=" << p.bestLapTime
        << " carName=\"" << name << "\""
        << std::endl;
}

int main(int argc, char** argv) {
    unsigned short LISTEN_PORT = 40445; // default; change to your plugin port or pass as arg

    if (argc >= 2) {
        LISTEN_PORT = static_cast<unsigned short>(std::stoi(argv[1]));
    }

    std::cout << "OpenSR UDP client starting. Listening port: " << LISTEN_PORT << std::endl;

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "socket failed\n";
        WSACleanup();
        return 1;
    }

    //sockaddr_in localAddr;
    //memset(&localAddr, 0, sizeof(localAddr));
    //localAddr.sin_family = AF_INET;
    //localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    //localAddr.sin_port = htons(LISTEN_PORT);

    //if (bind(sock, (sockaddr*)&localAddr, sizeof(localAddr)) == SOCKET_ERROR) {
    //    std::cerr << "bind failed, error: " << WSAGetLastError() << std::endl;
    //    closesocket(sock);
    //    WSACleanup();
    //    return 1;
    //}

    // Explicit bind to the port you expect
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(LISTEN_PORT);   // same port server sends to
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "bind failed: " << WSAGetLastError() << std::endl;
        return false;
    }
    std::cout << "Bound. Waiting for packets..." << std::endl;

    const int BUF_SZ = 8192;
    char buf[BUF_SZ];
    sockaddr_in from;
    int fromlen = sizeof(from);

    while (true) {

        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        int rec = recvfrom(sock, buf, BUF_SZ, 0, (sockaddr*)&from, &fromlen);
        if (rec == SOCKET_ERROR) {
            int err = WSAGetLastError();
            if (err == WSAEINTR || err == WSAEWOULDBLOCK) continue;
            std::cerr << "recvfrom error: " << err << std::endl;
            break;
        }
        if (rec < (int)sizeof(PacketHeader_t)) {
            std::cerr << "Received packet too small: " << rec << std::endl;
            continue;
        }

        PacketHeader_t hdr;
        memcpy(&hdr, buf, sizeof(PacketHeader_t));

        // Basic sanity checks
        if (!hdr.reportAvailable) {
            // skip if not available
            // but still process? we'll log and skip
            std::cout << "[" << nowStr() << "] Received packet but reportAvailable==0, skipping. frame:" << hdr.frameId << std::endl;
            continue;
        }

        // Basic sanity checks
        if (hdr.paused) {
            // skip if not available
            // but still process? we'll log and skip
            std::cout << "[" << nowStr() << "] Received packet but paused==1, skipping. frame:" << hdr.frameId << std::endl;
            continue;
        }

        dataType dt = static_cast<dataType>(hdr.packetType);

        // Determine expected sizes for safety / prevent overruns.
        switch (dt) {
        case dataType::dtMotionData:
            if (rec >= (int)(sizeof(PacketHeader_t) + sizeof(MotionData_t))) {
                MotionData_t m;
                memcpy(&m, buf + sizeof(PacketHeader_t), sizeof(MotionData_t));
                handleMotion(hdr, m);
            }
            else {
                std::cerr << "MotionData: packet too small\n";
            }
            break;
        case dataType::dtVehicleStateData:
            if (rec >= (int)(sizeof(PacketHeader_t) + sizeof(VehicleData_t))) {
                VehicleData_t v;
                memcpy(&v, buf + sizeof(PacketHeader_t), sizeof(VehicleData_t));
                handleVehicle(hdr, v);
            }
            else {
                std::cerr << "VehicleData: packet too small\n";
            }
            break;
        case dataType::dtSessionData:
            if (rec >= (int)(sizeof(PacketHeader_t) + sizeof(SessionData_t))) {
                SessionData_t s;
                memcpy(&s, buf + sizeof(PacketHeader_t), sizeof(SessionData_t));
                handleSession(hdr, s);
            }
            else {
                std::cerr << "SessionData: packet too small\n";
            }
            break;
        case dataType::dtWheelData:
            if (rec >= (int)(sizeof(PacketHeader_t) + sizeof(WheelData_t))) {
                WheelData_t w;
                memcpy(&w, buf + sizeof(PacketHeader_t), sizeof(WheelData_t));
                handleWheel(hdr, w);
            }
            else {
                std::cerr << "WheelData: packet too small\n";
            }
            break;
        case dataType::dtPlayersData:
            // your sender sends per-car packet: header + PlayerInfo_t
            if (rec >= (int)(sizeof(PacketHeader_t) + sizeof(PlayerInfo_t))) {
                PlayerInfo_t p;
                memcpy(&p, buf + sizeof(PacketHeader_t), sizeof(PlayerInfo_t));
                handlePlayerRecord(hdr, p);
            }
            else {
                std::cerr << "PlayerRecordsData: packet too small\n";
            }
            break;
        default:
            std::cerr << "Unknown packetType: " << int(hdr.packetType) << std::endl;
            break;
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
