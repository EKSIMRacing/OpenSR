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
//TelemetryPlayer.h
#pragma once
#ifndef TELEMETRY_PLAYER_H
#define TELEMETRY_PLAYER_H

#include <vector>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

// Represents a point in 3D space (z=forward, y=up, x=side-to-side)
struct Point3D {
    double x = 0.0, y = 0.0, z = 0.0;
};
// Holds a single, complete snapshot of telemetry data.
// Speed is now stored in meters per second (m/s).
struct TelemetryPoint {
    int timestamp_ms;
    double speed_ms;
    int gear;
    int rpm;
    Point3D position;
};



class TelemetryPlayer {
public:
    // Constructor: Initializes the car and its telemetry data.
    TelemetryPlayer(const std::wstring path);

    std::vector<TelemetryPoint> loadTelemetryDataFromFile(const std::wstring filePath);

    // Updates the car's state based on the total elapsed time of the simulation.
    void update(double total_elapsed_time_s);

    // Public Getters
    double getSpeedMps() const { return current_speed_ms; }
    double getSpeedKph() const { return current_speed_ms * 3.6; }
    int getGear() const { return current_gear; }
    int getRpm() const { return current_rpm; }
    double getLapDuration() const;

    // World Position Getters
    double getWorldX() const { return current_world_position.x; }
    double getWorldY() const { return current_world_position.y; }
    double getWorldZ() const { return current_world_position.z; }
    Point3D getWorldPosition() const { return current_world_position; } // Often more 

      // ================== NEW LAP TIMING GETTERS ==================
    int getLapCount() const { return lap_count_; }
    double getCurrentLapTime() const { return lap_timer_; }
    double getLastLapTime() const { return last_lap_time_; }


private:
    // Telemetry Data & State
    std::vector<TelemetryPoint> lap_data;
    size_t current_index;

    // Current State Variables
    double current_speed_ms;
    int current_gear;
    int current_rpm;

    Point3D current_world_position;
    // ================== NEW LAP TIMING MEMBERS ==================
    int lap_count_;
    double lap_timer_;
    double last_lap_time_;

    // TELEMETRY_DUMPER
    std::ofstream outfile_;
    bool is_first_entry_ = true; // State to correctly handle commas between lines
};

#endif // TELEMETRY_PLAYER_H

