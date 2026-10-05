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
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;

namespace TelemetryAutomation
{
    /// <summary>
    /// Represents a point in 3D space (z=forward, y=up, x=side-to-side).
    /// </summary>
    public struct Point3D
    {
        public double X;
        public double Y;
        public double Z;

        public Point3D(double x, double y, double z)
        {
            X = x;
            Y = y;
            Z = z;
        }
    }

    /// <summary>
    /// Holds a single, complete snapshot of telemetry data.
    /// Speed is stored in meters per second (m/s).
    /// </summary>
    public struct TelemetryPoint
    {
        public int TimestampMs;
        public double SpeedMs;
        public int Gear;
        public int Rpm;
        public Point3D Position;
    }

    public class TelemetryPlayer
    {
        // Telemetry Data & State
        private readonly List<TelemetryPoint> _lapData;
        private int _currentIndex;

        // Current State Variables
        private double _currentSpeedMs;
        private int _currentGear;
        private int _currentRpm;
        private Point3D _currentWorldPosition;

        // Lap Timing Members
        private int _lapCount;
        private double _lapTimer;
        private double _lastLapTime;

        #region Public Properties (Getters)

        public double SpeedMps => _currentSpeedMs;
        public double SpeedKph => _currentSpeedMs * 3.6;
        public int Gear => _currentGear;
        public int Rpm => _currentRpm;
        public double LapDuration => _lapData.Any() ? _lapData.Last().TimestampMs / 1000.0 : 0.0;

        // World Position Properties
        public double WorldX => _currentWorldPosition.X;
        public double WorldY => _currentWorldPosition.Y;
        public double WorldZ => _currentWorldPosition.Z;
        public Point3D WorldPosition => _currentWorldPosition;

        // Lap Timing Properties
        public int LapCount => _lapCount;
        public double CurrentLapTime => _lapTimer;
        public double LastLapTime => _lastLapTime;

        #endregion

        /// <summary>
        /// Constructor: Initializes the car and loads its telemetry data from a file.
        /// </summary>
        public TelemetryPlayer(string telemetryFilePath)
        {
            try
            {
                _lapData = LoadTelemetryDataFromFile(telemetryFilePath);
                if (!_lapData.Any())
                {
                    // Handle case where file is valid but contains no data
                    throw new InvalidDataException("Telemetry file is empty or contains no valid data.");
                }
            }
            catch (Exception ex)
            {
                // Catches file opening, parsing, or empty data errors
                Console.Error.WriteLine($"Error initializing TelemetryPlayer: {ex.Message}");
                // Initialize with an empty list to prevent null reference exceptions
                _lapData = new List<TelemetryPoint>();
                return;
            }

            _currentIndex = 0;

            // Initialize the car's state to be exactly at the start/finish line.
            var startPoint = _lapData[0];
            _currentSpeedMs = startPoint.SpeedMs;
            _currentGear = startPoint.Gear;
            _currentRpm = startPoint.Rpm;
            _currentWorldPosition = startPoint.Position;

            // Initialize lap timing
            _lapCount = 1;      // Start on Lap 1
            _lapTimer = 0.0;    // Lap timer starts at zero
            _lastLapTime = 0.0;
        }

        /// <summary>
        /// Updates the car's state based on the total elapsed time of the simulation.
        /// </summary>
        /// <param name="totalElapsedTimeS">Total simulation time in seconds.</param>
        public void Update(double totalElapsedTimeS)
        {
            // PRE-CONDITION: We must have data to work with
            if (_lapData.Count < 2)
            {
                return;
            }

            // STEP 1: TIME SYNCHRONIZATION
            double totalRecordingDurationMs = _lapData.Last().TimestampMs;
            double totalRecordingDurationS = totalRecordingDurationMs / 1000.0;

            if (totalRecordingDurationS <= 0.0)
            {
                return;
            }

            // Determine where the master game clock is within a single lap recording using the modulo operator.
            _lapTimer = totalElapsedTimeS % totalRecordingDurationS;
            int targetTimeMs = (int)(_lapTimer * 1000.0);

            // Update lap count for UI display
            _lapCount = (int)(totalElapsedTimeS / totalRecordingDurationS) + 1;
            if (_lapCount > 1)
            {
                _lastLapTime = totalRecordingDurationS;
            }

            // STEP 2: FIND THE CORRECT DATA SEGMENT
            if (targetTimeMs < _lapData[_currentIndex].TimestampMs)
            {
                _currentIndex = 0; // Time has looped, reset search from the beginning.
            }

            // "Fast-forward" through data until the *next* point is in the future.
            while (_currentIndex < _lapData.Count - 1 &&
                   _lapData[_currentIndex + 1].TimestampMs <= targetTimeMs)
            {
                _currentIndex++;
            }

            // STEP 3: CALCULATE AND INTERPOLATE THE CURRENT STATE
            var prevPoint = _lapData[_currentIndex];
            // If at the end, the "next" point is the start to make the loop seamless.
            var nextPoint = (_currentIndex == _lapData.Count - 1) ? _lapData[0] : _lapData[_currentIndex + 1];

            int segmentStartMs = prevPoint.TimestampMs;
            int segmentEndMs = nextPoint.TimestampMs;

            // Handle the final segment where time wraps from max back to 0.
            if (segmentEndMs < segmentStartMs)
            {
                segmentEndMs = (int)totalRecordingDurationMs;
            }

            int segmentDurationMs = segmentEndMs - segmentStartMs;

            if (segmentDurationMs <= 0)
            {
                // Car is stationary, no interpolation possible. Set state to the previous point.
                _currentSpeedMs = prevPoint.SpeedMs;
                _currentRpm = prevPoint.Rpm;
                _currentGear = prevPoint.Gear;
                _currentWorldPosition = prevPoint.Position;
            }
            else
            {
                // Car is moving. Calculate 't' (0.0 to 1.0) for interpolation.
                double t = (double)(targetTimeMs - segmentStartMs) / segmentDurationMs;
                t = Math.Max(0.0, Math.Min(1.0, t)); // Ensure t is in the [0, 1] range.

                // Use linear interpolation for a smooth state.
                _currentSpeedMs = Lerp(prevPoint.SpeedMs, nextPoint.SpeedMs, t);
                _currentRpm = (int)Lerp(prevPoint.Rpm, nextPoint.Rpm, t);
                _currentWorldPosition = Lerp(prevPoint.Position, nextPoint.Position, t);
                _currentGear = prevPoint.Gear; // Gear changes are instant.
            }
        }

        /// <summary>
        /// Loads and parses telemetry data from a CSV-like file.
        /// </summary>
        /// <param name="filePath">The path to the telemetry file.</param>
        /// <returns>A list of TelemetryPoint objects.</returns>
        /// <exception cref="FileNotFoundException">Thrown if the file does not exist.</exception>
        /// <exception cref="Exception">Thrown on parsing errors.</exception>
        public static List<TelemetryPoint> LoadTelemetryDataFromFile(string filePath)
        {
            if (!File.Exists(filePath))
            {
                throw new FileNotFoundException($"Error: Could not find file '{filePath}'");
            }

            var telemetryData = new List<TelemetryPoint>();
            int lineNumber = 0;

            // 'using' ensures the StreamReader is properly disposed of
            using (var reader = new StreamReader(filePath))
            {
                string line;
                while ((line = reader.ReadLine()) != null)
                {
                    lineNumber++;
                    if (string.IsNullOrWhiteSpace(line))
                    {
                        continue;
                    }

                    string[] parts = line.Split(',');
                    if (parts.Length != 7)
                    {
                        Console.Error.WriteLine($"Warning: Could not parse line {lineNumber}. Expected 7 values, found {parts.Length}. Line: '{line}'");
                        continue;
                    }

                    try
                    {
                        var point = new TelemetryPoint
                        {
                            TimestampMs = int.Parse(parts[0]),
                            SpeedMs = double.Parse(parts[1], CultureInfo.InvariantCulture),
                            Gear = (int)Math.Round(double.Parse(parts[2], CultureInfo.InvariantCulture)),
                            Rpm = (int)Math.Round(double.Parse(parts[3], CultureInfo.InvariantCulture)),
                            Position = new Point3D(
                                double.Parse(parts[4], CultureInfo.InvariantCulture),
                                double.Parse(parts[5], CultureInfo.InvariantCulture),
                                double.Parse(parts[6], CultureInfo.InvariantCulture)
                            )
                        };
                        telemetryData.Add(point);
                    }
                    catch (FormatException ex)
                    {
                        Console.Error.WriteLine($"Warning: Format error on line {lineNumber} in file '{filePath}': {ex.Message}. Line: '{line}'");
                    }
                }
            }

            return telemetryData;
        }

        #region Private Helper Methods

        /// <summary>
        /// Helper for linear interpolation of scalar values.
        /// </summary>
        private static double Lerp(double a, double b, double t)
        {
            return a + (b - a) * t;
        }

        /// <summary>
        /// Overloaded helper to interpolate a 3D point.
        /// </summary>
        private static Point3D Lerp(Point3D a, Point3D b, double t)
        {
            return new Point3D(
                Lerp(a.X, b.X, t),
                Lerp(a.Y, b.Y, t),
                Lerp(a.Z, b.Z, t)
            );
        }

        #endregion
    }
}