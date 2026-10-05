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
#ifndef OPENSR_HELPER_H
#define OPENSR_HELPER_H

// VS2010: Ensure we target Windows Vista/7 APIs at minimum for QueryFullProcessImageName
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include <windows.h>
#include <string>
#include <tchar.h>
#include <shlobj.h> // Required for SHCreateDirectoryEx (Recursive mkdir)
#include <errno.h>

// Link against Shell32.lib for directory creation
#pragma comment(lib, "Shell32.lib")

class OpenSRHelper {
public:

    // -------------------------------------------------------------------------
    // IsProcessRunningByPIDAndName
    // Checks if a PID is active and matches the expected executable name.
    // -------------------------------------------------------------------------
    static bool IsProcessRunningByPIDAndName(DWORD pid, const TCHAR* targetName) {
        // PROCESS_QUERY_LIMITED_INFORMATION is available on Vista+ (VS2010 supports this)
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, pid);
        if (!hProcess)
            return false;

        // Check if process is still running
        DWORD waitResult = WaitForSingleObject(hProcess, 0);
        if (waitResult != WAIT_TIMEOUT) {
            // Process has signaled (exited)
            CloseHandle(hProcess);
            return false;
        }

        TCHAR processPath[MAX_PATH] = { 0 };
        DWORD size = MAX_PATH;

        // QueryFullProcessImageName works on VS2010 if _WIN32_WINNT >= 0x0600
        if (!QueryFullProcessImageName(hProcess, 0, processPath, &size)) {
            CloseHandle(hProcess);
            return false;
        }

        // Extract filename from path (Find last backslash)
        const TCHAR* baseName = _tcsrchr(processPath, _T('\\'));
        baseName = baseName ? baseName + 1 : processPath;

        // Case-insensitive comparison
        bool match = (_tcsicmp(baseName, targetName) == 0);
        CloseHandle(hProcess);
        return match;
    }

    // -------------------------------------------------------------------------
    // Safe String Copying
    // Replaced complex templates with simple overloads for VS2010 stability.
    // -------------------------------------------------------------------------

    // Overload for std::string (char)
    static bool SafeTstrcpy(const std::string& source, char* destBuffer, size_t destBufferSize)
    {
        if (!destBuffer || destBufferSize == 0) return false;
        if (source.length() + 1 > destBufferSize) {
            destBuffer[0] = '\0';
            return false;
        }
        strcpy_s(destBuffer, destBufferSize, source.c_str());
        return true;
    }

    // Overload for std::wstring (wchar_t)
    static bool SafeTstrcpy(const std::wstring& source, wchar_t* destBuffer, size_t destBufferSize)
    {
        if (!destBuffer || destBufferSize == 0) return false;
        if (source.length() + 1 > destBufferSize) {
            destBuffer[0] = L'\0';
            return false;
        }
        wcscpy_s(destBuffer, destBufferSize, source.c_str());
        return true;
    }

    // Alias for specific usage
    static bool SafeWstrcpy(const std::wstring& source, wchar_t* destBuffer, size_t destBufferSize) {
        return SafeTstrcpy(source, destBuffer, destBufferSize);
    }

    // -------------------------------------------------------------------------
    // Create Directory Helper
    // Replaces std::filesystem::create_directories
    // -------------------------------------------------------------------------
    
    // Overload for std::wstring
    static int createDir(const std::wstring& path, bool create_parents = false) {
        if (create_parents) {
            // SHCreateDirectoryEx automatically creates parent directories (mkdir -p)
            // It requires <shlobj.h> and Shell32.lib
            int result = SHCreateDirectoryExW(NULL, path.c_str(), NULL);
            
            // ERROR_SUCCESS (0) -> Created
            // ERROR_ALREADY_EXISTS -> Exists (Success for our needs)
            if (result == ERROR_SUCCESS || result == ERROR_ALREADY_EXISTS || result == ERROR_FILE_EXISTS) {
                return 0;
            }
            return result; // Return Windows Error Code
        }
        else {
            // Standard CreateDirectory (single level)
            if (CreateDirectoryW(path.c_str(), NULL)) {
                return 0;
            }
            DWORD err = GetLastError();
            if (err == ERROR_ALREADY_EXISTS) {
                return 0;
            }
            return (int)err;
        }
    }

    // Overload for std::string
    static int createDir(const std::string& path, bool create_parents = false) {
        // Convert to wstring for the API call (Best practice in Windows)
        std::wstring wpath(path.begin(), path.end());
        return createDir(wpath, create_parents);
    }
};

#endif // OPENSR_HELPER_H