#pragma once
/*
#####################################################################
# Open Sim Relay (OpenSR) - Plugin Interface & Context Data
# Copyright (c) 2025 Zappadoc - All Rights Reserved
#
# This file is part of the OpenSR Plugins Interface SDK.
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
#   Do not modify this file. Future versions of OpenSR may update 
#   these definitions; always use the official SDK headers.
#
# Change History:
#   2025-10-01 : Initial public release
#####################################################################
*/

#define NOMINMAX
#include <Windows.h>
#include <string_view>
#include <string>
#include <system_error>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <limits>
#include <type_traits> // For std::is_same_v, etc.
#pragma comment(lib, "comctl32.lib")


namespace StringUtils {



    //  Forward Declarations for our conversion functions
    [[nodiscard]] std::wstring toWide(std::string_view narrowStr, UINT codePage = CP_UTF8);
    [[nodiscard]] std::string toNarrow(std::wstring_view wideStr, UINT codePage = CP_UTF8);


    //  Implementation of conversion functions
#ifdef _WIN32
    inline std::string toLower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return s;
    }

    [[nodiscard]] inline std::vector<std::string> SplitString(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        if (str.empty()) return tokens;
        std::string token;
        std::istringstream tokenStream(str);
        while (std::getline(tokenStream, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }

    [[nodiscard]] inline std::vector<std::wstring> SplitWString(const std::string& input, char delimiter) {
        std::vector<std::wstring> tokens;
        std::string token;
        std::istringstream tokenStream(input);
        while (std::getline(tokenStream, token, delimiter)) {
            token.erase(0, token.find_first_not_of(" \t\n\r"));
            token.erase(token.find_last_not_of(" \t\n\r") + 1);
            if (!token.empty()) {
                int size = MultiByteToWideChar(CP_UTF8, 0, &token[0], (int)token.size(), NULL, 0);
                std::wstring wstr(size, 0);
                MultiByteToWideChar(CP_UTF8, 0, &token[0], (int)token.size(), &wstr[0], size);
                tokens.push_back(wstr);
            }
        }
        return tokens;
    }

    [[nodiscard]] inline std::string SanitizeWindowsName(const std::string& raw) {
        std::string s = raw;

        // replace forbidden chars + space
        for (auto& c : s)
            if (strchr(" <>:\"/\\|?*", c)) c = '_';

        // trim trailing dots/underscores
        while (!s.empty() && (s.back() == '.' || s.back() == '_'))
            s.pop_back();

        // uppercased copy for reserved check
        std::string up = s;
        std::transform(up.begin(), up.end(), up.begin(), ::toupper);

        // reserved names
        if (up == "CON" || up == "PRN" || up == "AUX" || up == "NUL" ||
            (up.size() >= 4 && ((up.substr(0, 3) == "COM") || (up.substr(0, 3) == "LPT")) && isdigit(up[3])))
            s = "_" + s;

        return s;
    }

    [[nodiscard]] inline std::wstring SanitizeWindowsNameW(const std::wstring& raw) {
        std::wstring s = raw;

        // Replace forbidden chars + space with '_'
        for (auto& c : s)
            if (wcschr(L" <>:\"/\\|?*", c)) c = L'_';

        // Trim trailing dots/underscores
        while (!s.empty() && (s.back() == L'.' || s.back() == L'_')) s.pop_back();

        // Uppercase copy for reserved check
        std::wstring up = s;
        std::transform(up.begin(), up.end(), up.begin(), ::towupper);

        // Reserved names
        if (up == L"CON" || up == L"PRN" || up == L"AUX" || up == L"NUL" ||
            (up.size() >= 4 && (up.substr(0, 3) == L"COM" || up.substr(0, 3) == L"LPT") && iswdigit(up[3])))
            s = L"_" + s;

        return s;
    }

    template<typename... Args>
    inline void s_fmt(std::string& out, const char* fmt, Args&&... args)
    {
        int size = std::snprintf(nullptr, 0, fmt, std::forward<Args>(args)...);

        if (size <= 0)
        {
            out.clear();
            return;
        }

        out.resize(size);

        std::snprintf(out.data(), size + 1, fmt, std::forward<Args>(args)...);
    }
    /**
     * @brief Concatenates multiple string-like parts and safely assigns
     * the result to a fixed-size C-style array. It automatically handles type conversions
     * (e.g., std::string to wchar_t array) and deduces the buffer size.
     *
     * @tparam N The size of the destination buffer (deduced automatically).
     * @tparam CharT The character type of the destination buffer (deduced automatically).
     * @tparam Args A pack of different string-like types to concatenate.
     * @param destBuffer The destination array, e.g., `instance.path`.
     * @param args The parts to join, e.g., `L"C:\\", "folder", L"\\file.txt"`.
     */
    template <size_t N, typename CharT, typename... Args>
    [[nodiscard]] void assignAndBuild(CharT(&destBuffer)[N], Args&&... args) {
        // 1. Safety: Initialize the first character to null immediately.
        // If the logic fails or produces nothing, we at least return an empty string, not garbage.
        destBuffer[0] = static_cast<CharT>(0);

        std::basic_ostringstream<CharT> oss;

        auto process_arg = [&](auto&& arg) {
            using ArgType = std::decay_t<decltype(arg)>;

            // 2. Safety: Check for Null Pointers (for const char* / const wchar_t*)
            if constexpr (std::is_pointer_v<ArgType>) {
                if (arg == nullptr) {
                    return; // Skip nullptr, do not attempt to read or print it
                }
            }

            if constexpr (std::is_same_v<ArgType, std::string>) {
                if (arg.empty()) return; // Skip empty std::strings

                if constexpr (std::is_same_v<CharT, wchar_t>) {
                    oss << toWide(arg);
                }
                else {
                    oss << arg;
                }
            }
            else if constexpr (std::is_same_v<ArgType, std::wstring>) {
                if (arg.empty()) return; // Skip empty std::wstrings

                if constexpr (std::is_same_v<CharT, char>) {
                    oss << toNarrow(arg);
                }
                else {
                    oss << arg;
                }
            }
            else if constexpr (std::is_same_v<ArgType, std::filesystem::path>) {
                if (arg.empty()) return; // Skip empty paths

                if constexpr (std::is_same_v<CharT, wchar_t>) {
                    oss << arg.wstring();
                }
                else {
                    oss << arg.string();
                }
            }
            else {
                // Primitive types (int, float, valid char*)
                oss << arg;
            }
        };

        // Apply the lambda to all arguments
        (process_arg(std::forward<Args>(args)), ...);

        std::basic_string<CharT> final_string = oss.str();

        // 3. Safety: If result is empty, we are done (buffer is already \0 from step 1)
        if (final_string.empty()) {
            return;
        }

        // 4. Safely copy
        if constexpr (std::is_same_v<CharT, char>) {
            strncpy_s(destBuffer, N, final_string.c_str(), _TRUNCATE);
        }
        else {
            wcsncpy_s(destBuffer, N, final_string.c_str(), _TRUNCATE);
        }
    }
    //template <size_t N, typename CharT, typename... Args>
    //[[nodiscard]] void assignAndBuild(CharT(&destBuffer)[N], Args&&... args) {
    //    std::basic_ostringstream<CharT> oss;

    //    // A C++17 fold expression with a lambda to process each argument
    //    // This is where the magic happens!
    //    auto process_arg = [&](auto&& arg) {
    //        // Get the "decayed" type of the argument (removes const, &, etc.)
    //        using ArgType = std::decay_t<decltype(arg)>;

    //        if constexpr (std::is_same_v<ArgType, std::string>) {
    //            if constexpr (std::is_same_v<CharT, wchar_t>) {
    //                oss << toWide(arg); // Convert char-string to wide
    //            }
    //            else {
    //                oss << arg; // Types match
    //            }
    //        }
    //        else if constexpr (std::is_same_v<ArgType, std::wstring>) {
    //            if constexpr (std::is_same_v<CharT, char>) {
    //                oss << toNarrow(arg); // Convert wide-string to char
    //            }
    //            else {
    //                oss << arg; // Types match
    //            }
    //        }
    //        else if constexpr (std::is_same_v<ArgType, std::filesystem::path>) {
    //            // Filesystem paths need to be explicitly converted to a string type
    //            if constexpr (std::is_same_v<CharT, wchar_t>) {
    //                oss << arg.wstring();
    //            }
    //            else {
    //                oss << arg.string();
    //            }
    //        }
    //        else {
    //            // For other types like const char*, const wchar_t*, integers, etc.
    //            oss << arg;
    //        }
    //    };

    //    // Apply the lambda to all arguments
    //    (process_arg(std::forward<Args>(args)), ...);

    //    std::basic_string<CharT> final_string = oss.str();

    //    // Safely copy the result to the destination buffer
    //    if constexpr (std::is_same_v<CharT, char>) {
    //        strncpy_s(destBuffer, N, final_string.c_str(), _TRUNCATE);
    //    }
    //    else {
    //        wcsncpy_s(destBuffer, N, final_string.c_str(), _TRUNCATE);
    //    }
    //}
    [[nodiscard]] inline std::wstring toWide(std::string_view narrowStr, UINT codePage) {
        // 1. Handle empty input immediately
        if (narrowStr.empty()) return {};

        // 2. Safety Check: Windows API only accepts 'int' length. 
        // If string is larger than INT_MAX (2GB), throw to prevent overflow crash.
        if (narrowStr.length() > static_cast<size_t>((std::numeric_limits<int>::max)())) {
            throw std::length_error("toWide: String is too long for Windows API.");
        }

        // 3. Calculate required length (Pass length explicitly, do not rely on null terminator)
        int inputLen = static_cast<int>(narrowStr.length());
        int wideCharCount = MultiByteToWideChar(codePage, 0, narrowStr.data(), inputLen, nullptr, 0);

        if (wideCharCount == 0) {
            throw std::system_error(GetLastError(), std::system_category(), "MultiByteToWideChar failed size check.");
        }

        // 4. Allocate buffer
        std::wstring wideStr(wideCharCount, L'\0');

        // 5. Convert
        if (MultiByteToWideChar(codePage, 0, narrowStr.data(), inputLen, &wideStr[0], wideCharCount) == 0) {
            throw std::system_error(GetLastError(), std::system_category(), "MultiByteToWideChar failed conversion.");
        }

        return wideStr;
    }

    [[nodiscard]] inline std::string toNarrow(std::wstring_view wideStr, UINT codePage) {
        // 1. Handle empty input immediately
        if (wideStr.empty()) return {};

        // 2. Safety Check
        if (wideStr.length() > static_cast<size_t>((std::numeric_limits<int>::max)())) {
            throw std::length_error("toNarrow: String is too long for Windows API.");
        }

        // 3. Calculate required length
        int inputLen = static_cast<int>(wideStr.length());
        int mbCharCount = WideCharToMultiByte(codePage, 0, wideStr.data(), inputLen, nullptr, 0, nullptr, nullptr);

        if (mbCharCount == 0) {
            throw std::system_error(GetLastError(), std::system_category(), "WideCharToMultiByte failed size check.");
        }

        // 4. Allocate buffer
        std::string narrowStr(mbCharCount, '\0');

        // 5. Convert
        if (WideCharToMultiByte(codePage, 0, wideStr.data(), inputLen, &narrowStr[0], mbCharCount, nullptr, nullptr) == 0) {
            throw std::system_error(GetLastError(), std::system_category(), "WideCharToMultiByte failed conversion.");
        }

        return narrowStr;
    }

    //[[nodiscard]] inline std::wstring toWide(std::string_view narrowStr, UINT codePage) {
    //    if (narrowStr.empty()) return {};
    //    int wideCharCount = MultiByteToWideChar(codePage, 0, narrowStr.data(), (int)narrowStr.length(), nullptr, 0);
    //    if (wideCharCount == 0) throw std::system_error(GetLastError(), std::system_category(), "MultiByteToWideChar failed size check.");
    //    std::wstring wideStr(wideCharCount, L'\0');
    //    if (MultiByteToWideChar(codePage, 0, narrowStr.data(), (int)narrowStr.length(), &wideStr[0], wideCharCount) == 0) {
    //        throw std::system_error(GetLastError(), std::system_category(), "MultiByteToWideChar failed conversion.");
    //    }
    //    return wideStr;
    //}

    //[[nodiscard]] inline std::string toNarrow(std::wstring_view wideStr, UINT codePage) {
    //    if (wideStr.empty()) return {};
    //    int mbCharCount = WideCharToMultiByte(codePage, 0, wideStr.data(), (int)wideStr.length(), nullptr, 0, nullptr, nullptr);
    //    if (mbCharCount == 0) throw std::system_error(GetLastError(), std::system_category(), "WideCharToMultiByte failed size check.");
    //    std::string narrowStr(mbCharCount, '\0');
    //    if (WideCharToMultiByte(codePage, 0, wideStr.data(), (int)wideStr.length(), &narrowStr[0], mbCharCount, nullptr, nullptr) == 0) {
    //        throw std::system_error(GetLastError(), std::system_category(), "WideCharToMultiByte failed conversion.");
    //    }
    //    return narrowStr;
    //}

    /**
 * @brief Creates a std::string by concatenating multiple string-like parts.
 * Automatically handles conversion from wide strings (wchar_t) to narrow (char).
 *
 * @tparam Args A variable number of string-like arguments.
 * @param args The arguments to concatenate (e.g., "Path: ", L"wide_string", 123).
 * @return A new std::string with all parts joined and converted.
 */
    template <typename... Args>
    [[nodiscard]] std::string createString(Args&&... args) {
        std::ostringstream oss;

        auto process_arg = [&](auto&& arg) {
            using ArgType = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<ArgType, std::wstring>) {
                oss << toNarrow(arg); // Convert wide-string to narrow
            }
            else if constexpr (std::is_same_v<ArgType, std::filesystem::path>) {
                oss << arg.string(); // Use the narrow representation of the path
            }
            else {
                oss << arg; // All other types (char*, int, std::string)
            }
        };
        (process_arg(std::forward<Args>(args)), ...);
        return oss.str();
    }

    /**
     * @brief Creates a std::wstring by concatenating multiple string-like parts.
     * Automatically handles conversion from narrow strings (char) to wide (wchar_t).
     *
     * @tparam Args A variable number of string-like arguments.
     * @param args The arguments to concatenate (e.g., L"Path: ", "narrow_string", 123).
     * @return A new std::wstring with all parts joined and converted.
     */
    template <typename... Args>
    [[nodiscard]] std::wstring createWString(Args&&... args) {
        std::wostringstream oss;

        auto process_arg = [&](auto&& arg) {
            using ArgType = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<ArgType, std::string>) {
                oss << toWide(arg); // Convert narrow-string to wide
            }
            else if constexpr (std::is_same_v<ArgType, std::filesystem::path>) {
                oss << arg.wstring(); // Use the wide representation of the path
            }
            else {
                oss << arg; // All other types (wchar_t*, int, std::wstring)
            }
        };
        (process_arg(std::forward<Args>(args)), ...);
        return oss.str();
    }


    /**
     * @brief Checks if a file or directory exists by joining multiple path components.
     * This is a one-liner replacement for `std::filesystem::exists(path1 / path2 / ...)`.
     * It works with any mix of C-style buffers, std::strings, literals, etc.
     *
     * @tparam Args A variable number of path-like arguments.
     * @param args The path components to join (e.g., context.osrDocFolder, instance.path, "settings.dll").
     * @return True if the combined path exists, false otherwise.
     */
    template <typename... Args>
    [[nodiscard]] bool pathExists(Args&&... args) {
        // std::filesystem::path constructor and the / operator are already very powerful.
        // We can use a C++17 fold expression to chain them together elegantly.
        std::filesystem::path combined_path;
        (combined_path /= ... /= std::filesystem::path(std::forward<Args>(args)));
        return std::filesystem::exists(combined_path);
    }

    /**
     * @brief Checks if a C-style string is null or empty.
     */
    template <typename CharT>
    [[nodiscard]] bool isEmpty(const CharT* c_str) {
        return c_str == nullptr || *c_str == '\0';
    }
        
    /**
     * @brief Safely clears a fixed-size character buffer, making it an empty string.
     * This is the helper equivalent of `buffer[0] = '\0';`.
     *
     * @tparam N The size of the buffer (deduced automatically).
     * @tparam CharT The character type of the buffer (deduced automatically).
     * @param buffer The C-style array to clear.
     */
    template <size_t N, typename CharT>
    void clearBuffer(CharT(&buffer)[N]) {
        // Check that the buffer is not zero-sized, though this is a rare edge case.
        if constexpr (N > 0) {
            buffer[0] = CharT{ '\0' };
        }
    }

    /**
 * @brief Safely compares a C-style character buffer with another string-like value.
 * This is the correct replacement for `if (buffer == "literal")`. It performs
 * a content comparison, not a pointer comparison.
 *
 * @tparam N The size of the buffer (deduced automatically).
 * @tparam CharT The character type of the buffer (deduced automatically).
 * @tparam OtherStringType The type of the value to compare against (e.g., const char*, std::string).
 * @param buffer The C-style array to check.
 * @param other The value to compare the buffer's content against.
 * @return True if the contents are identical, false otherwise.
 */
    template <size_t N, typename CharT, typename OtherStringType>
    [[nodiscard]] bool equals(const CharT(&buffer)[N], const OtherStringType& other) {
        // std::basic_string_view is perfect for this. It can be constructed from
        // almost any string type without allocating memory, providing a unified
        // way to view the data for comparison.
        return std::basic_string_view<CharT>(buffer) == std::basic_string_view<CharT>(other);
    }

    /**
     * @brief (Case-Insensitive Version) Safely compares a C-style character buffer
     * with another string-like value, ignoring case.
     *
     * @tparam N The size of the buffer (deduced automatically).
     * @tparam CharT The character type of the buffer (deduced automatically).
     * @tparam OtherStringType The type of the value to compare against.
     * @param buffer The C-style array to check.
     * @param other The value to compare against.
     * @return True if the contents are identical ignoring case, false otherwise.
     */
    template <size_t N, typename CharT, typename OtherStringType>
    [[nodiscard]] bool equalsIgnoreCase(const CharT(&buffer)[N], const OtherStringType& other) {
        std::basic_string_view<CharT> view1(buffer);
        std::basic_string_view<CharT> view2(other);

        if (view1.length() != view2.length()) {
            return false;
        }

        // Use std::equal with a custom predicate for case-insensitive comparison
        return std::equal(view1.begin(), view1.end(), view2.begin(), view2.end(),
            [](CharT a, CharT b) {
                // A simple toupper/towupper for ASCII/basic multilingual plane characters.
                // For full Unicode correctness, a dedicated library would be better,
                // but this is sufficient for most use cases (like filenames on Windows).
                if constexpr (std::is_same_v<CharT, char>) {
                    return toupper(static_cast<unsigned char>(a)) == toupper(static_cast<unsigned char>(b));
                }
                else {
                    return towupper(static_cast<wint_t>(a)) == towupper(static_cast<wint_t>(b));
                }
            }
        );
    }


#else
// Provide non-windows stubs if needed, or implement using other libraries
#endif

} // namespace StringUtils