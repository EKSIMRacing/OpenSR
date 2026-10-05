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
#include "CSharpPluginWrapper.h"
#include <cstring>

HMODULE g_hSelf = nullptr;
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
        g_hSelf = hinst;   // this wrapper DLL's own handle
    return TRUE;
}

CSharpPluginWrapper::CSharpPluginWrapper()
    : m_pContext(nullptr),
    hModule(nullptr),
    csb_init(nullptr),
    csb_start(nullptr),
    csb_pause(nullptr),
    csb_resume(nullptr),
    csb_isRunning(nullptr),
    csb_stop(nullptr),
    csb_shutdown(nullptr),
    cs_initialize_bridge(nullptr),
    csb_isSettingsReady(nullptr),
    csb_getMeta(nullptr),
    csb_getMetaW(nullptr),
    csb_getPluginType(nullptr),
    csb_checkDevice(nullptr),
    csb_isCheckingAllowed(nullptr),
    csb_getSharedPointer(nullptr) {}

CSharpPluginWrapper::~CSharpPluginWrapper()
{
    Shutdown(); // idempotent: covers DestroyPlugin without an explicit Shutdown
}

// Resolves the plugin folder from THIS DLL's own path, loads the C++/CLI
// bridge, starts the CLR and loads the C# plugin. No OpenSRContext needed.
bool CSharpPluginWrapper::LoadManagedPart() {
    if (hModule)
        return true; // already loaded

    wchar_t selfPath[MAX_PATH] = { 0 };
    if (!GetModuleFileNameW(g_hSelf, selfPath, MAX_PATH))
        return false;

    // <plugin folder>\CSharpPluginBridge.dll  (we ARE in the plugin folder)
    std::wstring bridgePath(selfPath);
    size_t slash = bridgePath.find_last_of(L"\\");
    if (slash == std::wstring::npos)
        return false;
    bridgePath = bridgePath.substr(0, slash + 1);
#ifdef PLUGIN_TYPE_OUT
    bridgePath += L"CSharpPluginBridge.dll";
#else
    bridgePath += L"CSharpInPluginBridge.dll";
#endif

    hModule = LoadLibraryW(bridgePath.c_str());
    if (!hModule)
        return false;

    cs_initialize_bridge = (CS_InitializeBridgeFunc)GetProcAddress(hModule, "CS_InitializeBridge");
    if (!cs_initialize_bridge) {
        FreeLibrary(hModule);
        hModule = nullptr;
        return false; // This bridge is invalid
    }

    // This starts the CLR and loads the C# plugin assembly
    if (!cs_initialize_bridge()) {
        FreeLibrary(hModule);
        hModule = nullptr;
        return false; // The .NET part failed to initialize
    }

    csb_init = (CS_InitFunc)GetProcAddress(hModule, "CS_Init");
    csb_start = (CS_StartFunc)GetProcAddress(hModule, "CS_Start");
    csb_stop = (CS_StopFunc)GetProcAddress(hModule, "CS_Stop");
    csb_shutdown = (CS_ShutdownFunc)GetProcAddress(hModule, "CS_Shutdown");
    csb_pause = (CS_PauseFunc)GetProcAddress(hModule, "CS_Pause");
    csb_resume = (CS_ResumeFunc)GetProcAddress(hModule, "CS_Resume");
    csb_isRunning = (CS_IsRunningFunc)GetProcAddress(hModule, "CS_IsRunning");
    csb_onContextChanged = (CS_OnContextChanged)GetProcAddress(hModule, "CS_OnContextChanged");
    csb_isSettingsReady = (CS_IsSettingsReady)GetProcAddress(hModule, "CS_IsSettingsReady");
    // optional - newer bridge / C# plugins
    csb_getMeta = (CS_GetMetaFunc)GetProcAddress(hModule, "CS_GetMeta");
    csb_getMetaW = (CS_GetMetaWFunc)GetProcAddress(hModule, "CS_GetMetaW");
    csb_getPluginType = (CS_GetPluginTypeFunc)GetProcAddress(hModule, "CS_GetPluginType");
    csb_checkDevice = (CS_CheckDeviceFunc)GetProcAddress(hModule, "CS_CheckDevice");
    csb_isCheckingAllowed = (CS_IsCheckingAllowedFunc)GetProcAddress(hModule, "CS_IsCheckingAllowed");
    csb_getSharedPointer = (CS_GetSharedPointerFunc)GetProcAddress(hModule, "CS_GetSharedPointer");

    // mandatory
    if (!csb_init || !csb_start || !csb_stop || !csb_shutdown || !csb_pause || !csb_resume || !csb_isRunning)
    {
        FreeLibrary(hModule);
        hModule = nullptr;
        return false;
    }
    return true;
}

bool CSharpPluginWrapper::Init(OpenSRContext* ctx, void* buffers, const wchar_t* pluginPath) {
    if (!ctx || !buffers || !pluginPath) return false;
    if (!LoadManagedPart())
        return false; // safety: CreatePlugin already guarantees this
    m_pContext = ctx;
#ifdef PLUGIN_TYPE_OUT
    m_pBuffersOut = static_cast<OpenSRBuffersOUT*>(buffers);
#else
    m_pBuffersIn = static_cast<OpenSRBuffersIN*>(buffers);
#endif
    if (!csb_init(ctx, buffers, pluginPath))
        return false;
    m_isPluginRunning = true;
    return true;
}

//bool CSharpPluginWrapper::Init(OpenSRContext* ctx, void* buffers, const wchar_t* pluginPath) {
//    if (!ctx || !buffers || !pluginPath) return false;
//    m_pContext = ctx;
//#ifdef PLUGIN_TYPE_OUT
//    m_pBuffersOut = static_cast<OpenSRBuffersOUT*>(buffers);
//#else
//    m_pBuffersIn = static_cast<OpenSRBuffersIN*>(buffers);
//#endif
//    std::wstring csDllPath =
//        m_pContext->osrDocFolder
//        + std::wstring(L"\\")
//        + std::wstring(pluginPath)
//#ifdef PLUGIN_TYPE_OUT
//        + L"\\CSharpPluginBridge.dll";
//#else
//        + L"\\CSharpInPluginBridge.dll";
//#endif
//
//    hModule = LoadLibraryW(csDllPath.c_str());
//    if (!hModule) return false;
//
//    // First, get the address of the initialization function
//    cs_initialize_bridge = (CS_InitializeBridgeFunc)GetProcAddress(hModule, "CS_InitializeBridge");
//    if (!cs_initialize_bridge) {
//        FreeLibrary(hModule);
//        return false; // This bridge is invalid
//    }
//
//    // Now, call it. This will safely start the CLR and load the C# DLL.
//    if (!cs_initialize_bridge()) {
//        FreeLibrary(hModule);
//        return false; // The .NET part failed to initialize
//    }
//
//    csb_init = (CS_InitFunc)GetProcAddress(hModule, "CS_Init");
//    csb_start = (CS_StartFunc)GetProcAddress(hModule, "CS_Start");
//    csb_stop = (CS_StopFunc)GetProcAddress(hModule, "CS_Stop");
//    csb_shutdown = (CS_ShutdownFunc)GetProcAddress(hModule, "CS_Shutdown");
//    csb_pause = (CS_PauseFunc)GetProcAddress(hModule, "CS_Pause");
//    csb_resume = (CS_ResumeFunc)GetProcAddress(hModule, "CS_Resume");
//    csb_isRunning = (CS_IsRunningFunc)GetProcAddress(hModule, "CS_IsRunning");
//    csb_onContextChanged = (CS_OnContextChanged)GetProcAddress(hModule, "CS_OnContextChanged");
//
//    csb_isSettingsReady = (CS_IsSettingsReady)GetProcAddress(hModule, "CS_IsSettingsReady");
//    // optional - absent in older bridges/C# plugins, wrapper falls back
//    csb_getMeta = (CS_GetMetaFunc)GetProcAddress(hModule, "CS_GetMeta");
//    csb_getMetaW = (CS_GetMetaWFunc)GetProcAddress(hModule, "CS_GetMetaW");
//    csb_getPluginType = (CS_GetPluginTypeFunc)GetProcAddress(hModule, "CS_GetPluginType");
//    csb_checkDevice = (CS_CheckDeviceFunc)GetProcAddress(hModule, "CS_CheckDevice");
//    csb_isCheckingAllowed = (CS_IsCheckingAllowedFunc)GetProcAddress(hModule, "CS_IsCheckingAllowed");
//    csb_getSharedPointer = (CS_GetSharedPointerFunc)GetProcAddress(hModule, "CS_GetSharedPointer");
//
//    // mandatory
//    if (!csb_init || !csb_start || !csb_stop || !csb_shutdown || !csb_pause || !csb_resume || !csb_isRunning)
//    {
//        FreeLibrary(hModule);
//        hModule = nullptr;
//        return false;
//    }
//
//    if (!csb_init(ctx, buffers, pluginPath))
//    {
//        FreeLibrary(hModule);
//        hModule = nullptr;
//        return false;
//    }
//    m_isPluginRunning = true;
//    return true;
//}
 

bool CSharpPluginWrapper::Start() {
    return csb_start ? csb_start() : false;
}

void CSharpPluginWrapper::Pause() {
    if (csb_pause) csb_pause();
}

void CSharpPluginWrapper::Resume() {
    if (csb_resume) csb_resume();
}

void CSharpPluginWrapper::Stop() {
    if (csb_stop) csb_stop();
}

void CSharpPluginWrapper::Shutdown() {
    if (csb_shutdown) csb_shutdown();
    // invalidate every callback BEFORE releasing the module: a late host call
    // (IsRunning, GetMeta, ...) must never jump into freed code
    csb_init = nullptr;
    csb_start = nullptr;
    csb_pause = nullptr;
    csb_resume = nullptr;
    csb_isRunning = nullptr;
    csb_stop = nullptr;
    csb_shutdown = nullptr;
    cs_initialize_bridge = nullptr;
    csb_isSettingsReady = nullptr;
    csb_getMeta = nullptr;
    csb_getMetaW = nullptr;
    csb_getPluginType = nullptr;
    csb_checkDevice = nullptr;
    csb_isCheckingAllowed = nullptr;
    csb_getSharedPointer = nullptr;
    if (hModule) {
        FreeLibrary(hModule);
        hModule = nullptr;
    }
    m_isPluginRunning = false;
}

bool CSharpPluginWrapper::IsRunning() const {
    return m_isPluginRunning && csb_isRunning ? csb_isRunning() : false;
}

//void CSharpPluginWrapper::GetPluginName(char* buffer, size_t bufferSize) const {
//    strcpy_s(buffer, bufferSize, "CSharp Example Plugin");
//}
//
//void CSharpPluginWrapper::GetAuthor(char* buffer, size_t bufferSize) const {
//    strcpy_s(buffer, bufferSize, "OpenSR Team");
//}
//
//void CSharpPluginWrapper::GetVersion(char* buffer, size_t bufferSize) const {
//    strcpy_s(buffer, bufferSize, "0.1");
//}
//void CSharpPluginWrapper::GetLicenseType(char* buffer, size_t bufferSize) const {
//    strcpy_s(buffer, bufferSize, "MIT");
//}
//void CSharpPluginWrapper::GetDescription(char* buffer, size_t bufferSize) const {
//    strcpy_s(buffer, bufferSize, "C# plugin example + wrapper DLL to get telemetery from OpenSR Server. Source code included in OpenSR SDK.");
//}
//void CSharpPluginWrapper::GetTargetName(char* buffer, size_t bufferSize) const {
//#ifdef PLUGIN_TYPE_OUT
//    strcpy_s(buffer, bufferSize, "");
//#else
//    strcpy_s(buffer, bufferSize, "Notepad"); // replace with the process name (.exe) of the game
//#endif
//}
//void CSharpPluginWrapper::GetTargetProcessName(char* buffer, size_t bufferSize) const {
//#ifdef PLUGIN_TYPE_OUT
//    strcpy_s(buffer, bufferSize, "");
//#else
//    strcpy_s(buffer, bufferSize, "Notepad.exe"); // replace with the process name (.exe) of the game
//#endif
//}
//
//void CSharpPluginWrapper::GetPackageName(char* buffer, size_t bufferSize) const {
//    strncpy_s(buffer, bufferSize, "com.zappadoc.gameplugin.cspluginexample", _TRUNCATE);
//}
//
//int CSharpPluginWrapper::GetType() {
//#ifdef PLUGIN_TYPE_OUT
//    return OUT_PLUGIN_TYPE;
//#else
//    return GAME_PLUGIN_TYPE;
//#endif
//}
//void CSharpPluginWrapper::GetPluginGroupName(wchar_t* buffer, size_t bufferSize) const {
//    wcscpy_s(buffer, bufferSize, L"");
//}

// ── metadata: all strings come from the C# plugin via CS_GetMeta ─────────────
// kind values MUST match Core.GetMeta() in Plugin.cs
enum CSMetaKind {
    CS_META_PACKAGE = 0,
    CS_META_NAME,
    CS_META_AUTHOR,
    CS_META_VERSION,
    CS_META_LICENSE,
    CS_META_DESCRIPTION,
    CS_META_TARGET_NAME,
    CS_META_TARGET_PROCESS_NAME,
    CS_META_UI_TAB,
    CS_META_SETTINGS_TAB,
    CS_META_GROUP_NAME
};

void CSharpPluginWrapper::CopyMeta(int kind, char* buffer, size_t bufferSize) const {
    if (csb_getMeta) csb_getMeta(kind, buffer, (int)bufferSize);
    else buffer[0] = 0;
}

void CSharpPluginWrapper::GetPackageName(char* buffer, size_t bufferSize) const { CopyMeta(CS_META_PACKAGE, buffer, bufferSize); }
void CSharpPluginWrapper::GetPluginName(char* buffer, size_t bufferSize) const { CopyMeta(CS_META_NAME, buffer, bufferSize); }
void CSharpPluginWrapper::GetAuthor(char* buffer, size_t bufferSize) const { CopyMeta(CS_META_AUTHOR, buffer, bufferSize); }
void CSharpPluginWrapper::GetVersion(char* buffer, size_t bufferSize) const { CopyMeta(CS_META_VERSION, buffer, bufferSize); }
void CSharpPluginWrapper::GetLicenseType(char* buffer, size_t bufferSize) const { CopyMeta(CS_META_LICENSE, buffer, bufferSize); }
void CSharpPluginWrapper::GetDescription(char* buffer, size_t bufferSize) const { CopyMeta(CS_META_DESCRIPTION, buffer, bufferSize); }
void CSharpPluginWrapper::GetTargetName(char* buffer, size_t bufferSize) const { CopyMeta(CS_META_TARGET_NAME, buffer, bufferSize); }
void CSharpPluginWrapper::GetTargetProcessName(char* buffer, size_t bufferSize) const { CopyMeta(CS_META_TARGET_PROCESS_NAME, buffer, bufferSize); }
void CSharpPluginWrapper::GetUITabName(char* buffer, size_t bufferSize) const { CopyMeta(CS_META_UI_TAB, buffer, bufferSize); }
void CSharpPluginWrapper::GetSettingsTabName(char* buffer, size_t bufferSize) const { CopyMeta(CS_META_SETTINGS_TAB, buffer, bufferSize); }

int CSharpPluginWrapper::GetType() {
    if (csb_getPluginType) {
        int t = csb_getPluginType();
        if (t != -1) return t;
    }
#ifdef PLUGIN_TYPE_OUT
    return OUT_PLUGIN_TYPE;
#else
    return GAME_PLUGIN_TYPE;
#endif
}
void CSharpPluginWrapper::GetPluginGroupName(wchar_t* buffer, size_t bufferSize) const {
    if (csb_getMetaW) csb_getMetaW(CS_META_GROUP_NAME, buffer, (int)bufferSize);
    else buffer[0] = 0;
}

bool CSharpPluginWrapper::IsCheckingAllowed() {
    return csb_isCheckingAllowed ? csb_isCheckingAllowed() : false;
}

void CSharpPluginWrapper::CheckDevice() {
    if (csb_checkDevice) csb_checkDevice();
}

uint64_t CSharpPluginWrapper::GetSharedPointer() {
    return csb_getSharedPointer ? csb_getSharedPointer() : 0;
}

void  CSharpPluginWrapper::OnContextChanged(osr::OpenSRContextChange reason) {
    if (csb_onContextChanged)
        csb_onContextChanged(static_cast<int>(reason));
}

bool  CSharpPluginWrapper::IsSettingsReady() {
    if (csb_isSettingsReady)
       return csb_isSettingsReady();

    return true; // ready by default
}

// === Factory exports ===
extern "C" __declspec(dllexport) osr::IOpenSRPlugin * CreatePlugin() {
    CSharpPluginWrapper* p = new CSharpPluginWrapper();
    // The host queries GetPluginName/GetPackageName/... right after this
    // call and BEFORE Init() - so the C# plugin must already be loaded here.
    if (!p->LoadManagedPart())
    {
        delete p;   // destructor is a no-op (nothing was loaded)
        return nullptr; // host logs and discards this DLL
    }
    return p;
}

extern "C" __declspec(dllexport) void DestroyPlugin(osr::IOpenSRPlugin * plugin) {
    if (plugin)
        delete plugin;
}
