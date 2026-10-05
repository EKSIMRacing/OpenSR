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

#ifndef TELEMETRY_PLAYER_H
#define TELEMETRY_PLAYER_H

#include <vector>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>


// Represents a point in 3D space
struct Point3D {
    double x;
    double y;
    double z;

    // VS2010: Constructor required for initialization
    Point3D() : x(0.0), y(0.0), z(0.0) {}
    Point3D(double _x, double _y, double _z) : x(_x), y(_y), z(_z) {}
};

// Holds a single, complete snapshot of telemetry data.
struct TelemetryPoint {
    int timestamp_ms;
    double speed_ms;
    int gear;
    int rpm;
    Point3D position;

    // VS2010: Constructor for safety
    TelemetryPoint() : timestamp_ms(0), speed_ms(0.0), gear(0), rpm(0) {}
};

class TelemetryPlayer {
public:
    // Constructor
    TelemetryPlayer(const std::wstring& path);

    std::vector<TelemetryPoint> loadTelemetryDataFromFile(const std::wstring& filePath);

    void update(double total_elapsed_time_s);

    // Getters
    double getSpeedMps() const { return current_speed_ms; }
    double getSpeedKph() const { return current_speed_ms * 3.6; }
    int getGear() const { return current_gear; }
    int getRpm() const { return current_rpm; }
    double getLapDuration() const;

    double getWorldX() const { return current_world_position.x; }
    double getWorldY() const { return current_world_position.y; }
    double getWorldZ() const { return current_world_position.z; }
    Point3D getWorldPosition() const { return current_world_position; }

    int getLapCount() const { return lap_count_; }
    double getCurrentLapTime() const { return lap_timer_; }
    double getLastLapTime() const { return last_lap_time_; }

private:
    std::vector<TelemetryPoint> lap_data;
    size_t current_index;

    double current_speed_ms;
    int current_gear;
    int current_rpm;
    Point3D current_world_position;

    int lap_count_;
    double lap_timer_;
    double last_lap_time_;

    std::ofstream outfile_;
    bool is_first_entry_; // Initialized in constructor
};

#endif // TELEMETRY_PLAYER_H