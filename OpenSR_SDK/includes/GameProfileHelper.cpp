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
// GameProfileHelper.cpp
// =====================================================================================
//
// Standalone XML parser for OpenSR game profiles.
//
// This helper allows plugin developers to:
//
// - load OpenSR XML profiles
// - parse dashboard configuration
// - parse shift-lights configuration
// - parse LED mappings
// - parse hardware input bindings
//
// =====================================================================================

#include "GameProfileHelper.h"
#include <iostream>

using namespace tinyxml2;

// =====================================================================================
// TRIM STRING
// =====================================================================================
//
// Removes:
// - spaces
// - tabs
// - CRLF
//
// Useful for parsing display function strings from XML.
//
// =====================================================================================

std::string GameProfileHelper::Trim(const std::string& str)
{
	size_t first = str.find_first_not_of(" \t\n\r");

	if (std::string::npos == first)
		return str;

	size_t last = str.find_last_not_of(" \t\n\r");

	return str.substr(first, (last - first + 1));
}

// =====================================================================================
// PARSE CSV FLOAT
// =====================================================================================
//
// Example:
//
// "55,60,64,68"
//
// becomes:
//
// { 55.0f, 60.0f, 64.0f, 68.0f }
//
// =====================================================================================

std::vector<float> GameProfileHelper::ParseCsvFloat(const char* text)
{
	std::vector<float> values;

	if (!text)
		return values;

	std::stringstream ss(text);

	std::string item;

	while (std::getline(ss, item, ','))
	{
		try {

			values.push_back(std::stof(item));
		}
		catch (...)
		{
			// Ignore malformed values
		}
	}

	return values;
}

// =====================================================================================
// PARSE CSV INTEGER
// =====================================================================================
//
// Example:
//
// "12000,12500,13000"
//
// becomes:
//
// { 12000, 12500, 13000 }
//
// =====================================================================================

std::vector<int> GameProfileHelper::ParseCsvInt(const char* text)
{
	std::vector<int> values;

	if (!text)
		return values;

	std::stringstream ss(text);

	std::string item;

	while (std::getline(ss, item, ','))
	{
		try {

			values.push_back(std::stoi(item));
		}
		catch (...)
		{
			// Ignore malformed values
		}
	}

	return values;
}

// =====================================================================================
// PARSE HEX COLOR
// =====================================================================================
//
// XML stores:
//
// RRGGBBAA
//
// Example:
//
// FF0000FF
//
// The helper converts it to:
//
// 0x00RRGGBB
//
// because most OpenSR LED systems internally use RGB.
//
// =====================================================================================

uint32_t GameProfileHelper::ParseHexColor(const char* text)
{
	if (!text)
		return 0;

	try {

		uint32_t rgba = (uint32_t)std::stoul(text, nullptr, 16);

		// Remove alpha byte
		return rgba >> 8;
	}
	catch (...)
	{
		return 0;
	}
}

// =====================================================================================
// LOAD PROFILE
// =====================================================================================
//
// Main XML parser entry point.
//
// This function:
//
// 1. Loads XML file
// 2. Clears previous mappings
// 3. Finds root <settings>
// 4. Parses all supported sections
//
// =====================================================================================

bool GameProfileHelper::LoadProfile(const std::wstring& profilePath)
{
	XMLDocument doc;
    FILE* file = _wfsopen(profilePath.c_str(), L"rb", _SH_DENYNO);
    if (!file) {
        std::wcerr << L"Failed to open file: " << profilePath << std::endl;
        return false;
    }

    XMLError error = doc.LoadFile(file);
    fclose(file);

    if (error != XML_SUCCESS) 
		return false;

	// Reset previous mappings
	inputMappings.clear();

	// Find XML root
	XMLElement* root = doc.FirstChildElement("settings");

	if (!root)
		return false;

	// =========================================================================
	// PARSE XML SECTIONS
	// =========================================================================

	if (auto elem = root->FirstChildElement("general"))
		ParseGeneralSection(elem);

	if (auto elem = root->FirstChildElement("display"))
		ParseDisplaySection(elem);

	if (auto elem = root->FirstChildElement("shiftlights"))
		ParseShiftLightsSection(elem);

	if (auto elem = root->FirstChildElement("pitlimiter"))
		ParsePitLimiterSection(elem);

	if (auto elem = root->FirstChildElement("shiftpoints"))
		ParseShiftPointsSection(elem);

	if (auto elem = root->FirstChildElement("brightness"))
		ParseBrightnessSection(elem);

	if (auto elem = root->FirstChildElement("led"))
		ParseLedSection(elem);

	return true;
}

// =====================================================================================
// GENERATE SHIFT LIGHTS
// =====================================================================================
//
// Automatically generates a progressive RPM LED curve.
//
// Example:
//
// 9 LEDs:
//
// 60%
// 65%
// 70%
// ...
// 97%
// 98%
// 99%
//
// Also automatically computes absolute RPM values.
//
// =====================================================================================

void GameProfileHelper::GenerateShiftLights(int numLeds, int maxRpm)
{
	if (numLeds <= 0)
		return;

	// Clear previous values
	m_config.shiftLights.rpmThresholdsPct.clear();
	m_config.shiftLights.rpmThresholdsAbs.clear();

	// Start later when there are fewer LEDs
	float startPct = 50.0f;

	if (numLeds < 10)
		startPct = 60.0f;

	float endPct = 99.0f;

	float step =
		(endPct - startPct) / (float)(numLeds - 1);

	for (int i = 0; i < numLeds; i++)
	{
		float pct = startPct + (step * i);

		// Tighten the last 3 LEDs
		if (numLeds > 5 && i >= numLeds - 3)
		{
			if (i == numLeds - 3)
				pct = 97.0f;

			if (i == numLeds - 2)
				pct = 98.0f;

			if (i == numLeds - 1)
				pct = 99.0f;
		}

		m_config.shiftLights.rpmThresholdsPct.push_back(
			floor(pct * 10) / 10
		);

		int absVal =
			(int)((pct / 100.0f) * maxRpm);

		m_config.shiftLights.rpmThresholdsAbs.push_back(absVal);
	}
}

// =====================================================================================
// DISPLAY FUNCTION STRING -> ENUM
// =====================================================================================
//
// Converts:
//
// "speed"
//
// into:
//
// DisplayFuncIndex::speed
//
// =====================================================================================

int GameProfileHelper::GetDisplayFuncIndex(
	const std::string& items,
	int swIndex)
{
	static std::map<std::string, int> funcMap =
	{
		{"none", 						DisplayFuncIndex::none},
		{"speed", 						DisplayFuncIndex::speed},
		{"lap", 						DisplayFuncIndex::lap},
		{"position", 					DisplayFuncIndex::position},
		{"fuel", 						DisplayFuncIndex::fuel},
		{"sector", 						DisplayFuncIndex::sector},
		{"gear", 						DisplayFuncIndex::gear},
		{"rpm", 						DisplayFuncIndex::rpm},
		{"laps remaining", 				DisplayFuncIndex::laps_remaining},
		{"brake bias", 					DisplayFuncIndex::brake_bias},
		{"total laps", 					DisplayFuncIndex::total_laps},
		{"system date", 				DisplayFuncIndex::system_date},
		{"system time", 				DisplayFuncIndex::system_time},
		{"osp label", 					DisplayFuncIndex::osp_text},
		{"osp factor", 					DisplayFuncIndex::osp_factor},
		{"brightness label", 			DisplayFuncIndex::brightness_text},
		{"brightness", 					DisplayFuncIndex::brightness},
		{"session time remaining", 		DisplayFuncIndex::session_time_remain},
		{"abs",							DisplayFuncIndex::antilock_brakes},
		{"traction control",			DisplayFuncIndex::traction_control},
		{"antiroll bar front",			DisplayFuncIndex::antiroll_bar_front},
		{"antiroll bar rear",			DisplayFuncIndex::antiroll_bar_rear},
		{"left weight jacket",			DisplayFuncIndex::left_weight_jacket},
		{"right weight jacket",			DisplayFuncIndex::right_weight_jacket},
		{"front flap",					DisplayFuncIndex::front_wing},
		{"rear flap",					DisplayFuncIndex::rear_wing},
		{"water temp",					DisplayFuncIndex::water_temp},
		{"oil temp",					DisplayFuncIndex::oil_temp},
		{"rev limit",					DisplayFuncIndex::rev_limit},
		{"fuel per lap",				DisplayFuncIndex::fuel_per_lap},
		{"fuel mix",					DisplayFuncIndex::fuel_mix},
		{"throttle shape",				DisplayFuncIndex::throttle_shape},
		{"diff entry",					DisplayFuncIndex::diff_entry},
		{"diff middle",					DisplayFuncIndex::diff_middle},
		{"diff exit",					DisplayFuncIndex::diff_exit},
		{"distance",					DisplayFuncIndex::distance},
		{"track size",					DisplayFuncIndex::track_size},

		{"current lap time", 			DisplayFuncIndex::current_lap_time},
		{"last lap time", 				DisplayFuncIndex::last_lap_time},
		{"best lap time", 				DisplayFuncIndex::best_lap_time},
		{"gap vs leader", 				DisplayFuncIndex::gap_vs_leader},
		{"gap vs next", 				DisplayFuncIndex::gap_vs_next},
		{"gap vs behind", 				DisplayFuncIndex::gap_vs_behind},
		{"delta time vs best", 			DisplayFuncIndex::delta_time_vs_best},
		{"delta time vs Best Native", 	DisplayFuncIndex::delta_time_vs_best_native},

		{"slipro fuel:speed", 			DisplayFuncIndex::slipro_fuel_speed},
		{"slipro position:speed", 		DisplayFuncIndex::slipro_position_speed},
		{"slipro lap:speed", 			DisplayFuncIndex::slipro_lap_speed},
		{"slipro sector:speed", 		DisplayFuncIndex::slipro_sector_speed},
		{"slipro lap:total laps", 		DisplayFuncIndex::slipro_lap_total_laps},
		{"slipro rpm:gear",				DisplayFuncIndex::slipro_rpm_gear},
		{"slipro fuel kg:speed",		DisplayFuncIndex::slipro_kg_fuel_speed},
	};

	std::string key;

	// =========================================================================
	// HANDLE ROTARY SWITCH LISTS
	// =========================================================================
	//
	// Example:
	//
	// "speed,lap,fuel"
	//
	// swIndex = 1
	//
	// returns:
	//
	// "lap"
	//
	// =========================================================================

	if (swIndex > -1)
	{
		std::stringstream ss(items);

		std::string item;

		int index = 0;

		while (std::getline(ss, item, ','))
		{
			if (index == swIndex)
			{
				key = Trim(item);
				break;
			}

			index++;
		}
	}
	else
	{
		key = Trim(items);
	}

	// Lowercase comparison
	std::transform(
		key.begin(),
		key.end(),
		key.begin(),
		::tolower
	);

	auto it = funcMap.find(key);

	if (it != funcMap.end())
		return it->second;

	// Default fallback
	return DisplayFuncIndex::speed;
}

// =====================================================================================
// GENERAL SECTION
// =====================================================================================

void GameProfileHelper::ParseGeneralSection(XMLElement* elem)
{
	// =========================================================================
	// GEAR SUBSECTION
	// =========================================================================

	XMLElement* gear = elem->FirstChildElement("gear");
	if (gear) {
		for (XMLElement* opt = gear->FirstChildElement("option"); opt; opt = opt->NextSiblingElement("option")) {
			const char* name = opt->Attribute("name");
			if (!name) continue;
			std::string n = name;
			if (n == "reverse" && opt->GetText()) m_config.general.reverseChar = opt->GetText()[0];
			else if (n == "neutral" && opt->GetText()) m_config.general.neutralChar = opt->GetText()[0];
			/* else if (n == "idle char" && opt->GetText()) m_config.general.idleChar =opt->GetText()[0];*/
			else if (n == "max forward gears at startup") m_config.general.maxForwardGears = opt->IntText(6);
		}
	}

	for (XMLElement* opt = elem->FirstChildElement("option"); opt; opt = opt->NextSiblingElement("option"))
	{
		const char* name = opt->Attribute("name");
		if (!name)
			continue;

		std::string n = name;

		// =========================================================================
		// GENERAL OPTIONS
		// =========================================================================

		if (n == "general unit")
		{
			const char* val = opt->GetText();

			if (val && strcmp(val, "imperial") == 0)
				m_config.general.useMetric = false;
			else
				m_config.general.useMetric = true;
		}
		else if (n == "speed unit")
		{
			const char* val = opt->GetText();

			if (val && strcmp(val, "mph") == 0)
				m_config.general.speedUnitMetric = false;
			else
				m_config.general.speedUnitMetric = true;
		}
		else if (n == "laptime report delay")
		{
			m_config.general.laptimeReportDelay =
				opt->IntText(8);
		}
		else if (n == "blink delay")
		{
			m_config.general.blinkDelay =
				opt->IntText(32);
		}
	}

	ParseControlsInputs(elem);
}

// =====================================================================================
// DISPLAY SECTION
// =====================================================================================

void GameProfileHelper::ParseDisplaySection(XMLElement* elem)
{
	for (XMLElement* opt = elem->FirstChildElement("option");
		opt;
		opt = opt->NextSiblingElement("option"))
	{
		const char* name = opt->Attribute("name");

		if (!name)
			continue;

		std::string n = name;

		const char* val = opt->GetText();

		if (!val)
			continue;

		if (n == "left display function")
		{
			m_config.display.leftDisplayFunc = val;
		}
		else if (n == "right display function")
		{
			m_config.display.rightDisplayFunc = val;
		}
		else if (n == "quick info left function")
		{
			m_config.display.quickInfoLeftFunc =
				GetDisplayFuncIndex(val);
		}
		else if (n == "quick info right function")
		{
			m_config.display.quickInfoRightFunc =
				GetDisplayFuncIndex(val);
		}
	}

	ParseControlsInputs(elem);
}

// =====================================================================================
// SHIFT LIGHTS SECTION
// =====================================================================================

void GameProfileHelper::ParseShiftLightsSection(XMLElement* elem)
{
	for (XMLElement* opt = elem->FirstChildElement("option");
		opt;
		opt = opt->NextSiblingElement("option"))
	{
		const char* name = opt->Attribute("name");

		if (!name)
			continue;

		std::string n = name;

		if (n == "shift-lights function allowed")
		{
			m_config.shiftLights.allowed =
				opt->BoolText(true);
		}
		else if (n == "shift-lights method")
		{
			const char* val = opt->GetText();

			if (val)
			{
				if (strcmp(val, "Progressive") == 0)
					m_config.shiftLights.method = mthProgressive;

				else if (strcmp(val, "Alternate") == 0)
					m_config.shiftLights.method = mthAlternate;

				else if (strcmp(val, "Percentage") == 0)
					m_config.shiftLights.method = mthPercentage;

				else if (strcmp(val, "Absolute") == 0)
					m_config.shiftLights.method = mthAbsolute;

				else if (strcmp(val, "Side To Center") == 0)
					m_config.shiftLights.method = mthSideToCenter;

				else if (strcmp(val, "F1 Semi Progressive") == 0)
					m_config.shiftLights.method = mthSemiProgressiveF1Style;

				else if (strcmp(val, "F1 KERS+Alternate") == 0)
					m_config.shiftLights.method = mthKERSGreenAndAlternate;

				else if (strcmp(val, "F1 Reverse KERS+Alternate") == 0)
					m_config.shiftLights.method = mthReversKERSGreenAndAlternate;

				else if (strcmp(val, "F1 DRS+Alternate") == 0)
					m_config.shiftLights.method = mthPercentageModernF1DRS;
			}
		}
		else if (n == "rpm threshold percentage")
		{
			m_config.shiftLights.rpmThresholdsPct =
				ParseCsvFloat(opt->GetText());
		}
		else if (n == "rpm threshold absolute")
		{
			m_config.shiftLights.rpmThresholdsAbs =
				ParseCsvInt(opt->GetText());
		}
		else if (n == "blank shift-lights with last gear")
		{
			m_config.shiftLights.blankWithLastGear =
				opt->BoolText(false);
		}
		else if (n == "shift-lights colors style") {
			const char* val = opt->GetText();
			if (val) {
				if (strcmp(val, "Native") == 0)  m_config.shiftLights.style = styleRally;
				else if (strcmp(val, "GT") == 0)  m_config.shiftLights.style = styleGt;
				else if (strcmp(val, "F1") == 0)  m_config.shiftLights.style = styleF1;
			}
		}
	}

	ParseControlsInputs(elem);
}

// =====================================================================================
// PIT LIMITER SECTION
// =====================================================================================

void GameProfileHelper::ParsePitLimiterSection(XMLElement* elem)
{
	for (XMLElement* opt = elem->FirstChildElement("option");
		opt;
		opt = opt->NextSiblingElement("option"))
	{
		const char* name = opt->Attribute("name");

		if (!name)
			continue;

		std::string n = name;

		if (n == "pit-limiter function allowed")
		{
			m_config.pitLimiter.allowed =
				opt->BoolText(true);
		}
		else if (n == "pit-limiter active state")
		{
			int state = std::stoi(opt->GetText());

			if (state > 0)
				m_config.pitLimiter.method = state;
		}
		else if (n == "pit-limiter alternate state")
		{
			int state = std::stoi(opt->GetText());

			if (state > 0)
				m_config.pitLimiter.altMethod = state;
		}
		else if (n == "pit-limiter on rpm led only")
		{
			m_config.pitLimiter.onRpmLedOnly =
				opt->BoolText(false);
		}
		else if (n == "pit-limiter blink delay")
		{
			m_config.pitLimiter.blinkDelay =
				opt->IntText(32);
		}
	}

	ParseControlsInputs(elem);
}

// =====================================================================================
// SHIFT POINTS SECTION
// =====================================================================================

void GameProfileHelper::ParseShiftPointsSection(XMLElement* elem)
{
	for (XMLElement* opt = elem->FirstChildElement("option");
		opt;
		opt = opt->NextSiblingElement("option"))
	{
		const char* name = opt->Attribute("name");

		if (!name)
			continue;

		std::string n = name;

		const char* val = opt->GetText();

		if (n == "optimal shift-points method" && val)
		{
			if (strcmp(val, "Default") == 0)
				m_config.shiftPoints.method = mtdDefault;

			else if (strcmp(val, "Default and Last RPM blinking") == 0)
				m_config.shiftPoints.method = mtdDefaultAndLastRPMLedBlinking;

			else if (strcmp(val, "Default Blinking and Last RPM Not Blinking") == 0)
				m_config.shiftPoints.method = mtdDefaultBlinkingLastRPMNotBlinking;

			else if (strcmp(val, "Last RPM Alone Blinking") == 0)
				m_config.shiftPoints.method = mtdLastRPMAloneBlinking;

			else if (strcmp(val, "Last RPM Alone Not Blinking") == 0)
				m_config.shiftPoints.method = mtdLastRPMAloneNotBlinking;
		}
		else if (n == "optimal shift-points function allowed")
		{
			m_config.shiftPoints.allowed =
				opt->BoolText(true);
		}
		else if (n == "optimal shift-points factor")
		{
			m_config.shiftPoints.ospFactor =
				opt->IntText(150);
		}
		else if (n == "optimal shift-points factors values")
		{
			m_config.shiftPoints.factorsPerGear =
				ParseCsvFloat(opt->GetText());
		}
		else if (n == "optimal shift-points blink delay")
		{
			m_config.shiftPoints.blinkDelay =
				opt->IntText(4);
		}
		else if (n == "optimal shift-points factors match gear ratio allowed")
		{
			m_config.shiftPoints.matchGearRatio =
				opt->BoolText(false);
		}
		else if (n == "optimal shift-points left char")
		{
			m_config.shiftPoints.ospLChar =
				opt->GetText()[0];
		}
		else if (n == "optimal shift-points right char")
		{
			m_config.shiftPoints.ospRChar =
				opt->GetText()[0];
		}
		else if (n == "optimal shift-points left-right chars allowed")
		{
			m_config.shiftPoints.isOspOnGearAllowed =
				opt->BoolText(true);
		}
	}

	ParseControlsInputs(elem);
}

// =====================================================================================
// BRIGHTNESS SECTION
// =====================================================================================

void GameProfileHelper::ParseBrightnessSection(XMLElement* elem)
{
	// no Brightness option here as it is now global (in plugin settings not in game profile)
	for (XMLElement* opt = elem->FirstChildElement("option"); opt; opt = opt->NextSiblingElement("option"))
	{
		const char* name = opt->Attribute("name");
		if (!name)
			continue;

		std::string n = name;
		if (n == "brightness step value")
		{
			// brightness increment step value for inputs
			m_config.brightness.stepValue =
				opt->IntText(10);
		}
	}
	// control Brightness with inputs
	ParseControlsInputs(elem);
}

// =====================================================================================
// LED SECTION
// =====================================================================================

void GameProfileHelper::ParseLedSection(XMLElement* elem)
{
	for (XMLElement* opt = elem->FirstChildElement("option");
		opt;
		opt = opt->NextSiblingElement("option"))
	{
		const char* name = opt->Attribute("name");
		const char* type = opt->Attribute("type");

		if (!name || !type)
			continue;

		std::string t = type;

		// =========================================================================
		// RGB LED
		// =========================================================================

		if (t == "rgba_led")
		{
			int idx = opt->IntAttribute("index");

			const char* hexStr = opt->GetText();

			if (hexStr)
			{
				m_config.led.colors[idx] =
					ParseHexColor(hexStr);
			}
		}

		// =========================================================================
		// FLAG / EXTERNAL LED
		// =========================================================================

		else if (t == "flag_external_led")
		{
			m_config.led.flagExternalEventMasks[name] =
				opt->IntText(0);
		}
	}
}

// =====================================================================================
// CONTROLS / INPUTS
// =====================================================================================
//
// Parses controller bindings.
//
// Example:
//
// <option signature="SLI-PRO Interface_7634_259_6-000000-0-0000" device="SLI-PRO Interface" name="btn quick information" tooltip="Input to show quick information" type="input" info="Button11">B11</option>
//
// =====================================================================================

void GameProfileHelper::ParseControlsInputs(XMLElement* sectionElem)
{
	if (!sectionElem)
		return;

	XMLElement* controls =
		sectionElem->FirstChildElement("controls");

	if (!controls)
		return;

	// One device block per section
	XMLElement* deviceElem =
		controls->FirstChildElement();

	if (!deviceElem)
		return;

	for (XMLElement* opt = deviceElem->FirstChildElement("option");
		opt;
		opt = opt->NextSiblingElement("option"))
	{
		const char* name = opt->Attribute("name");
		const char* signature = opt->Attribute("signature");
		const char* type = opt->Attribute("type");
		const char* text = opt->GetText();

		if (!name || !signature || !text)
			continue;

		// Only process input bindings
		if (type && std::string(type) != "input")
			continue;

		std::string input = text;

		// Invalid input
		if (input.size() < 2)
			continue;

		try
		{
			int devIndex =
				std::stoi(input.substr(1));

			// =========================================================================
			// BUTTON
			// =========================================================================

			if (input[0] == 'B')
			{
				inputMappings[signature][devIndex] = name;
			}

			// =========================================================================
			// AXIS
			// =========================================================================

			else if (input[0] == 'S')
			{
				inputMappings[signature][devIndex + 1000] = name;
			}

			// =========================================================================
			// HAT
			// =========================================================================

			else if (input[0] == 'H')
			{
				inputMappings[signature][devIndex + 2000] = name;
			}
		}
		catch (...)
		{
			// Ignore malformed inputs
		}
	}
}
