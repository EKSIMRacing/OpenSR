// ConfigHelper.h
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

#pragma once

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include "tinyxml2.h"

/**
 * @brief ConfigHelper provides a toolbox for loading and parsing OpenSR 
 * configuration profiles.
 * 
 * It includes a C-compatible struct (OpenSRConfig) to hold the configuration
 * and a static Load function to handle the boilerplate of file I/O and XML traversal.
 */
class ConfigHelper {
public:
    /**
     * @brief C-Compatible Configuration Structure.
     * This struct uses fixed-size arrays and primitive types to remain compatible
     * with low-level device drivers while storing all OpenSR configuration data.
     */
    struct OpenSRConfig {
        // General
        char unit[16] = "metric";
        char speedUnit[8] = "kmh";
        int blinkDelay = 600;
        int laptimeReportDelay = 8;
        uint8_t flagLedAllowed = 0; // 0=false, 1=true
        char neutralChar = 'n';
        char reverseChar = 'r';
        int maxForwardGears = 6;

        // Display
        char displayFunc[64] = "speed";
        char leftDisplayFunc[64] = "speed";
        char rightDisplayFunc[64] = "current lap time";
        char quickInfoFunc[64] = "position";
        char quickInfoLeftFunc[64] = "position";
        char quickInfoRightFunc[64] = "fuel";
        uint8_t showGearWhenShifting = 1;

        // ShiftLights
        uint8_t shiftLightsAllowed = 1;
        char shiftLightsMethod[32] = "Percentage";
        float rpmThresholdAbs[24] = {0};
        float rpmThresholdPct[24] = {0};
        char shiftLightsColorsStyle[16] = "GT";
        uint8_t blankShiftLightsWithLastGear = 0;

        // PitLimiter
        uint8_t pitLimiterAllowed = 1;
        int pitLimiterActiveState = 0;
        int pitLimiterBlinkDelay = 600;
        uint8_t pitLimiterOnRpmLedOnly = 0;

        // ShiftPoints
        uint8_t shiftPointsAllowed = 1;
        char shiftPointsMethod[32] = "default";
        uint8_t shiftPointsAutoBlinkAllowed = 0;
        int shiftPointsBlinkDelay = 50;
        int shiftPointsFactor = 120;
        uint8_t shiftPointsMatchGearRatioAllowed = 0;
        float shiftPointsFactorsValues[10] = {0};
        uint8_t shiftPointsLeftRightCharsAllowed = 1;
        char shiftPointsLeftChar = '[';
        char shiftPointsRightChar = ']';
        uint8_t shiftPointsOnFirstGearAllowed = 0;
        uint8_t shiftPointsRpmThresholdsAllowed = 0;
        float shiftPointsRpmThresholds[10] = {0};

        // Brightness
        int brightnessStepValue = 10;

        // Led
        // Indices 1-10 (for Fanatec RGB)
        uint32_t fanatecRgbColors[10] = {0};
        
        // Flag External Masks
        // We use a fixed array of key-value pairs to maintain C-compatibility
        struct FlagMask {
            char name[32];
            int mask;
        } flagExternalMasks[20];
        int flagExternalMaskCount = 0;
    };

    /**
     * @brief Primary entry point for third-party developers.
     * Loads the XML file and populates the provided OpenSRConfig struct.
     */
    static bool LoadOpenSRConfig(const std::wstring& filepath, OpenSRConfig* outConfig);

    // --- Helper Toolbox Methods ---
    
    // Standard primitive decoders (Wrappers for tinyxml2 Query methods)
    bool GetBool(tinyxml2::XMLElement* elem, const std::string& name, bool defaultValue);
    int GetInt(tinyxml2::XMLElement* elem, const std::string& name, int defaultValue);
    float GetFloat(tinyxml2::XMLElement* elem, const std::string& name, float defaultValue);
    std::string GetString(tinyxml2::XMLElement* elem, const std::string& name, const std::string& defaultValue);

    // Non-standard decoders
    std::vector<float> GetCsvFloat(tinyxml2::XMLElement* elem, const std::string& name, std::vector<float> defaultValue);
    std::vector<int> GetCsvInt(tinyxml2::XMLElement* elem, const std::string& name, std::vector<int> defaultValue);
    uint32_t GetHexColor(tinyxml2::XMLElement* elem, const std::string& name, uint32_t defaultValue);
    std::string dfTrim(const std::string& str);

protected:
    virtual bool ParseBool(const char* text);
    virtual int ParseInt(const char* text);
    virtual float ParseFloat(const char* text);
    virtual std::vector<float> ParseCsvFloat(const char* text);
    virtual std::vector<int> ParseCsvInt(const char* text);
    virtual uint32_t ParseHexColor(const char* text);

private:
    tinyxml2::XMLElement* FindOption(tinyxml2::XMLElement* elem, const std::string& name);
};
