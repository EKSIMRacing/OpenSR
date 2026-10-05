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
#include "TelemetryPlayer.h"
#include <cmath>    
#include <algorithm>
#include <vector>

// VS2010 Helper: std::round is C++11. 
// We implement a simple version here.
static double simple_round(double r) {
    return (r > 0.0) ? floor(r + 0.5) : ceil(r - 0.5);
}

// Helper function for linear interpolation
template<typename T>
T lerp(T a, T b, double t) {
    return static_cast<T>(a + (b - a) * t);
}

// Overloaded helper for 3D points
Point3D lerp(Point3D a, Point3D b, double t) {
    return Point3D(
        lerp(a.x, b.x, t),
        lerp(a.y, b.y, t),
        lerp(a.z, b.z, t)
    );
}

// VS2010: max/min often require explicit std:: or macro avoidance
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif


TelemetryPlayer::TelemetryPlayer(const std::wstring& path) 
    : current_index(0), 
      current_speed_ms(0.0), current_gear(0), current_rpm(0),
      lap_count_(1), lap_timer_(0.0), last_lap_time_(0.0),
      is_first_entry_(true) // Initialization moved to initializer list
{
    try {
        lap_data = loadTelemetryDataFromFile(path);
    }
    catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return; 
    }

    if (!lap_data.empty()) {
        const TelemetryPoint& start_point = lap_data[0];
        current_speed_ms = start_point.speed_ms;
        current_gear = start_point.gear;
        current_rpm = start_point.rpm;
        current_world_position = start_point.position;
    }
}

std::vector<TelemetryPoint> TelemetryPlayer::loadTelemetryDataFromFile(const std::wstring& filePath) {

    std::vector<TelemetryPoint> telemetryData;

    // VS2010 supports wchar_t* in ifstream constructor as an extension
    std::ifstream file(filePath.c_str());

    if (!file.is_open()) {
        // Fallback or error handling
        // Note: VS2010 might fail silently on std::string construction from wstring implies
        // but passing c_str() to ifstream usually works in MSVC.
        throw std::runtime_error("Error: Could not open file");
    }

    std::string line;
    int lineNumber = 0;

    while (std::getline(file, line)) {
        lineNumber++;

        if (line.empty()) continue;

        std::stringstream ss(line);
        TelemetryPoint point;
        char comma; 

        double temp_gear = 0;
        double temp_rpm = 0;

        if ((ss >> point.timestamp_ms >> comma) && (comma == ',') &&
            (ss >> point.speed_ms >> comma) && (comma == ',') &&
            (ss >> temp_gear >> comma) && (comma == ',') &&
            (ss >> temp_rpm >> comma) && (comma == ',') &&
            (ss >> point.position.x >> comma) && (comma == ',') &&
            (ss >> point.position.y >> comma) && (comma == ',') &&
            (ss >> point.position.z))
        {
            // Use local simple_round instead of std::round
            point.gear = static_cast<int>(simple_round(temp_gear));
            point.rpm = static_cast<int>(simple_round(temp_rpm));
            telemetryData.push_back(point);
        }
        else {
            std::cerr << "Warning: Could not parse line " << lineNumber
                << " in file map: " << line << std::endl;
        }
    }

    return telemetryData;
}

void TelemetryPlayer::update(double total_elapsed_time_s) {
    if (lap_data.size() < 2) {
        return;
    }

    const double total_recording_duration_ms = (double)lap_data.back().timestamp_ms;
    const double total_recording_duration_s = total_recording_duration_ms / 1000.0;

    if (total_recording_duration_s <= 0.0) return;

    lap_timer_ = fmod(total_elapsed_time_s, total_recording_duration_s);
    int target_time_ms = static_cast<int>(lap_timer_ * 1000.0);

    lap_count_ = static_cast<int>(total_elapsed_time_s / total_recording_duration_s);
    if (lap_count_ > 0) {
        last_lap_time_ = total_recording_duration_s;
    }

    if (target_time_ms < lap_data[current_index].timestamp_ms) {
        current_index = 0;
    }

    while (current_index < lap_data.size() - 1 &&
        lap_data[current_index + 1].timestamp_ms <= target_time_ms)
    {
        current_index++;
    }

    // VS2010: Replace 'const auto&' with explicit type
    const TelemetryPoint& prev_point = lap_data[current_index];
    const TelemetryPoint& next_point = (current_index == lap_data.size() - 1) ? lap_data[0] : lap_data[current_index + 1];

    int segment_start_ms = prev_point.timestamp_ms;
    int segment_end_ms = next_point.timestamp_ms;

    if (segment_end_ms < segment_start_ms) {
        segment_end_ms = static_cast<int>(total_recording_duration_ms);
    }

    int segment_duration_ms = segment_end_ms - segment_start_ms;

    if (segment_duration_ms <= 0) {
        current_speed_ms = prev_point.speed_ms;
        current_rpm = prev_point.rpm;
        current_gear = prev_point.gear;
        current_world_position = prev_point.position;
    }
    else {
        double t = static_cast<double>(target_time_ms - segment_start_ms) / segment_duration_ms;
        
        // Manual clamp for VS2010 if std::min/max behave oddly with macros
        if(t < 0.0) t = 0.0;
        if(t > 1.0) t = 1.0;

        current_speed_ms = lerp(prev_point.speed_ms, next_point.speed_ms, t);
        current_rpm = static_cast<int>(lerp((double)prev_point.rpm, (double)next_point.rpm, t));
        current_world_position = lerp(prev_point.position, next_point.position, t);
        current_gear = prev_point.gear; 
    }
}

double TelemetryPlayer::getLapDuration() const {
    if (lap_data.empty()) return 0.0;
    return static_cast<double>(lap_data.back().timestamp_ms) / 1000.0;
}