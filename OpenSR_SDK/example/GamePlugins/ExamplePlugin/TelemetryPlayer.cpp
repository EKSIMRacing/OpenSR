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

// Helper function for linear interpolation of scalar values
template<typename T>
T lerp(T a, T b, double t) {
    return static_cast<T>(a + (b - a) * t);
}

// Overloaded helper function to interpolate a 3D point
Point3D lerp(Point3D a, Point3D b, double t) {
    return {
        lerp(a.x, b.x, t),
        lerp(a.y, b.y, t),
        lerp(a.z, b.z, t)
    };
}

// Definition of the new function
std::vector<TelemetryPoint> TelemetryPlayer::loadTelemetryDataFromFile(const std::wstring filePath) {

    std::vector<TelemetryPoint> telemetryData;

    // 1. Open the file
    std::ifstream file(filePath);

    // 2. Check if the file was successfully opened
    if (!file.is_open()) {
        throw std::runtime_error("Error: Could not open file");
    }

    std::string line;
    int lineNumber = 0;

    // 3. Read the file line by line
    while (std::getline(file, line)) {
        lineNumber++;

        // Skip empty lines
        if (line.empty()) {
            continue;
        }

        std::stringstream ss(line);
        TelemetryPoint point;
        char comma; // To consume the commas

        double temp_gear;
        double temp_rpm;

        // 4. Parse the line, extracting 7 values and expecting 6 commas
        if ((ss >> point.timestamp_ms >> comma) && (comma == ',') &&
            (ss >> point.speed_ms >> comma) && (comma == ',') &&
            (ss >> temp_gear >> comma) && (comma == ',') &&
            (ss >> temp_rpm >> comma) && (comma == ',') &&
            (ss >> point.position.x >> comma) && (comma == ',') &&
            (ss >> point.position.y >> comma) && (comma == ',') &&
            (ss >> point.position.z))
        {
            // If parsing is successful, add the point to our vector
            point.gear = static_cast<int>(std::round(temp_gear));
            point.rpm = static_cast<int>(std::round(temp_rpm));
            telemetryData.push_back(point);
        }
        else {
            // If the line is malformed, print a warning and continue
            std::cerr << "Warning: Could not parse line " << lineNumber
                << " in file map: " << line << std::endl;
        }
    }

    return telemetryData;
}

// NOTE: This code assumes the existence of the following member variables
// in your TelemetryPlayer class, as per your original function:
// - std::vector<TelemetryPoint> lap_data;
// - size_t current_index;
// - int lap_count_;
// - double last_lap_time_;
// - double lap_timer_;
// - double current_speed_ms;
// - int current_rpm;
// - int current_gear;
// - Point3D current_world_position;
// It also assumes a lerp() function is available.

void TelemetryPlayer::update(double total_elapsed_time_s) {
    // PRE-CONDITION: We must have data to work with
    if (lap_data.size() < 2) {
        return;
    }

    // STEP 1: TIME SYNCHRONIZATION
    // Find the total duration of the entire recording from the data itself.
    const double total_recording_duration_ms = lap_data.back().timestamp_ms;
    const double total_recording_duration_s = total_recording_duration_ms / 1000.0;

    // If the data is invalid, do nothing.
    if (total_recording_duration_s <= 0.0) {
        return;
    }

    // Determine where the master game clock is within a single lap recording.
    // fmod() makes the time loop automatically.
    // Example: if game time is 150s and lap is 144s, this gives 6s.
    lap_timer_ = fmod(total_elapsed_time_s, total_recording_duration_s);
    int target_time_ms = static_cast<int>(lap_timer_ * 1000.0);

    // Update lap count for UI display
    lap_count_ = static_cast<int>(total_elapsed_time_s / total_recording_duration_s);
    if (lap_count_ > 0) {
        last_lap_time_ = total_recording_duration_s;
    }


    // STEP 2: FIND THE CORRECT DATA SEGMENT
    // If the time has looped (target_time_ms is smaller than our current spot),
    // we must reset our search from the beginning of the data.
    if (target_time_ms < lap_data[current_index].timestamp_ms) {
        current_index = 0;
    }

    // This is the core search loop. It "fast-forwards" through the data
    // until the *next* data point is in the future.
    while (current_index < lap_data.size() - 1 &&
        lap_data[current_index + 1].timestamp_ms <= target_time_ms)
    {
        current_index++;
    }


    // STEP 3: CALCULATE AND INTERPOLATE THE CURRENT STATE
    // We now have the two points in time that our target_time_ms is between.
    const auto& prev_point = lap_data[current_index];
    // If we're at the end of the data, the "next" point is the start of the data, to make the loop seamless.
    const auto& next_point = (current_index == lap_data.size() - 1) ? lap_data[0] : lap_data[current_index + 1];

    int segment_start_ms = prev_point.timestamp_ms;
    int segment_end_ms = next_point.timestamp_ms;

    // Handle the final segment of the lap where time wraps from max back to 0.
    if (segment_end_ms < segment_start_ms) {
        segment_end_ms = static_cast<int>(total_recording_duration_ms);
    }

    int segment_duration_ms = segment_end_ms - segment_start_ms;

    if (segment_duration_ms <= 0) {
        // This condition is met when the car is stationary (e.g., at the start).
        // The time between two data points is zero. We CANNOT interpolate.
        // The car's state is simply the state of the last data point.
        current_speed_ms = prev_point.speed_ms;
        current_rpm = prev_point.rpm;
        current_gear = prev_point.gear;
        current_world_position = prev_point.position;
    }
    else {
        // The car is moving between two points in time.
        // Calculate 't' (a value from 0.0 to 1.0) of how far we are through this segment.
        double t = static_cast<double>(target_time_ms - segment_start_ms) / segment_duration_ms;
        t = std::max(0.0, std::min(1.0, t)); // Clamp t to the [0, 1] range to be safe

        // Use linear interpolation to calculate a smooth state.
        current_speed_ms = lerp(prev_point.speed_ms, next_point.speed_ms, t);
        current_rpm = lerp(prev_point.rpm, next_point.rpm, t);
        current_world_position = lerp(prev_point.position, next_point.position, t);
        current_gear = prev_point.gear; // Gear changes are instant, so we don't interpolate.
    }
}

TelemetryPlayer::TelemetryPlayer(const std::wstring path) {

    try {
        // Call the function to load the data from the file.
        lap_data = loadTelemetryDataFromFile(path);
  
    }
    catch (const std::runtime_error& e) {
        // Catches file opening errors
        std::cerr << e.what() << std::endl;
        return; // Exit with an error code
    }

    current_index = 0;
  
    // Initialize the car's state to be exactly at the new start/finish line.
    const auto& start_point = lap_data[current_index];
    current_speed_ms = start_point.speed_ms;
    current_gear = start_point.gear;
    current_rpm = start_point.rpm;
    current_world_position = start_point.position;

    // Initialize lap timing
    lap_count_ = 1;      // Start on Lap 1
    lap_timer_ = 0.0;    // Lap timer starts at zero
    last_lap_time_ = 0.0;
}

double TelemetryPlayer::getLapDuration() const {
    if (lap_data.empty()) return 0.0;
    return static_cast<double>(lap_data.back().timestamp_ms) / 1000.0;
}


