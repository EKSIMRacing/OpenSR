// IPluginUI.h
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

#include "OpenSRContext.h"
#include <Windows.h>

// The plugin must implement a class that inherits from this interface.
class IPluginUI {
public:
    virtual ~IPluginUI() = default;

    // Called once with the HWND of the child window the plugin can use.
    // The plugin should return its desired initial height.
    virtual BOOL Initialize(HWND hWindow, OpenSRContext* context, void* buffersOut, const wchar_t* pluginFolderPath, const uint64_t sharedBuffer = 0) = 0;

    // Called when the settings view is being destroyed. return TRUE if settings has changed
    virtual BOOL Shutdown() = 0;

    // Return TRUE if this settings UI should be treated as a dialog
     // (i.e. host will run IsDialogMessage on its behalf).
    virtual BOOL WantsDialogMessages() const { return FALSE; }

};

// The plugin DLL must export these two C-style functions.
extern "C" {
    __declspec(dllexport) IPluginUI* CreatePluginUI();
    __declspec(dllexport) void DestroyPluginUI(IPluginUI* pUI);
}