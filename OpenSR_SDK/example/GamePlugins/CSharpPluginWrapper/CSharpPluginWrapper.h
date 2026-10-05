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

#include "IOpenSRPlugin.h"
#include "OpenSRBuffers.h"
#include <string>
#include <functional>
#include <windows.h>
#include <mutex>
#include <atomic>

// Typedefs for the C# exposed functions
extern "C" {
    typedef bool(__stdcall* CS_InitializeBridgeFunc)();
    typedef bool(__stdcall* CS_InitFunc)(OpenSRContext* context, void* buffers, const wchar_t* pluginPath);
    typedef bool(__stdcall* CS_StartFunc)();
    typedef bool(__stdcall* CS_PauseFunc)();
    typedef bool(__stdcall* CS_ResumeFunc)();
    typedef void(__stdcall* CS_StopFunc)();
    typedef void(__stdcall* CS_ShutdownFunc)();
    typedef bool(__stdcall* CS_IsRunningFunc)();
    typedef bool(__stdcall* CS_OnContextChanged)(int reason);
    typedef bool(__stdcall* CS_IsSettingsReady)();
    typedef void(__stdcall* CS_GetMetaFunc)(int kind, char* buffer, int bufferSize);
    typedef void(__stdcall* CS_GetMetaWFunc)(int kind, wchar_t* buffer, int bufferSize);
    typedef int(__stdcall* CS_GetPluginTypeFunc)();
    typedef void(__stdcall* CS_CheckDeviceFunc)();
    typedef bool(__stdcall* CS_IsCheckingAllowedFunc)();
    typedef uint64_t(__stdcall* CS_GetSharedPointerFunc)();
}

class CSharpPluginWrapper : public osr::IOpenSRPlugin {
public:
    CSharpPluginWrapper();
    ~CSharpPluginWrapper();

    // Loads the C++/CLI bridge + C# plugin from the folder containing THIS
    // DLL (the host loaded us from the plugin folder - no OpenSRContext
    // needed). Called by CreatePlugin(), because the host queries metadata
    // (GetPluginName, ...) immediately after createPlugin() and BEFORE Init.
    bool LoadManagedPart();

    bool Init(OpenSRContext* ctx, void* buffers, const wchar_t* pluginPath) override;
    bool Start() override;
    void Stop() override;
    void Shutdown() override;

    void Pause() override;
    void Resume() override;

    bool IsRunning() const override;

    void GetPluginName(char* buffer, size_t bufferSize) const override;
    void GetAuthor(char* buffer, size_t bufferSize) const override;
    void GetVersion(char* buffer, size_t bufferSize) const override;
    void GetLicenseType(char* buffer, size_t bufferSize) const override;
    void GetDescription(char* buffer, size_t bufferSize) const override;
    void GetTargetName(char* buffer, size_t bufferSize) const override;
    void GetTargetProcessName(char* buffer, size_t bufferSize) const override;
    void GetPackageName(char* buffer, size_t bufferSize) const override;

    int GetType() override;

    void GetPluginGroupName(wchar_t* buffer, size_t bufferSize) const override;

    bool IsCheckingAllowed() override;
    void CheckDevice() override;
    uint64_t GetSharedPointer() override;
    void GetUITabName(char* buffer, size_t bufferSize) const override;
    void GetSettingsTabName(char* buffer, size_t bufferSize) const override;

    void OnContextChanged(osr::OpenSRContextChange reason) override;

    bool IsSettingsReady() override;

private:
    void CopyMeta(int kind, char* buffer, size_t bufferSize) const;
    OpenSRContext* m_pContext;
#ifdef PLUGIN_TYPE_OUT
    OpenSRBuffersOUT* m_pBuffersOut = nullptr;
#else
    OpenSRBuffersIN* m_pBuffersIn = nullptr;
#endif
    HMODULE hModule;
    
    CS_InitFunc csb_init = nullptr;
    CS_StartFunc csb_start = nullptr;
    CS_PauseFunc csb_pause = nullptr;
    CS_ResumeFunc csb_resume = nullptr;
    CS_StopFunc csb_stop = nullptr;
    CS_ShutdownFunc csb_shutdown = nullptr;
    CS_InitializeBridgeFunc cs_initialize_bridge = nullptr;
    CS_IsRunningFunc csb_isRunning = nullptr;
    CS_OnContextChanged csb_onContextChanged = nullptr;
    CS_IsSettingsReady csb_isSettingsReady = nullptr;
    CS_GetMetaFunc csb_getMeta = nullptr;
    CS_GetMetaWFunc csb_getMetaW = nullptr;
    CS_GetPluginTypeFunc csb_getPluginType = nullptr;
    CS_CheckDeviceFunc csb_checkDevice = nullptr;
    CS_IsCheckingAllowedFunc csb_isCheckingAllowed = nullptr;
    CS_GetSharedPointerFunc csb_getSharedPointer = nullptr;

    std::atomic<bool> m_isPluginRunning = false;
    std::atomic<bool> m_stopRequested = false;

    std::atomic<bool> m_isPluginPaused = false;
    std::mutex m_pauseMutex;
    std::condition_variable m_pauseCv;
};

