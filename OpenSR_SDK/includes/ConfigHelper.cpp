// ConfigHelper.cpp
//
// Legacy/simple profile loader for OpenSR game profile XML files.
//
// Use ConfigHelper when:
// - your plugin targets an older toolchain (e.g. Visual Studio 2010)
// - you need a plain C-compatible struct (OpenSRConfig: fixed arrays,
//   no std::string/std::map) to share with low-level device drivers
//
// Use GameProfileHelper (modern C++11+) when:
// - you want enums (display functions, shift-light methods)
// - you need parsed hardware input bindings (inputMappings)
// - you need GenerateShiftLights() or subclassable section parsers
//
// Both parse the same <settings> XML format.
// GameProfileHelper is the recommended default for new plugins.

#include "ConfigHelper.h"
#include <sstream>
#include <iostream>
#include <cstring>

using namespace tinyxml2;

// --- Loader Implementation ---

bool ConfigHelper::LoadOpenSRConfig(const std::wstring& filepath, OpenSRConfig* outConfig) {
    if (!outConfig) return false;

    XMLDocument doc;
    FILE* file = _wfsopen(filepath.c_str(), L"rb", _SH_DENYNO);
    if (!file) {
        std::wcerr << L"Failed to open file: " << filepath << std::endl;
        return false;
    }

    XMLError error = doc.LoadFile(file);
    fclose(file);

    if (error != XML_SUCCESS) return false;

    XMLElement* root = doc.FirstChildElement("settings");
    if (!root) return false;

    ConfigHelper helper;

    // --- General ---
    XMLElement* gen = root->FirstChildElement("general");
    if (gen) {
        std::string unit = helper.GetString(gen, "general unit", "metric");
        strncpy(outConfig->unit, unit.c_str(), sizeof(outConfig->unit) - 1);
        
        std::string speed = helper.GetString(gen, "speed unit", "kmh");
        strncpy(outConfig->speedUnit, speed.c_str(), sizeof(outConfig->speedUnit) - 1);
        
        outConfig->blinkDelay = helper.GetInt(gen, "blink delay", 600);
        outConfig->laptimeReportDelay = helper.GetInt(gen, "laptime report delay", 8);
        outConfig->flagLedAllowed = helper.GetBool(gen, "Flag LED Allowed (12 RPM LED Mode + Flag LED)", false) ? 1 : 0;
        
        XMLElement* gear = gen->FirstChildElement("gear");
        if (gear) {
            outConfig->neutralChar = helper.GetString(gear, "neutral", "n")[0];
            outConfig->reverseChar = helper.GetString(gear, "reverse", "r")[0];
            outConfig->maxForwardGears = helper.GetInt(gear, "max forward gears at startup", 6);
        }
    }

    // --- Display ---
    XMLElement* disp = root->FirstChildElement("display");
    if (disp) {
        std::string dFunc = helper.GetString(disp, "display function", "speed");
        strncpy(outConfig->displayFunc, dFunc.c_str(), sizeof(outConfig->displayFunc) - 1);
        
        std::string lFunc = helper.GetString(disp, "left display function", "speed");
        strncpy(outConfig->leftDisplayFunc, lFunc.c_str(), sizeof(outConfig->leftDisplayFunc) - 1);
        
        std::string rFunc = helper.GetString(disp, "right display function", "current lap time");
        strncpy(outConfig->rightDisplayFunc, rFunc.c_str(), sizeof(outConfig->rightDisplayFunc) - 1);
        
        std::string qFunc = helper.GetString(disp, "quick info function", "position");
        strncpy(outConfig->quickInfoFunc, qFunc.c_str(), sizeof(outConfig->quickInfoFunc) - 1);
        
        std::string qLFunc = helper.GetString(disp, "quick info left function", "position");
        strncpy(outConfig->quickInfoLeftFunc, qLFunc.c_str(), sizeof(outConfig->quickInfoLeftFunc) - 1);
        
        std::string qRFunc = helper.GetString(disp, "quick info right function", "fuel");
        strncpy(outConfig->quickInfoRightFunc, qRFunc.c_str(), sizeof(outConfig->quickInfoRightFunc) - 1);
        
        outConfig->showGearWhenShifting = helper.GetBool(disp, "show gear when shifting", true) ? 1 : 0;
    }

    // --- ShiftLights ---
    XMLElement* sl = root->FirstChildElement("shiftlights");
    if (sl) {
        outConfig->shiftLightsAllowed = helper.GetBool(sl, "shift-lights function allowed", true) ? 1 : 0;
        std::string m = helper.GetString(sl, "shift-lights method", "Percentage");
        strncpy(outConfig->shiftLightsMethod, m.c_str(), sizeof(outConfig->shiftLightsMethod) - 1);
        
        std::vector<float> absVals = helper.GetCsvFloat(sl, "rpm threshold absolute", {});
        for(size_t i=0; i < std::min((size_t)24, absVals.size()); ++i) outConfig->rpmThresholdAbs[i] = absVals[i];
        
        std::vector<float> pctVals = helper.GetCsvFloat(sl, "rpm threshold percentage", {});
        for(size_t i=0; i < std::min((size_t)24, pctVals.size()); ++i) outConfig->rpmThresholdPct[i] = pctVals[i];
        
        std::string style = helper.GetString(sl, "shift-lights colors style", "GT");
        strncpy(outConfig->shiftLightsColorsStyle, style.c_str(), sizeof(outConfig->shiftLightsColorsStyle) - 1);
        outConfig->blankShiftLightsWithLastGear = helper.GetBool(sl, "blank shift-lights with last gear", false) ? 1 : 0;
    }

    // --- PitLimiter ---
    XMLElement* pl = root->FirstChildElement("pitlimiter");
    if (pl) {
        outConfig->pitLimiterAllowed = helper.GetBool(pl, "pit-limiter function allowed", true) ? 1 : 0;
        outConfig->pitLimiterActiveState = helper.GetInt(pl, "pit-limiter active state", 0);
        outConfig->pitLimiterBlinkDelay = helper.GetInt(pl, "pit-limiter blink delay", 600);
        outConfig->pitLimiterOnRpmLedOnly = helper.GetBool(pl, "pit-limiter on rpm led only", false) ? 1 : 0;
    }

    // --- ShiftPoints ---
    XMLElement* sp = root->FirstChildElement("shiftpoints");
    if (sp) {
        outConfig->shiftPointsAllowed = helper.GetBool(sp, "optimal shift-points function allowed", true) ? 1 : 0;
        std::string m = helper.GetString(sp, "optimal shift-points method", "default");
        strncpy(outConfig->shiftPointsMethod, m.c_str(), sizeof(outConfig->shiftPointsMethod) - 1);
        outConfig->shiftPointsAutoBlinkAllowed = helper.GetBool(sp, "optimal shift-points auto blink allowed", false) ? 1 : 0;
        outConfig->shiftPointsBlinkDelay = helper.GetInt(sp, "optimal shift-points blink delay", 50);
        outConfig->shiftPointsFactor = helper.GetInt(sp, "optimal shift-points factor", 120);
        outConfig->shiftPointsMatchGearRatioAllowed = helper.GetBool(sp, "optimal shift-points factors match gear ratio allowed", false) ? 1 : 0;
        
        std::vector<float> factors = helper.GetCsvFloat(sp, "optimal shift-points factors values", {});
        for(size_t i=0; i < std::min((size_t)10, factors.size()); ++i) outConfig->shiftPointsFactorsValues[i] = factors[i];
        
        outConfig->shiftPointsLeftRightCharsAllowed = helper.GetBool(sp, "optimal shift-points left-right chars allowed", true) ? 1 : 0;
        outConfig->shiftPointsLeftChar = helper.GetString(sp, "optimal shift-points left char", "[")[0];
        outConfig->shiftPointsRightChar = helper.GetString(sp, "optimal shift-points right char", "]")[0];
        outConfig->shiftPointsOnFirstGearAllowed = helper.GetBool(sp, "optimal shift-points on first gear allowed", false) ? 1 : 0;
        outConfig->shiftPointsRpmThresholdsAllowed = helper.GetBool(sp, "optimal shift-points rpm thresholds allowed", false) ? 1 : 0;
        
        std::vector<float> rps = helper.GetCsvFloat(sp, "optimal shift-points rpm thresholds", {});
        for(size_t i=0; i < std::min((size_t)10, rps.size()); ++i) outConfig->shiftPointsRpmThresholds[i] = rps[i];
    }

    // --- Brightness ---
    XMLElement* br = root->FirstChildElement("brightness");
    if (br) {
        outConfig->brightnessStepValue = helper.GetInt(br, "brightness step value", 10);
    }

    // --- Led ---
    XMLElement* led = root->FirstChildElement("led");
    if (led) {
        for (XMLElement* opt = led->FirstChildElement("option"); opt; opt = opt->NextSiblingElement("option")) {
            const char* name = opt->Attribute("name");
            const char* type = opt->Attribute("type");
            if (!name || !type) continue;
            
            std::string n = name;
            std::string t = type;

            if (t == "fanatec_rgb_led") {
                int idx = opt->IntAttribute("index");
                if (idx >= 1 && idx <= 10) {
                    outConfig->fanatecRgbColors[idx - 1] = helper.GetHexColor(led, name, 0);
                }
            }
            else if (t == "flag_external_led") {
                if (outConfig->flagExternalMaskCount < 20) {
                    strncpy(outConfig->flagExternalMasks[outConfig->flagExternalMaskCount].name, n.c_str(), 31);
                    outConfig->flagExternalMasks[outConfig->flagExternalMaskCount].mask = helper.GetInt(led, name, 0);
                    outConfig->flagExternalMaskCount++;
                }
            }
        }
    }

    return true;
}

// --- Helper Toolbox Implementation ---

XMLElement* ConfigHelper::FindOption(XMLElement* elem, const std::string& name) {
    if (!elem) return nullptr;
    for (XMLElement* opt = elem->FirstChildElement("option"); opt; opt = opt->NextSiblingElement("option")) {
        const char* attrName = opt->Attribute("name");
        if (attrName && std::string(attrName) == name) return opt;
    }
    return nullptr;
}

bool ConfigHelper::GetBool(XMLElement* elem, const std::string& name, bool defaultValue) {
    XMLElement* opt = FindOption(elem, name);
    if (!opt || !opt->GetText()) return defaultValue;
    return ParseBool(opt->GetText());
}

int ConfigHelper::GetInt(XMLElement* elem, const std::string& name, int defaultValue) {
    XMLElement* opt = FindOption(elem, name);
    if (!opt || !opt->GetText()) return defaultValue;
    return ParseInt(opt->GetText());
}

float ConfigHelper::GetFloat(XMLElement* elem, const std::string& name, float defaultValue) {
    XMLElement* opt = FindOption(elem, name);
    if (!opt || !opt->GetText()) return defaultValue;
    return ParseFloat(opt->GetText());
}

std::string ConfigHelper::GetString(XMLElement* elem, const std::string& name, const std::string& defaultValue) {
    XMLElement* opt = FindOption(elem, name);
    if (!opt || !opt->GetText()) return defaultValue;
    return std::string(opt->GetText());
}

std::vector<float> ConfigHelper::GetCsvFloat(XMLElement* elem, const std::string& name, std::vector<float> defaultValue) {
    XMLElement* opt = FindOption(elem, name);
    if (!opt || !opt->GetText()) return defaultValue;
    return ParseCsvFloat(opt->GetText());
}

std::vector<int> ConfigHelper::GetCsvInt(XMLElement* elem, const std::string& name, std::vector<int> defaultValue) {
    XMLElement* opt = FindOption(elem, name);
    if (!opt || !opt->GetText()) return defaultValue;
    return ParseCsvInt(opt->GetText());
}

uint32_t ConfigHelper::GetHexColor(XMLElement* elem, const std::string& name, uint32_t defaultValue) {
    XMLElement* opt = FindOption(elem, name);
    if (!opt || !opt->GetText()) return defaultValue;
    return ParseHexColor(opt->GetText());
}

bool ConfigHelper::ParseBool(const char* text) {
    std::string s(text);
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return (s == "true" || s == "1" || s == "yes");
}

int ConfigHelper::ParseInt(const char* text) {
    try { return std::stoi(text); } catch (...) { return 0; }
}

float ConfigHelper::ParseFloat(const char* text) {
    try { return std::stof(text); } catch (...) { return 0.0f; }
}

std::vector<float> ConfigHelper::ParseCsvFloat(const char* text) {
    std::vector<float> values;
    if (!text) return values;
    std::stringstream ss(text);
    std::string item;
    while (std::getline(ss, item, ',')) {
        try { values.push_back(std::stof(item)); } catch (...) {}
    }
    return values;
}

std::vector<int> ConfigHelper::ParseCsvInt(const char* text) {
    std::vector<int> values;
    if (!text) return values;
    std::stringstream ss(text);
    std::string item;
    while (std::getline(ss, item, ',')) {
        try { values.push_back(std::stoi(item)); } catch (...) {}
    }
    return values;
}

uint32_t ConfigHelper::ParseHexColor(const char* text) {
    if (!text) return 0;
    try {
        uint32_t rgba = (uint32_t)std::stoul(text, nullptr, 16);
        return rgba >> 8;
    } catch (...) { return 0; }
}

std::string ConfigHelper::dfTrim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (std::string::npos == first) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}
