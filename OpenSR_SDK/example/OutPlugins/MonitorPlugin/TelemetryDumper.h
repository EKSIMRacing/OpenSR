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
// telemetry_dumper.h

#ifndef TELEMETRY_DUMPER_H
#define TELEMETRY_DUMPER_H

#include <iostream>
#include <fstream>
#include <string>
#include <iomanip> // For std::fixed, std::setprecision



/**
 * @class TelemetryDumper
 * @brief Manages the real-time streaming of telemetry data to a C++ header file.
 *
 * Create an instance of this class at the start of your data recording session.
 * Call dumpPoint() for each new piece of telemetry.
 * When the object goes out of scope (e.g., at the end of a function), its
 * destructor will automatically write the file footer and close it properly.
 */
class TelemetryDumper {
public:
  
    /**
     * @brief Constructor that opens the output file and writes the C++ header boilerplate.
     * @param filename The hardcoded path to the output file (e.g., "lap_data_dump...").
     * @param array_name The variable name for the array in the generated file (e.g., "lap_data").
     */
    TelemetryDumper() {}

    void Init(const wchar_t* path = nullptr, const wchar_t* filename = nullptr) {
        // 1. Ensure the 'Map' directory exists.

     
        // 2. Build the full file path.
        std::wstring fullpath;
        if (filename && path)
        {   
            // ensure dir is present
            _wmkdir(path);

            // Use provided filepath
            fullpath = path;
            fullpath += filename;
        }
        else
        {
            // ensure dir is present
            _wmkdir(L"\\Map");

            fullpath = L"Map\\lap_dump.lap"; // Use default filename
        }

        m_dumpReady = false;
        outfile_.open(fullpath);

        if (!outfile_.is_open()) {
            std::cerr << "FATAL ERROR: Could not open file for writing: " << filename << std::endl;
            return;
        }

        // Set floating-point precision for consistent formatting
        outfile_ << std::fixed << std::setprecision(2);

        //// Write the standard C++ header file boilerplate
        //outfile_ << "// THIS IS AN AUTO-GENERATED FILE. DO NOT EDIT MANUALLY.\n\n";
        //outfile_ << "#ifndef LAP_DATA_DUMP_H\n";
        //outfile_ << "#define LAP_DATA_DUMP_H\n\n";
        //outfile_ << "#include <vector>\n\n";

        //// Write the required struct definitions
        //outfile_ << "struct Point3D { double x, y, z; };\n";
        //outfile_ << "struct TelemetryPoint { int timestamp_ms; double speed_ms; int gear; int rpm; Point3D position; };\n\n";

        //// Start the array definition
        //outfile_ << "const std::vector<TelemetryPoint> " << array_name << " = {\n";
        m_dumpReady = true;
        std::cout << "TelemetryDumper initialized. Writing to " << filename << "..." << std::endl;
    }

    /**
     * @brief Writes a single line of telemetry data to the file.
     * This is the function you call repeatedly from your game loop.
     */
    void dumpPoint(int timestamp_ms, double speed_ms, int gear, int rpm, const Point3D& position) {
        if (!m_dumpReady || !outfile_.is_open()) {
            return; // Don't try to write if the file isn't open
        }

        // Add a comma and newline before this entry IF it's not the very first one.
        if (!is_first_entry_) {
            outfile_ << ",\n";
        }

        // Write the data in the exact format: {ts, speed, gear, rpm, {x, y, z}}
        outfile_ 
            << timestamp_ms << ","
            << speed_ms << ","
            << gear << ","
            << rpm << ","
            << position.x << "," << position.y << "," << position.z;

        is_first_entry_ = false; // Subsequent entries are not the first
    }

    void Stop() {
        m_dumpReady = false;
        if (outfile_.is_open()) {
            // Write the end of the array and the file
           /* outfile_ << "\n};\n\n";
            outfile_ << "#endif // LAP_DATA_DUMP_H\n";*/
            outfile_.close();
            std::cout << "TelemetryDumper finalized." << std::endl;
        }
    }
    /**
     * @brief Destructor that finalizes the C++ file by writing the closing
     *        brackets and include guards, then closes the file stream.
     */
    ~TelemetryDumper() {
        Stop();
        m_dumpReady = false;
    }
    bool m_dumpReady = false;
private:
    std::ofstream outfile_;
    bool is_first_entry_ = true; // State to correctly handle commas between lines

};

#endif // TELEMETRY_DUMPER_H

