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

// =====================================================================================
// GameProfileHelper.h
// =====================================================================================
//
// OpenSR SDK
// Standalone helper class used to parse OpenSR game profile XML files.
//
//
// Goal:
// -----
// Allow ANY plugin developer to easily load and parse an OpenSR profile if needed:
//
//     GameProfileHelper profile;
//
//     if (profile.LoadProfile(L"mygame.xml"))
//     {
//         auto& cfg = profile.GetConfig();
//
//         if (cfg.shiftLights.allowed)
//         {
//         }
//     }
//
// =====================================================================================

#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <filesystem>

#include "tinyxml2.h"

class GameProfileHelper
{
public:

	// =================================================================================
	// DISPLAY FUNCTIONS
	// =================================================================================
	//
	// Used by:
	// - dashboard displays
	// - quick info screens
	// - rotary switch functions
	//
	// XML Example:
	//
	// <option name="left display function">speed</option>
	//
	// =================================================================================

	enum DisplayFuncIndex {
		none = 100,
		speed = 101,
		lap = 102,
		position = 103,
		fuel = 104,
		sector = 105,
		gear = 106,
		rpm = 107,
		laps_remaining = 108,
		brake_bias = 109,
		total_laps = 110,
		system_date = 111,
		system_time = 112,
		osp_text = 113,
		osp_factor = 114,
		brightness_text = 115,
		brightness = 116,
		session_time_remain = 117,
		antilock_brakes = 118,
		traction_control = 119,
		antiroll_bar_front = 120,
		antiroll_bar_rear = 121,
		left_weight_jacket = 122,
		right_weight_jacket = 123,
		front_wing = 124,
		rear_wing = 125,
		water_temp = 126,
		oil_temp = 127,
		rev_limit = 128,
		fuel_per_lap = 129,
		fuel_mix = 131,
		throttle_shape = 132,
		diff_entry = 133,
		diff_middle = 134,
		diff_exit = 135,
		distance = 136,
		track_size = 137,
		current_lap_time = 201,
		last_lap_time = 202,
		best_lap_time = 203,
		gap_vs_leader = 204,
		gap_vs_next = 205,
		gap_vs_behind = 206,
		delta_time_vs_best = 207,
		delta_time_vs_best_native = 208,
		slipro_fuel_speed = 301,
		slipro_position_speed = 302,
		slipro_lap_speed = 303,
		slipro_sector_speed = 304,
		slipro_lap_total_laps = 305,
		slipro_rpm_gear = 306,
		slipro_kg_fuel_speed = 307
	};

	// =================================================================================
	// SHIFT LIGHTS STYLE
	// =================================================================================

	enum ShiftLightsStyle {

		styleRally = 0,
		styleGt = 1,
		styleF1 = 2
	};

	// =================================================================================
	// SHIFT LIGHTS METHOD
	// =================================================================================
	//
	// Defines HOW RPM LEDs are filled.
	//
	// Example:
	// - Progressive
	// - Alternate
	// - F1 style
	// - Side to center
	//
	// =================================================================================

	enum ShiftLightsMethod {

		mthProgressive = 0,
		mthAlternate = 1,
		mthPercentage = 2,
		mthAbsolute = 3,
		mthSideToCenter = 4,
		mthKERSGreenAndAlternate = 5,
		mthReversKERSGreenAndAlternate = 6,
		mthSemiProgressiveF1Style = 7,
		mthPercentageModernF1DRS = 8,
		mthAbsoluteModernF1DRS = 9
	};

	// =================================================================================
	// OPTIMAL SHIFT POINTS
	// =================================================================================

	enum ShiftPointMethod {

		mtdDefault,
		mtdDefaultAndLastRPMLedBlinking,
		mtdDefaultBlinkingLastRPMNotBlinking,
		mtdLastRPMAloneBlinking,
		mtdLastRPMAloneNotBlinking
	};

	// =================================================================================
	// CASE INSENSITIVE MAP COMPARATOR
	// =================================================================================
	//
	// Used by:
	// std::map<std::string, int>
	//
	// Allows:
	// "LOWFUEL"
	// "lowfuel"
	// "LowFuel"
	//
	// to be treated identically.
	//
	// =================================================================================

	struct CaseInsensitiveCompare {

		bool operator()(const std::string& a, const std::string& b) const {

			return std::lexicographical_compare(
				a.begin(), a.end(),
				b.begin(), b.end(),
				[](unsigned char c1, unsigned char c2)
				{
					return std::tolower(c1) < std::tolower(c2);
				}
			);
		}
	};

	// =================================================================================
	// GENERAL SECTION
	// =================================================================================

	struct GeneralConfig {

		// Gear characters shown on display
		char reverseChar = 'r';
		char neutralChar = 'n';

		// Maximum forward gears supported
		int maxForwardGears = 6;

		// Unit system
		bool useMetric = true;

		// Speed display unit
		bool speedUnitMetric = true;

		// Delays
		int laptimeReportDelay = 8;
		int blinkDelay = 600;
	};

	// =================================================================================
	// DISPLAY SECTION
	// =================================================================================

	struct DisplayConfig {

		// Main display functions
		std::string leftDisplayFunc = "speed";
		std::string rightDisplayFunc = "current_lap_time";

		// Quick info rotary/switch functions
		int quickInfoLeftFunc = DisplayFuncIndex::position;
		int quickInfoRightFunc = DisplayFuncIndex::fuel;

		// Misc
		int ignitionMethod = 0;
		int clutchBitingPoint = 0;

		bool gearOverride = true;
	};

	// =================================================================================
	// SHIFT LIGHTS
	// =================================================================================

	struct ShiftLightsConfig {

		bool allowed = true;

		// Fill method
		int method = ShiftLightsMethod::mthPercentage;

		// Visual style
		int style = ShiftLightsStyle::styleGt;

		// Percentage thresholds
		//
		// Example:
		// 55,60,64,68,72
		//
		std::vector<float> rpmThresholdsPct;

		// Absolute RPM thresholds
		//
		// Example:
		// 12000,12500,13000
		//
		std::vector<int> rpmThresholdsAbs;

		// Disable LEDs on last gear
		bool blankWithLastGear = false;
	};

	// =================================================================================
	// PIT LIMITER
	// =================================================================================

	struct PitLimiterConfig {

		bool allowed = true;

		// Bitmask ON state
		int method = 0;

		// Bitmask OFF state
		int altMethod = 0;

		bool isBlinkAllowed = true;
		bool onRpmLedOnly = false;

		int blinkDelay = 100;
	};

	// =================================================================================
	// OPTIMAL SHIFT POINTS
	// =================================================================================

	struct ShiftPointsConfig {

		bool allowed = true;

		int method = ShiftPointMethod::mtdDefault;

		int ospFactor = 120;

		bool matchGearRatio = false;

		std::vector<float> factorsPerGear;

		bool allowedFirstGear = false;

		int blinkDelay = 40;

		char ospLChar = '[';
		char ospRChar = ']';

		bool isOspOnGearAllowed = true;
	};

	// =================================================================================
	// BRIGHTNESS
	// =================================================================================

	struct BrightnessConfig {

		int maxBrightness = 145;

		int globalBrightnessPct = 98;

		int stepValue = 10;
	};

	// =================================================================================
	// LED CONFIG
	// =================================================================================

	struct LedConfig {

		// RGB LED colors
		//
		// Key   = LED index
		// Value = RGB color
		//
		std::map<int, uint32_t> colors;

		// External/flag LEDs
		//
		// Example:
		// "lowfuel" -> 4
		//
		using ConfigMap = std::map<std::string, int, CaseInsensitiveCompare>;

		ConfigMap flagExternalEventMasks;
	};

	// =================================================================================
	// GLOBAL CONFIG
	// =================================================================================

	struct Config {

		GeneralConfig general;
		DisplayConfig display;
		ShiftLightsConfig shiftLights;
		PitLimiterConfig pitLimiter;
		ShiftPointsConfig shiftPoints;
		BrightnessConfig brightness;
		LedConfig led;
	};

public:

	GameProfileHelper() = default;
	virtual ~GameProfileHelper() = default;

	// =================================================================================
	// LOAD PROFILE
	// =================================================================================
	//
	// Load and parse an OpenSR XML profile.
	//
	// Returns:
	// true  = success
	// false = failed
	//
	// =================================================================================

	bool LoadProfile(const std::wstring& profilePath);

	// =================================================================================
	// GENERATE DEFAULT SHIFT LIGHT CURVE
	// =================================================================================
	//
	// Automatically generates:
	//
	// - percentage thresholds
	// - absolute thresholds
	//
	// Useful for:
	// - dynamic hardware
	// - profile generators
	// - plugins with custom LED counts
	//
	// =================================================================================

	void GenerateShiftLights(int numLeds, int maxRpm);

	// =================================================================================
	// GET CONFIG
	// =================================================================================

	const Config& GetConfig() const {
		return m_config;
	}

	// =================================================================================
	// INPUT MAPPINGS
	// =================================================================================
	//
	// Contains parsed hardware inputs.
	//
	// Signature -> InputIndex -> Name
	//
	// Example:
	//
	// "G27" -> 4 -> "btn pit limiter"
	//
	// =================================================================================

	std::unordered_map<
		std::string,
		std::unordered_map<int, std::string>
	> inputMappings;

protected:

	// =================================================================================
	// XML PARSERS
	// =================================================================================

	virtual void ParseGeneralSection(tinyxml2::XMLElement* elem);
	virtual void ParseDisplaySection(tinyxml2::XMLElement* elem);
	virtual void ParseShiftLightsSection(tinyxml2::XMLElement* elem);
	virtual void ParsePitLimiterSection(tinyxml2::XMLElement* elem);
	virtual void ParseShiftPointsSection(tinyxml2::XMLElement* elem);
	virtual void ParseBrightnessSection(tinyxml2::XMLElement* elem);
	virtual void ParseLedSection(tinyxml2::XMLElement* elem);
	virtual void ParseControlsInputs(tinyxml2::XMLElement* sectionElem);

	// =================================================================================
	// HELPERS
	// =================================================================================

	static std::vector<float> ParseCsvFloat(const char* text);
	static std::vector<int> ParseCsvInt(const char* text);

	static uint32_t ParseHexColor(const char* text);

	static std::string Trim(const std::string& str);

	// Convert:
	// "speed" -> DisplayFuncIndex::speed
	static int GetDisplayFuncIndex(const std::string& items, int swIndex = -1);

protected:

	Config m_config;
};