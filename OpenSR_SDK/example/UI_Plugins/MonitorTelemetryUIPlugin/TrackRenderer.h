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
#ifdef NOMINMAX
#undef NOMINMAX
#endif
#include <Windows.h>
#include <gdiplus.h> // Ensure this is included at the top of your .cpp file
#include <string>
#include <vector>
#include <cmath>
#include <iomanip>
#include <map>
#include <fstream>
#include <iostream>
#include <sstream>      // For parsing strings (std::stringstream)
#include <stdexcept> 

#pragma comment (lib,"Gdiplus.lib") // Alternative way to link the library

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

// Helper function to perform Catmull-Rom spline interpolation for a single coordinate.
// It calculates a point on a curve defined by p0, p1, p2, p3 at interval t (0.0 to 1.0).
double CatmullRom(double p0, double p1, double p2, double p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    return 0.5 * ((2.0 * p1) +
        (-p0 + p2) * t +
        (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * t2 +
        (-p0 + 3.0 * p1 - 3.0 * p2 + p3) * t3);
}

// A self-contained class to handle all track rendering logic using GDI+
class TrackRenderer {
public:

    TrackRenderer() {}

    bool init(const wchar_t* path = nullptr, const wchar_t* filename = nullptr){
        // 1. Ensure the 'Map' directory exists.
  

        std::vector<Point3D> control_points;

        // 2. Build the full file path.
       
        if (filename && path)
        {
            // ensure dir is present
            _wmkdir(path);
            // Use provided filepath
            std::wstring fullpath(path);
            fullpath += filename;
            control_points = loadControlPointsFromFile(fullpath);
        }
        else
        {
   
            control_points.push_back({ 0,0,0 });
        }

        // The control_points vector is already high - resolution, so we use it directly
        // without any further smoothing.
        smoothed_track_path_ = control_points;

        // Ensure the path is visually closed by adding the first point to the end.
        if (!smoothed_track_path_.empty()) {
            smoothed_track_path_.push_back(smoothed_track_path_.front());
        }

        // Pre-calculate bounding box
        if (!smoothed_track_path_.empty()) {
            min_x_ = max_x_ = smoothed_track_path_[0].x;
            min_z_ = max_z_ = smoothed_track_path_[0].z;
            for (const auto& p : smoothed_track_path_) {
                min_x_ = (std::min)(min_x_, p.x); max_x_ = (std::max)(max_x_, p.x);
                min_z_ = (std::min)(min_z_, p.z); max_z_ = (std::max)(max_z_, p.z);
            }
        }

        return control_points.size();
    }

    /**
 * @brief Loads 3D control points from a comma-delimited file.
 *
 * Each line in the file is expected to be in the format: "x,y,z"
 *
 * @param filePath The path to the input file (e.g., "Map/spa.trk").
 * @return A vector of Point3D objects.
 * @throws std::runtime_error if the file cannot be opened.
 */
    std::vector<Point3D> loadControlPointsFromFile(const std::wstring& filePath) {

        std::vector<Point3D> points;

        // 1. Open the file
        std::ifstream file(filePath);

        // 2. Check if the file was successfully opened
        if (!file.is_open()) {
            // If not, throw an exception with a descriptive error message
            std::wcerr << L"Error: Could not open file '" << filePath << L"'" << std::endl;
            points.push_back({ 0, 0, 0 });
            return points;
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
                //point.gear = static_cast<int>(std::round(temp_gear));
                //point.rpm = static_cast<int>(std::round(temp_rpm));
              
                points.push_back({ point.position.x, point.position.y, point.position.z });
            
            }
            else {
                // If the line is malformed, print a warning and continue
                std::wcerr << L"Warning: Could not parse line " << lineNumber
                    << L" in file '" << filePath << L"': " << line.c_str() << std::endl;
            }
        }

        // The file is automatically closed when 'file' goes out of scope.

        return points;
    }

    float fixed_scale_ = 0.0f;
    void Draw(HDC hdc, const RECT& clientRect, const Point3D& carPosition) {
        if (smoothed_track_path_.empty())
            return;

        Gdiplus::Graphics graphics(hdc);
        graphics.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);

        const double worldWidth = max_x_ - min_x_;
        const double worldHeight = max_z_ - min_z_;

        if (worldWidth <= 0 || worldHeight <= 0) {
            return;
        }

        // Start: One-Time Scale Calculation Logic

        // 1. If scale has not been calculated yet (it's our first time drawing)...
        if (fixed_scale_ == 0.0f) {
            // ...calculate the scale factor to make the drawing fit the CURRENT clientRect.
            const float screenWidth = static_cast<float>(clientRect.right - clientRect.left);
            const float screenHeight = static_cast<float>(clientRect.bottom - clientRect.top);

            // This is the "fit with aspect ratio" calculation
            fixed_scale_ = std::min<>(
                screenWidth / static_cast<float>(worldWidth),
                screenHeight / static_cast<float>(worldHeight)
            );
        }
        // On every subsequent call, this 'if' block is skipped, and fixed_scale_ retains its original value.

        // End: One-Time Scale Calculation Logic

        // 2. Calculate the drawing's dimensions using the UNCHANGING fixed_scale_.
        const float scaledDrawingWidth = static_cast<float>(worldWidth) * fixed_scale_;
        const float scaledDrawingHeight = static_cast<float>(worldHeight) * fixed_scale_;

        // 3. Calculate the origin point to CENTER the fixed-size drawing within the CURRENT clientRect.
        const float originX = clientRect.left + ((clientRect.right - clientRect.left) - scaledDrawingWidth) / 2.0f;
        const float originY = clientRect.top + ((clientRect.bottom - clientRect.top) - scaledDrawingHeight) / 2.0f;

        // Use a clipping region to ensure drawing never spills outside the clientRect boundary.
        Gdiplus::Rect clipRect(clientRect.left, clientRect.top, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
        graphics.SetClip(clipRect);

        Gdiplus::Pen blackPen(Gdiplus::Color(255, 0, 0, 0), 4.0f);
        Gdiplus::SolidBrush redBrush(Gdiplus::Color(255, 255, 0, 0));

        std::vector<Gdiplus::PointF> screenPoints;
        screenPoints.reserve(smoothed_track_path_.size());
        for (const auto& worldPoint : smoothed_track_path_) {
            float relativeX = static_cast<float>(worldPoint.x - min_x_);
            float relativeZ = static_cast<float>(worldPoint.z - min_z_);

            float screenX = originX + (relativeX * fixed_scale_);
            float screenY = originY + (relativeZ * fixed_scale_);
            screenPoints.emplace_back(screenX, screenY);
        }

        if (screenPoints.size() > 1) {
            graphics.DrawLines(&blackPen, screenPoints.data(), (INT)screenPoints.size());
        }

        float carRelativeX = static_cast<float>(carPosition.x - min_x_);
        float carRelativeZ = static_cast<float>(carPosition.z - min_z_);
        Gdiplus::PointF carScreenPos(
            originX + (carRelativeX * fixed_scale_),
            originY + (carRelativeZ * fixed_scale_)
        );


        float carRadius = 7.0f;
        Gdiplus::RectF carRect(carScreenPos.X - carRadius, carScreenPos.Y - carRadius, carRadius * 2, carRadius * 2);
        graphics.FillEllipse(&redBrush, carRect);

        graphics.ResetClip();
    }
  
    std::wstring m_fullPath;

private:
    std::vector<Point3D> smoothed_track_path_;
    double min_x_ = 0, max_x_ = 0, min_z_ = 0, max_z_ = 0;

    // WorldToScreen function
    Gdiplus::PointF WorldToScreen(const Point3D& worldPos, const RECT& clientRect, double worldW, double worldH) {
        double paddedMinX = min_x_ - worldW * 0.05;
        double paddedMaxX = max_x_ + worldW * 0.05;
        double paddedMinZ = min_z_ - worldH * 0.05;
        double paddedWorldW = paddedMaxX - paddedMinX;
        double paddedWorldH = worldH * 1.1;
        int screenW = clientRect.right - clientRect.left;
        int screenH = clientRect.bottom - clientRect.top;
        double scale = (std::min)(screenW / paddedWorldW, screenH / paddedWorldH);
        double effectiveW = paddedWorldW * scale;
        double effectiveH = paddedWorldH * scale;
        double xOffset = (screenW - effectiveW) / 2.0;
        double zOffset = (screenH - effectiveH) / 2.0;
        float screenX = (float)(xOffset + ((paddedMaxX - worldPos.x) * scale));
        float screenY = (float)(zOffset + (screenH - ((worldPos.z - paddedMinZ) * scale)));
        return Gdiplus::PointF(screenX, screenY);
    }
};