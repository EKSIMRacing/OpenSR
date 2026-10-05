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

#include <filesystem>
#include <system_error> // For std::error_code
#include <cerrno>       // For errno constants like ENOTDIR
#include <string>       // For std::string and std::wstring
#include <string.h>
#include <locale>       // For setting up wcout locale
#include <tchar.h>
#include <iostream>
#include <mutex>

namespace fs = std::filesystem;

class OpenSRHelper {
public:
    // uint32_t signal allows the enum values to pass through implicitly
    static inline void setSignal(uint32_t& mask, uint32_t signal) {
        mask |= signal;
    }

    static inline void clearSignal(uint32_t& mask, uint32_t signal) {
        mask &= ~signal;
    }

    static inline bool testSignal(uint32_t mask, uint32_t signal) {
        return (mask & signal) != 0;
    }

    static bool IsProcessRunningByPIDAndName(DWORD pid, const TCHAR* targetName)
    {
        using clock = std::chrono::steady_clock;

        static std::mutex mtx;
        std::lock_guard<std::mutex> lock(mtx);

        // CRITICAL FIX: Initialize lastCheck to the past so the very first call doesn't hit the 50ms throttle
        static clock::time_point lastCheck = clock::now() - std::chrono::milliseconds(100);
        static clock::time_point lastHardReset = clock::now();
        static clock::time_point lastNameCheck = clock::now() - std::chrono::seconds(1);

        static bool lastResult = false;
        static bool lastNameMatch = false;
        static HANDLE hProcess = nullptr;
        static DWORD lastPid = 0;

        auto now = clock::now();

        // 1. Hard reset every 5 seconds
        if (now - lastHardReset > std::chrono::seconds(5)) {
            lastHardReset = now;
            if (hProcess && hProcess != INVALID_HANDLE_VALUE) {
                CloseHandle(hProcess);
                hProcess = nullptr;
            }
            lastNameCheck = now - std::chrono::seconds(1);

            // FIX: Force Step 3 to pass by resetting lastCheck into the past
            lastCheck = now - std::chrono::milliseconds(100);
        }

        // 2. Clear entire cache state if the PID changes
        if (pid != lastPid) {
            if (hProcess && hProcess != INVALID_HANDLE_VALUE) {
                CloseHandle(hProcess);
                hProcess = nullptr;
            }
            lastPid = pid;
            lastResult = false;
            lastNameMatch = false;
            lastNameCheck = now - std::chrono::seconds(1);

            // FIX: Force Step 3 to pass by resetting lastCheck into the past
            lastCheck = now - std::chrono::milliseconds(100);
        }

        // 3. Fast cache path (50ms rate limit)
        if (now - lastCheck < std::chrono::milliseconds(50)) {
            return lastResult;
        }
        lastCheck = now;

        // 4. Open process handle safely
        if (hProcess == nullptr || hProcess == INVALID_HANDLE_VALUE) {
            hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, pid);
            lastNameCheck = now - std::chrono::seconds(1); // Force a name validation on new handle

            if (!hProcess || hProcess == INVALID_HANDLE_VALUE) {
                hProcess = nullptr;
                lastResult = false;
                lastNameMatch = false;
                return false;
            }
        }

        // 5. Verify process is still active
        DWORD waitResult = WaitForSingleObject(hProcess, 0);
        if (waitResult != WAIT_TIMEOUT) {
            CloseHandle(hProcess);
            hProcess = nullptr;
            lastResult = false;
            lastNameMatch = false;
            return false;
        }

        // 6. Rate-limited process name verification (500ms)
        if (now - lastNameCheck > std::chrono::milliseconds(500)) {
            lastNameCheck = now;

            TCHAR processPath[MAX_PATH] = { 0 };
            DWORD size = MAX_PATH;

            if (!QueryFullProcessImageName(hProcess, 0, processPath, &size)) {
                lastNameMatch = false;
            }
            else {
                const TCHAR* baseName = _tcsrchr(processPath, _T('\\'));
                baseName = baseName ? baseName + 1 : processPath;
                lastNameMatch = (_tcsicmp(baseName, targetName) == 0);
            }
        }

        lastResult = lastNameMatch;
        return lastResult;
    }

//    static bool IsProcessRunningByPIDAndName(DWORD pid, const TCHAR* targetName)
//    {
//        using clock = std::chrono::steady_clock;
//
//        static std::mutex mtx;
//        std::lock_guard<std::mutex> lock(mtx);
//
//        static clock::time_point lastCheck = clock::now();
//        static clock::time_point lastHardReset = clock::now();
//        static clock::time_point lastNameCheck = clock::now() - std::chrono::seconds(1);
//
//        static bool lastResult = false;
//        static bool lastNameMatch = false;
//        static HANDLE hProcess = nullptr;
//        static DWORD lastPid = 0;
//
//        auto now = clock::now();
//
//        // 1. Hard reset every 5 seconds
//        if (now - lastHardReset > std::chrono::seconds(5)) {
//            //std::cout << "[DEBUG] --- 5s Hard Reset Triggered ---" << std::endl;
//            lastHardReset = now;
//            if (hProcess && hProcess != INVALID_HANDLE_VALUE) {
//              // std::cout << "[DEBUG] Closing old handle during hard reset." << std::endl;
//                CloseHandle(hProcess);
//                hProcess = nullptr;
//            }
//            lastNameCheck = now - std::chrono::seconds(1);
//        }
//
//        // 2. Clear entire cache state if the PID changes
//        if (pid != lastPid) {
//           // std::cout << "[DEBUG] PID Changed from " << lastPid << " to " << pid << ". Clearing cache." << std::endl;
//            if (hProcess && hProcess != INVALID_HANDLE_VALUE) {
//                CloseHandle(hProcess);
//                hProcess = nullptr;
//            }
//            lastPid = pid;
//            lastResult = false;
//            lastNameMatch = false;
//            lastNameCheck = now - std::chrono::seconds(1);
//        }
//
//        // 3. Fast cache path (50ms rate limit)
//        if (now - lastCheck < std::chrono::milliseconds(50)) {
//            // Uncomment the line below if you want to see the fast-path spam, 
//            // but it will flood your console very quickly.
//            // std::cout << "[DEBUG] Fast cache hit (<50ms). Returning: " << lastResult << std::endl;
//            return lastResult;
//        }
//        lastCheck = now;
//
//        // 4. Open process handle safely
//        if (hProcess == nullptr || hProcess == INVALID_HANDLE_VALUE) {
//           // std::cout << "[DEBUG] Attempting to OpenProcess for PID: " << pid << std::endl;
//            hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, pid);
//            lastNameCheck = now - std::chrono::seconds(1); // Force a name validation on new handle
//
//            if (!hProcess || hProcess == INVALID_HANDLE_VALUE) {
//               // std::cout << "[DEBUG] FAIL: OpenProcess failed. Error code: " << GetLastError() << std::endl;
//                hProcess = nullptr;
//                lastResult = false;
//                lastNameMatch = false;
//                return false;
//            }
//          //  std::cout << "[DEBUG] SUCCESS: Opened process handle: " << hProcess << std::endl;
//        }
//
//        // 5. Verify process is still active
//        DWORD waitResult = WaitForSingleObject(hProcess, 0);
//        if (waitResult != WAIT_TIMEOUT) {
//        /*    std::cout << "[DEBUG] FAIL: WaitForSingleObject returned " << waitResult
//                << " (Process likely exited). Closing handle." << std::endl;*/
//            CloseHandle(hProcess);
//            hProcess = nullptr;
//            lastResult = false;
//            lastNameMatch = false;
//            return false;
//        }
//
//        // 6. Rate-limited process name verification (500ms)
//        if (now - lastNameCheck > std::chrono::milliseconds(500)) {
//            //std::cout << "[DEBUG] Running 500ms rate-limited process name verification..." << std::endl;
//            lastNameCheck = now;
//
//            TCHAR processPath[MAX_PATH] = { 0 };
//            DWORD size = MAX_PATH;
//
//            if (!QueryFullProcessImageName(hProcess, 0, processPath, &size)) {
//              //  std::cout << "[DEBUG] FAIL: QueryFullProcessImageName failed. Error: " << GetLastError() << std::endl;
//                lastNameMatch = false;
//            }
//            else {
//                const TCHAR* baseName = _tcsrchr(processPath, _T('\\'));
//                baseName = baseName ? baseName + 1 : processPath;
//
//                lastNameMatch = (_tcsicmp(baseName, targetName) == 0);
//
//                // Safe printing regardless of UNICODE / MBCS settings
////#ifdef UNICODE
////                std::wcout << L"[DEBUG] Extracted Name: " << baseName << L" | Target: " << targetName
////                    << L" | Match Result: " << lastNameMatch << std::endl;
////#else
////                std::cout << "[DEBUG] Extracted Name: " << baseName << " | Target: " << targetName
////                    << " | Match Result: " << lastNameMatch << std::endl;
////#endif
//            }
//        }
//
//        lastResult = lastNameMatch;
//        // std::cout << "[DEBUG] Final Function Output for this frame: " << lastResult << std::endl;
//        return lastResult;
//    }
//
//    //static bool IsProcessRunningByPIDAndName(DWORD pid, const TCHAR* targetName)
    //{
    //    using clock = std::chrono::steady_clock;

    //    // Mutex to protect all static variables from concurrent thread access
    //    static std::mutex mtx;
    //    std::lock_guard<std::mutex> lock(mtx);

    //    static clock::time_point lastCheck = clock::now();
    //    static clock::time_point lastHardReset = clock::now();
    //    static clock::time_point lastNameCheck = clock::now() - std::chrono::milliseconds(600);

    //    static bool lastResult = false;
    //    static bool lastNameMatch = false;
    //    static HANDLE hProcess = nullptr;
    //    static DWORD lastPid = 0;

    //    auto now = clock::now();

    //    // 1. Hard reset every 5 seconds
    //    if (now - lastHardReset > std::chrono::seconds(5)) {
    //        lastHardReset = now;
    //        lastNameCheck = now - std::chrono::milliseconds(600);
    //        if (hProcess && hProcess != INVALID_HANDLE_VALUE) {
    //            CloseHandle(hProcess);
    //            hProcess = nullptr;
    //        }
    //    }

    //    // 2. Clear entire cache state if the PID changes
    //    if (pid != lastPid) {
    //        if (hProcess && hProcess != INVALID_HANDLE_VALUE) {
    //            CloseHandle(hProcess);
    //            hProcess = nullptr;
    //        }
    //        lastPid = pid;
    //        lastResult = false;
    //        lastNameMatch = false;
    //        lastNameCheck = now - std::chrono::milliseconds(600);
    //    }

    //    // 3. Fast cache path (50ms rate limit)
    //    if (now - lastCheck < std::chrono::milliseconds(50)) {
    //        return lastResult;
    //    }
    //    lastCheck = now;

    //    // 4. Open process handle safely
    //    if (hProcess == nullptr || hProcess == INVALID_HANDLE_VALUE) {
    //        hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, pid);
    //    }

    //    if (!hProcess || hProcess == INVALID_HANDLE_VALUE) {
    //        hProcess = nullptr; // Ensure it stays null if OpenProcess failed
    //        lastResult = false;
    //        return false;
    //    }

    //    // 5. Verify process is still active
    //    DWORD waitResult = WaitForSingleObject(hProcess, 0);
    //    if (waitResult != WAIT_TIMEOUT) {
    //        CloseHandle(hProcess);
    //        hProcess = nullptr;
    //        lastResult = false;
    //        return false;
    //    }

    //    // 6. Rate-limited process name verification (500ms)
    //    if (now - lastNameCheck > std::chrono::milliseconds(500)) {
    //        lastNameCheck = now;

    //        TCHAR processPath[MAX_PATH] = { 0 };
    //        DWORD size = MAX_PATH;

    //        if (!QueryFullProcessImageName(hProcess, 0, processPath, &size)) {
    //            lastNameMatch = false;
    //        }
    //        else {
    //            const TCHAR* baseName = _tcsrchr(processPath, _T('\\'));
    //            baseName = baseName ? baseName + 1 : processPath;
    //            lastNameMatch = (_tcsicmp(baseName, targetName) == 0);
    //        }
    //    }

    //    lastResult = lastNameMatch;
    //    return lastResult;
    //}

    //static bool IsProcessRunningByPIDAndName(DWORD pid, const TCHAR* targetName)
    //{
    //    using clock = std::chrono::steady_clock;

    //    // --- Cache global pour éviter les appels trop fréquents ---
    //    static clock::time_point lastCheck = clock::now();
    //    static bool lastResult = false;

    //    // --- On ne refait une vérification complète que toutes les 50 ms ---
    //    auto now = clock::now();
    //    if (now - lastCheck < std::chrono::milliseconds(50)) {
    //        return lastResult;
    //    }
    //    lastCheck = now;

    //    // --- Handle persistant pour éviter OpenProcess en boucle ---
    //    static HANDLE hProcess = nullptr;

    //    // Si pas encore ouvert ou si le PID a changé, on ouvre un nouveau handle
    //    static DWORD lastPid = 0;
    //    if (pid != lastPid || hProcess == nullptr) {
    //        if (hProcess) {
    //            CloseHandle(hProcess);
    //        }
    //        hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, pid);
    //        lastPid = pid;
    //    }

    //    if (!hProcess) {
    //        lastResult = false;
    //        return false;
    //    }

    //    // --- Vérifie si le process est encore vivant ---
    //    DWORD waitResult = WaitForSingleObject(hProcess, 0);
    //    if (waitResult != WAIT_TIMEOUT) {
    //        CloseHandle(hProcess);
    //        hProcess = nullptr;
    //        lastResult = false;
    //        return false;
    //    }

    //    // --- Vérification du nom du process (toutes les 500 ms) ---
    //    static clock::time_point lastNameCheck = clock::now();
    //    static bool lastNameMatch = false;

    //    if (now - lastNameCheck > std::chrono::milliseconds(500)) {
    //        lastNameCheck = now;

    //        TCHAR processPath[MAX_PATH] = { 0 };
    //        DWORD size = MAX_PATH;

    //        if (!QueryFullProcessImageName(hProcess, 0, processPath, &size)) {
    //            lastNameMatch = false;
    //        }
    //        else {
    //            const TCHAR* baseName = _tcsrchr(processPath, _T('\\'));
    //            baseName = baseName ? baseName + 1 : processPath;
    //            lastNameMatch = (_tcsicmp(baseName, targetName) == 0);
    //        }
    //    }

    //    lastResult = lastNameMatch;
    //    return lastResult;
    //}

    //static bool IsProcessRunningByPIDAndName(DWORD pid, const TCHAR* targetName) {
    //    static auto last = std::chrono::steady_clock::now();
    //    auto now = std::chrono::steady_clock::now();

    //    if (now - last < std::chrono::milliseconds(50)) {
    //        return true; // ou la dernière valeur connue si tu veux être exact
    //    }

    //    last = now;

    //    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, pid);
    //    if (!hProcess)
    //        return false;

    //    DWORD waitResult = WaitForSingleObject(hProcess, 0);
    //    if (waitResult != WAIT_TIMEOUT) {
    //        CloseHandle(hProcess);
    //        return false;
    //    }

    //    TCHAR processPath[MAX_PATH] = { 0 };
    //    DWORD size = MAX_PATH;
    //    if (!QueryFullProcessImageName(hProcess, 0, processPath, &size)) {
    //        CloseHandle(hProcess);
    //        return false;
    //    }

    //    const TCHAR* baseName = _tcsrchr(processPath, _T('\\'));
    //    baseName = baseName ? baseName + 1 : processPath;

    //    bool match = (_tcsicmp(baseName, targetName) == 0);
    //    CloseHandle(hProcess);
    //    return match;
    //}

    template<typename CharT>
    static bool SafeTstrcpy(const std::basic_string<CharT>& source, CharT* destBuffer, size_t destBufferSize);

    // Specialization for char (std::string)
    template<>
   static bool SafeTstrcpy<char>(const std::string& source, char* destBuffer, size_t destBufferSize)
    {
        if (!destBuffer || destBufferSize == 0) return false;
        if (source.length() + 1 > destBufferSize) {
            destBuffer[0] = '\0';
            return false;
        }
        strcpy_s(destBuffer, destBufferSize, source.c_str());
        return true;
    }

    // Specialization for wchar_t (std::wstring)
    template<>
  static  bool SafeTstrcpy<wchar_t>(const std::wstring& source, wchar_t* destBuffer, size_t destBufferSize)
    {
        if (!destBuffer || destBufferSize == 0) return false;
        if (source.length() + 1 > destBufferSize) {
            destBuffer[0] = L'\0';
            return false;
        }
        wcscpy_s(destBuffer, destBufferSize, source.c_str());
        return true;
    }
    /**
     * @brief Safely copies a std::wstring into a C-style wide character buffer.
     *
     * @param source The std::wstring to copy from.
     * @param destBuffer The destination buffer to copy to.
     * @param destBufferSize The total number of elements (not bytes) in the destination buffer.
     * @return true if the copy was successful.
     * @return false if the source string was too large to fit in the buffer.
     */
   static bool SafeWstrcpy(const std::wstring& source, wchar_t* destBuffer, size_t destBufferSize)
    {
        // Check if the destination buffer is invalid or has zero size.
        if (destBuffer == nullptr || destBufferSize == 0) {
            return false;
        }

        // Check if the source string (including the null terminator) will fit.
        if (source.length() + 1 > destBufferSize) {
            // It won't fit. To be safe, set the first character to null
            // so the buffer is a valid, empty C-string.
            destBuffer[0] = L'\0';
            return false;
        }

        // Use the secure CRT function to perform the copy.
        // It will copy the string and append the null terminator.
        wcscpy_s(destBuffer, destBufferSize, source.c_str());
        return true;
    }


   /**
 * @brief Creates a directory with options to create parent directories.
 *
 * This is a C++17-based, C-style-return helper function that works with
 * various string types (const char*, std::string, const wchar_t*, std::wstring).
 * It is declared static to be easily included in multiple source files.
 *
 * @tparam PathType The type of the path string.
 * @param p The path of the directory to create.
 * @param create_parents If true, creates all necessary parent directories (like mkdir -p).
 *                       If false (default), only creates the final directory and fails
 *                       if a parent does not exist.
 * @return 0 on success (directory was created or already existed).
 * @return A non-zero error code (like errno) on failure.
 */
   template<typename PathType>
   static int createDir(const PathType& p, bool create_parents = false) {
       const fs::path path(p);
       std::error_code ec;

       if (create_parents) {
           // This function handles the "already exists" case as a success,
           // so the logic is simpler.
           fs::create_directories(path, ec);
       }
       else {
           // This function errors if the directory already exists, so we must
           // handle that case specifically to treat it as a success.
           fs::create_directory(path, ec);
       }

       if (!ec) {
           // No error occurred, success.
           return 0;
       }

       // An error occurred. Check if it's the "already exists" case,
       // which we want to treat as a success.
       if (ec == std::errc::file_exists) {
           // We must verify that the existing path is a directory.
           // If it's a file, it's an error.
           std::error_code is_dir_ec;
           if (fs::is_directory(path, is_dir_ec) && !is_dir_ec) {
               return 0; // It exists and is a directory, so success.
           }
           else {
               return ENOTDIR; // It exists but is a file, or another error occurred.
           }
       }

       // For any other error, return the platform-specific error code.
       return ec.value();
   }
   
   
};