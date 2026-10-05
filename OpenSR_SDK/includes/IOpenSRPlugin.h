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

#include <string>
#include <functional>
//#include <future>
#include "OpenSRContext.h"

#define OpenSR_PLUGIN_API extern "C" __declspec(dllexport)

#define PLUGIN_TYPE_NONE -1
#define GAME_PLUGIN_TYPE 0
#define OUT_PLUGIN_TYPE 1

// Check if we are compiling with VS2010 (Ver 1600) or older
#if defined(_MSC_VER) && (_MSC_VER <= 1600)
    // VS2010: Does not support "= default"
    #define OSR_DTOR_BODY {}
    #define OSR_NOEXCEPT
#else
    // Modern VS (2015+): Use modern C++ standards
    #define OSR_DTOR_BODY = default;
    #define OSR_NOEXCEPT noexcept
#endif 
namespace osr {
    enum OpenSRContextChange
    {
        ProfilePathChanged,
        SettingsChanged,
        FullContextReloaded,
        CheckDevice,
        Init
    };


    class IOpenSRPlugin {
    public: 
        virtual ~IOpenSRPlugin() OSR_DTOR_BODY

        // Called immediately after loading the DLL to provide setup and shared memory access
        virtual bool Init(OpenSRContext* context, void* buffers, const wchar_t* pluginPath) = 0;

        // Called to start processing. This should be a NON-BLOCKING call.
        // The plugin should start its work in a separate thread.
        virtual bool Start() = 0;

        // Called to request the plugin to pause its processing
        virtual void Pause() = 0;

        // Called to request the plugin to resume its processing
        virtual void Resume() = 0;

        // Returns true if the plugin's main processing loop is active.
        // This is used by the manager to detect if the plugin has terminated unexpectedly.
        virtual bool IsRunning() const = 0;

        // Called to request the plugin to stop its processing (join threads)
        virtual void Stop() = 0;

        // Called before unloading to cleanup
        virtual void Shutdown() = 0;

        // Returns generic pointer. 
        // Host sees void*, Plugin UI casts it to SharedData* later.
        virtual uint64_t GetSharedPointer() {
            return 0;
        }

        // metadata
        virtual void GetPackageName(char* buffer, size_t bufferSize) const = 0;   // e.g. "com.author.outplugin.slipro" or "com.company.gameplugin.rfactor2"
        virtual void GetPluginName(char* buffer, size_t bufferSize) const = 0;
        virtual void GetAuthor(char* buffer, size_t bufferSize) const = 0;
        virtual void GetVersion(char* buffer, size_t bufferSize) const = 0;
        virtual void GetLicenseType(char* buffer, size_t bufferSize) const = 0;
        virtual void GetDescription(char* buffer, size_t bufferSize) const = 0;
        virtual void GetTargetName(char* buffer, size_t bufferSize) const = 0;
        virtual void GetTargetProcessName(char* buffer, size_t bufferSize) const = 0;

        virtual void GetPluginGroupName(wchar_t* buffer, size_t bufferSize) const = 0;

        virtual bool IsBackend() const { return false; }

        virtual int GetType() = 0;

        virtual bool IsCheckingAllowed() { return false; }

        virtual void CheckDevice() {}

        // reload notification
        virtual void OnContextChanged(osr::OpenSRContextChange reason)
        {
            // Default: do nothing
        }

        // wait settings to be ready before loading, parsing and rendering xml
        virtual bool IsSettingsReady() { return true; }

        // Optional: Set UI TAB Name (Default 'Plugin')
        virtual void GetUITabName(char* buffer, size_t bufferSize) const
        {
            strncpy_s(buffer, bufferSize, "Plugin", _TRUNCATE);
        }

        // Optional: Set Settings TAB Name (Default 'Settings')
        virtual void GetSettingsTabName(char* buffer, size_t bufferSize) const
        {
            strncpy_s(buffer, bufferSize, "Settings", _TRUNCATE);
        }

    };

}

// Mandatory exported factory function for the loader
typedef osr::IOpenSRPlugin* (*CreatePluginFunc)();
typedef void (*DestroyPluginFunc)(osr::IOpenSRPlugin*);

OpenSR_PLUGIN_API osr::IOpenSRPlugin* CreatePlugin();
OpenSR_PLUGIN_API void DestroyPlugin(osr::IOpenSRPlugin* plugin);

