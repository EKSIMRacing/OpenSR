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

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <chrono>
#include <random>
#include <iomanip>
#include <utility> // For std::pair

// ANSI color codes for terminal output
const std::string RESET_COLOR = "\033[0m";
const std::string RED_COLOR = "\033[91m";
const std::string YELLOW_COLOR = "\033[93m";
const std::string GREEN_COLOR = "\033[92m";
const std::string BLUE_COLOR = "\033[94m";

// Helper function for cross-platform screen clearing
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

class RacingCar {
public:
    // Represents the car's current driving state
    enum class CarState { ACCELERATING, BRAKING, CRUISING };

    // Represents a section of the race track
    struct TrackSegment {
        std::string name;
        double length_m;
        CarState target_state;
        double target_speed_mps; // Speed to aim for in this segment
    };

    RacingCar() : gen(rd()) { // Constructor
        buildMonzaTrack(); // Initialize the track layout
        current_segment_index = 0;
        distance_on_lap = 0.0;
        gear = 1;
        speed_mps = 0.0; // Start from a standstill
        rpm = IDLE_RPM;
        state = CarState::ACCELERATING; // Start by accelerating from the line
    }

    // The main update function, now driven by track position
    void update(double dt) {
        // 1. Update Position and Track Segment
        double distance_this_frame = speed_mps * dt;
        distance_on_lap += distance_this_frame;

        // Check if we've completed a lap
        if (distance_on_lap >= total_lap_distance) {
            distance_on_lap -= total_lap_distance;
            current_segment_index = 0;
        }

        // Determine the cumulative length of the track up to the current segment
        double cumulative_distance = 0.0;
        for (int i = 0; i <= current_segment_index; ++i) {
            cumulative_distance += track_layout[i].length_m;
        }

        // If we've passed the end of the current segment, move to the next one
        if (distance_on_lap > cumulative_distance) {
            current_segment_index = (current_segment_index + 1) % track_layout.size();
        }

        // 2. Update Car State based on the current segment
        const auto& current_segment = track_layout[current_segment_index];
        state = current_segment.target_state;

        // 3. Update Speed based on State and Target Speed
        double target_speed = current_segment.target_speed_mps;

        if (state == CarState::ACCELERATING) {
            // Accelerate if below target speed for this segment
            if (speed_mps < target_speed) {
                // Stronger acceleration in lower gears
                double acceleration_factor = (8.0 - gear) * 1.6;
                speed_mps += acceleration_factor * dt;
            }
            else {
                // Maintain speed with slight fluctuations if target is reached
                std::uniform_real_distribution<> cruise_fluctuation(-0.5, 0.5);
                speed_mps += cruise_fluctuation(gen) * dt;
            }
        }
        else if (state == CarState::BRAKING) {
            // Find the target speed of the *next* important segment (usually a corner)
            int next_corner_idx = (current_segment_index + 1) % track_layout.size();
            double next_target_speed = track_layout[next_corner_idx].target_speed_mps;

            // Brake hard if we are faster than the speed needed for the upcoming corner
            if (speed_mps > next_target_speed) {
                double deceleration = 18.0 * dt;
                speed_mps -= deceleration;
            }
        }
        else if (state == CarState::CRUISING) {
            // Try to hold the target speed for corners like Lesmos or Ascari
            if (speed_mps > target_speed) {
                speed_mps -= 4.0 * dt; // Gentle lift off throttle
            }
            else {
                speed_mps += 4.0 * dt; // Gentle throttle application
            }
        }

        // Clamp speed to its valid range [0, MAX_SPEED_MPS]
        speed_mps = std::max(0.0, std::min(speed_mps, MAX_SPEED_MPS));

        // 4. Update Gear and RPM based on new speed
        updateGear();
        updateRpm();
    }

    // Public Getters for display
    int getGear() const { return gear; }
    double getSpeedMps() const { return speed_mps; }
    double getSpeedKph() const { return speed_mps * 3.6; }
    double getRpm() const { return rpm; }
    std::string getStateString() const {
        switch (state) {
        case CarState::ACCELERATING: return "ACCELERATING";
        case CarState::BRAKING:      return "BRAKING";
        case CarState::CRUISING:     return "MAINTAINING SPEED";
        default:                     return "UNKNOWN";
        }
    }
    std::string getCurrentSegmentName() const { return track_layout[current_segment_index].name; }
    double getLapDistance() const { return distance_on_lap; }
    double getTotalLapDistance() const { return total_lap_distance; }


    // Constants (can be public for external access like in display)
public:
    static constexpr double MAX_RPM = 8200.0;
    static constexpr double MAX_SPEED_MPS = 295.0 * 1000.0 / 3600.0;
    static constexpr int MAX_GEAR = 6;

private:
    // Constants
    static constexpr double IDLE_RPM = 900.0;
    static constexpr double RPM_SHIFT_UP = 7800.0;

    const std::map<int, std::pair<double, double>> GEAR_SPEED_RANGES = {
        {1, {0.0, 20.0}},   //  0-72 km/h
        {2, {15.0, 40.0}},  // 54-144 km/h
        {3, {35.0, 60.0}},  // 126-216 km/h
        {4, {55.0, 75.0}},  // 198-270 km/h
        {5, {70.0, 85.0}},  // 252-306 km/h
        {6, {80.0, MAX_SPEED_MPS + 5.0}}
    };

    // Member Variables
    int gear;
    double speed_mps;
    double rpm;
    CarState state;

    // Track-related members
    std::vector<TrackSegment> track_layout;
    int current_segment_index;
    double distance_on_lap;
    double total_lap_distance = 0.0;

    // Randomness Engine
    std::random_device rd;
    std::mt19937 gen;

    // Private Helper Methods
    void buildMonzaTrack() {
        // Distances are approximate for good simulation flow
        track_layout = {
            {"Rettifilo Tribune", 1150, CarState::ACCELERATING, MAX_SPEED_MPS},
            {"Rettifilo Braking", 120,  CarState::BRAKING,      18.0}, // Braking FOR the chicane
            {"Variante Rettifilo",150,  CarState::ACCELERATING, 35.0}, // Accelerate out of chicane
            {"Curva Grande",      730,  CarState::ACCELERATING, 75.0}, // Flat out corner
            {"Roggia Braking",    100,  CarState::BRAKING,      25.0}, // Braking for 2nd chicane
            {"Variante Roggia",   150,  CarState::ACCELERATING, 45.0},
            {"Lesmo 1",           270,  CarState::CRUISING,     48.0}, // Maintain speed through corner
            {"Lesmo 2",           280,  CarState::CRUISING,     45.0},
            {"Serraglio Straight",950,  CarState::ACCELERATING, MAX_SPEED_MPS},
            {"Ascari Braking",    90,   CarState::BRAKING,      38.0},
            {"Variante Ascari",   450,  CarState::CRUISING,     60.0}, // Fast, flowing chicane
            {"Parabolica Approach",400, CarState::ACCELERATING, MAX_SPEED_MPS},
            {"Parabolica Corner", 753,  CarState::ACCELERATING, 65.0} // Long accelerating corner exit
        };
        // Calculate total lap distance once
        for (const auto& segment : track_layout) {
            total_lap_distance += segment.length_m;
        }
    }

    void updateGear() {
        auto [current_min, current_max] = GEAR_SPEED_RANGES.at(gear);
        if (speed_mps > current_max && gear < MAX_GEAR) {
            gear++;
        }
        else if (speed_mps < current_min && gear > 1) {
            gear--;
        }
    }

    void updateRpm() {
        if (speed_mps <= 1.0) {
            rpm = IDLE_RPM;
            return;
        }
        auto [min_speed, max_speed] = GEAR_SPEED_RANGES.at(gear);
        double speed_in_gear_range = speed_mps - min_speed;
        double total_range = max_speed - min_speed;
        double percentage_in_gear = (total_range > 0) ? (speed_in_gear_range / total_range) : 1.0;
        double rpm_range = RPM_SHIFT_UP - (IDLE_RPM + 1000.0);
        rpm = (IDLE_RPM + 1000.0) + (percentage_in_gear * rpm_range);
        std::uniform_real_distribution<> jitter_dist(-100.0, 100.0);
        rpm += jitter_dist(gen);
        rpm = std::max(IDLE_RPM, std::min(rpm, MAX_RPM));
    }


void displayDashboard(const RacingCar& car) {
    clearScreen();

    // RPM Bar
    int rpm_bar_length = 30;
    double rpm_percentage = car.getRpm() / RacingCar::MAX_RPM;
    int rpm_filled = static_cast<int>(rpm_bar_length * rpm_percentage);
    std::string rpm_bar_color = GREEN_COLOR;
    if (car.getRpm() > 7000) rpm_bar_color = RED_COLOR;
    else if (car.getRpm() > 5000) rpm_bar_color = YELLOW_COLOR;

    // Speed Bar
    int speed_bar_length = 30;
    double speed_percentage = car.getSpeedMps() / car.MAX_SPEED_MPS;
    int speed_filled = static_cast<int>(speed_bar_length * speed_percentage);

    // Printing
    std::cout << "--- RACING SIMULATOR (C++) ---\n";
    std::cout << "STATE     : " << std::left << std::setw(15) << car.getStateString() << "\n";
    std::cout << "------------------------------\n";
    std::cout << "GEAR      : [" << car.getGear() << "]\n";
    std::cout << "SPEED     : " << std::fixed << std::setprecision(1) << std::setw(6)
        << car.getSpeedKph() << " km/h | " << std::setw(5) << car.getSpeedMps() << " m/s\n";
    std::cout << "            " << BLUE_COLOR;
    for (int i = 0; i < speed_filled; ++i) std::cout << "█";
    std::cout << RESET_COLOR;
    for (int i = 0; i < speed_bar_length - speed_filled; ++i) std::cout << "-";
    std::cout << "\n";

    std::cout << "RPM       : " << std::fixed << std::setprecision(0) << std::setw(7) << car.getRpm() << " RPM\n";
    std::cout << "            " << rpm_bar_color;
    for (int i = 0; i < rpm_filled; ++i) std::cout << "█";
    std::cout << RESET_COLOR;
    for (int i = 0; i < rpm_bar_length - rpm_filled; ++i) std::cout << "-";
    std::cout << "\n";

    std::cout << "------------------------------\n";
    std::cout << "(Press Ctrl+C to exit)\n" << std::flush;
}

};