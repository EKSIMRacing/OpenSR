// CSharpPluginBridge.h
#pragma once
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
#include <stdint.h>
// Forward declarations
#ifdef PLUGIN_TYPE_OUT
namespace CSharpOutPlugin {
    ref class Plugin;
}
#else
namespace CSharpInPlugin {
    ref class Plugin;
}
#endif

// Bridge namespace
namespace CSharpPluginBridge {

    // Wrapper class for C# Plugin
    public ref class PluginWrapper {
    public:
        // This function will set up the .NET part
        static bool InitializeBridge();
        static bool CS_Init(void* ctxPtr, void* buffers, const wchar_t* path);
        static bool CS_Start();
        static void CS_Pause();
        static void CS_Resume();
        static bool CS_IsRunning();
        static void CS_Stop();
        static bool CS_Shutdown();
        static void CS_OnContextChanged(int reason);
        static bool CS_IsSettingsReady();
        static void CS_GetMeta(int kind, char* buffer, int bufferSize);
        static void CS_GetMetaW(int kind, wchar_t* buffer, int bufferSize);
        static int CS_GetPluginType();
        static void CS_CheckDevice();
        static bool CS_IsCheckingAllowed();
        static uint64_t CS_GetSharedPointer();
    };

    // Native C exports
    extern "C" {
        __declspec(dllexport) bool __stdcall CS_InitializeBridge();
        __declspec(dllexport) bool __stdcall CS_Init(void* ctxPtr, void* buffers, const wchar_t* path);
        __declspec(dllexport) bool __stdcall CS_Start();
        __declspec(dllexport) void __stdcall CS_Pause();
        __declspec(dllexport) void __stdcall CS_Resume();
        __declspec(dllexport) bool __stdcall CS_IsRunning();
        __declspec(dllexport) void __stdcall CS_Stop();
        __declspec(dllexport) bool __stdcall CS_Shutdown();
        __declspec(dllexport) void __stdcall CS_OnContextChanged(int reason);
        __declspec(dllexport) bool __stdcall CS_IsSettingsReady();
        __declspec(dllexport) void __stdcall CS_GetMeta(int kind, char* buffer, int bufferSize);
        __declspec(dllexport) void __stdcall CS_GetMetaW(int kind, wchar_t* buffer, int bufferSize);
        __declspec(dllexport) int  __stdcall CS_GetPluginType();
        __declspec(dllexport) void __stdcall CS_CheckDevice();
        __declspec(dllexport) bool __stdcall CS_IsCheckingAllowed();
        __declspec(dllexport) uint64_t __stdcall CS_GetSharedPointer();
    }
}
