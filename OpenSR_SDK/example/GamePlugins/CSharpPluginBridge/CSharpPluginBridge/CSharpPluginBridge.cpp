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

// CSharpPluginBridge.cpp
#include "CSharpPluginBridge.h"
#include <vcclr.h>
#include <msclr\auto_gcroot.h>

// Reference the C# DLL
//#using "..\Documents\OpenSR\OutPlugins\CSharpPlugin\CSharpOutPlugin.dll" as_friend

// Use the C# namespace
using namespace System;
#ifdef PLUGIN_TYPE_OUT
using namespace CSharpOutPlugin;
#else
using namespace CSharpInPlugin;
#endif
using namespace System::Reflection;

msclr::auto_gcroot<Type^> pluginType;
msclr::auto_gcroot<MethodInfo^> initMethod;
msclr::auto_gcroot<MethodInfo^> startMethod;
msclr::auto_gcroot<MethodInfo^> stopMethod;
msclr::auto_gcroot<MethodInfo^> shutdownMethod;
msclr::auto_gcroot<MethodInfo^> pauseMethod;
msclr::auto_gcroot<MethodInfo^> resumeMethod;
msclr::auto_gcroot<MethodInfo^> isRunningMethod;
msclr::auto_gcroot<MethodInfo^> onContextChanged;
msclr::auto_gcroot<MethodInfo^> isSettingsReady;
// optional methods - nullptr when the C# plugin doesn't provide them
msclr::auto_gcroot<MethodInfo^> getMeta;
msclr::auto_gcroot<MethodInfo^> getPluginType;
msclr::auto_gcroot<MethodInfo^> checkDevice;
msclr::auto_gcroot<MethodInfo^> isCheckingAllowed;
msclr::auto_gcroot<MethodInfo^> getSharedPointer;

namespace CSharpPluginBridge {
    // Wrapper class implementation
    bool PluginWrapper::InitializeBridge() {
        try {
            // Get the full path of this C++/CLI DLL itself.
            String^ bridgePath = Assembly::GetExecutingAssembly()->Location;
            String^ pluginDirectory = System::IO::Path::GetDirectoryName(bridgePath);
#ifdef PLUGIN_TYPE_OUT
            String^ csharpPluginPath = System::IO::Path::Combine(pluginDirectory, "CSharpOutPlugin.dll");
#else
            String^ csharpPluginPath = System::IO::Path::Combine(pluginDirectory, "CSharpInPlugin.dll");
#endif
            // Manually load the C# assembly from its path
           // Assembly^ pluginAssembly = Assembly::LoadFrom(csharpPluginPath);

            // Load through the byte-stream API so a DLL updated on disk is
           // re-loaded on the next Init (Assembly.LoadFrom caches by path for
           // the whole life of the CLR, which never dies under .NET Framework).
         /*   System::IO::FileStream^ fs = gcnew System::IO::FileStream(
                csharpPluginPath, System::IO::FileMode::Open, System::IO::FileAccess::Read);
            Assembly^ pluginAssembly = AppDomain::CurrentDomain->Load(fs);
            fs->Close();*/
            array<System::Byte>^ bytes = System::IO::File::ReadAllBytes(csharpPluginPath);
            Assembly^ pluginAssembly = Assembly::Load(bytes);

            // Find the public static class "Plugin" inside the loaded assembly
#ifdef PLUGIN_TYPE_OUT
            pluginType.reset(pluginAssembly->GetType("CSharpOutPlugin.Plugin"));
#else
            pluginType.reset(pluginAssembly->GetType("CSharpInPlugin.Plugin"));
#endif
            if (pluginType.get() == nullptr) {
                // Log error: could not find the Plugin class
                System::Diagnostics::Debug::WriteLine("Error: could not find the Plugin class or dll not present.");
                return false;
            }

            // Find all the methods we need and store them
            initMethod.reset(pluginType->GetMethod("Init"));
            startMethod.reset(pluginType->GetMethod("Start"));
            stopMethod.reset(pluginType->GetMethod("Stop"));
            shutdownMethod.reset(pluginType->GetMethod("Shutdown"));

            pauseMethod.reset(pluginType->GetMethod("Pause"));
            resumeMethod.reset(pluginType->GetMethod("Resume"));
            isRunningMethod.reset(pluginType->GetMethod("IsRunning"));
            
            onContextChanged.reset(pluginType->GetMethod("OnContextChanged"));
            isSettingsReady.reset(pluginType->GetMethod("IsSettingsReady"));

            // optional - missing on older C# plugins, the wrapper falls back.
            // NOTE: the C# method is named GetPluginType, NOT GetType, on purpose:
            // Type::GetMethod("GetType") would ambiguously match Object.GetType().
            getMeta.reset(pluginType->GetMethod("GetMeta"));
            getPluginType.reset(pluginType->GetMethod("GetPluginType"));
            checkDevice.reset(pluginType->GetMethod("CheckDevice"));
            isCheckingAllowed.reset(pluginType->GetMethod("IsCheckingAllowed"));
            getSharedPointer.reset(pluginType->GetMethod("GetSharedPointer"));

            // Check that we found all of them
            if (initMethod.get() == nullptr || startMethod.get() == nullptr ||
                stopMethod.get() == nullptr || shutdownMethod.get() == nullptr ||
                pauseMethod.get() == nullptr || resumeMethod.get() == nullptr ||
                isRunningMethod.get() == nullptr || onContextChanged.get() == nullptr || 
                isSettingsReady.get() == nullptr)
            {
                // Optional: Add logging to see which method was not found
                System::Diagnostics::Debug::WriteLine("Error: One or more plugin methods could not be found via reflection.");
                return false;
            }

            return true;
        }
        catch (Exception^ ex) {
            // Log the exception
            System::Diagnostics::Debug::WriteLine(ex->ToString());
            return false;
        }
    }

    bool PluginWrapper::CS_Init(void* ctxPtr, void* buffers, const wchar_t* path) {
        if (initMethod.get() == nullptr) {
            return false; // Not initialized
        }
        // Prepare parameters to call the method via reflection
        array<Object^>^ args = gcnew array<Object^>(3);
        args[0] = IntPtr(ctxPtr);
        args[1] = IntPtr(buffers);
        args[2] = gcnew String(path);

        // Invoke the static "Init" method we found earlier
        Object^ result = initMethod->Invoke(nullptr, args);
        return (bool)result;
    }

    bool PluginWrapper::CS_Start() {
        // Check if the Start method was found during initialization
        if (startMethod.get() == nullptr) {
            return false;
        }

        // Invoke the static "Start" method. It takes no parameters (nullptr for the first arg).
        Object^ result = startMethod->Invoke(nullptr, nullptr);
        return (bool)result;
    }

    void PluginWrapper::CS_Pause() {
        if (pauseMethod.get() != nullptr) {
            pauseMethod->Invoke(nullptr, nullptr);
        }
    }

    void PluginWrapper::CS_Resume() {
        if (resumeMethod.get() != nullptr) {
            resumeMethod->Invoke(nullptr, nullptr);
        }
    }

    bool PluginWrapper::CS_IsRunning() {
        if (isRunningMethod.get() == nullptr) return false;
        Object^ result = isRunningMethod->Invoke(nullptr, nullptr);
        return (bool)result;
    }

    void PluginWrapper::CS_Stop() {
        // Check if the Stop method was found
        if (stopMethod.get() == nullptr) {
            return; // Or handle error
        }

        // Invoke the static "Stop" method. It has a void return type.
        stopMethod->Invoke(nullptr, nullptr);
    }

    bool PluginWrapper::CS_Shutdown() {
        // Check if the Shutdown method was found
        if (shutdownMethod.get() == nullptr) {
            return false;
        }

        // Invoke the static "Shutdown" method.
        Object^ result = shutdownMethod->Invoke(nullptr, nullptr);
        return (bool)result;
    }

    void PluginWrapper::CS_OnContextChanged(int reason)
    {
        if (onContextChanged.get() == nullptr) {
            return; // Or handle error
        }

        try {
            array<Object^>^ args = gcnew array<Object^>(1);
            args[0] = reason;
            onContextChanged->Invoke(nullptr, args);
        }
        catch (Exception^ ex) {
            System::Diagnostics::Debug::WriteLine(
                "Exception in OnContextChanged: " + ex->ToString());
        }
    }

    bool PluginWrapper::CS_IsSettingsReady() {
        // Check if the isSettingsReady method was found
        if (isSettingsReady.get() == nullptr) {
            return true; // default is ready
        }

        // Invoke the static "isSettingsReady" method.
        Object^ result = isSettingsReady->Invoke(nullptr, nullptr);
        return (bool)result;
    }

  
    void PluginWrapper::CS_GetMeta(int kind, char* buffer, int bufferSize) {
        if (bufferSize <= 0) return;
        buffer[0] = 0;
        if (getMeta.get() == nullptr) return;
        try {
            array<Object^>^ args = gcnew array<Object^>(1);
            args[0] = kind;
            Object^ result = getMeta->Invoke(nullptr, args);
            String^ s = (result == nullptr) ? String::Empty : result->ToString();
            array<Byte>^ bytes = System::Text::Encoding::UTF8->GetBytes(s); // UTF-8 into char*
            int n = bytes->Length < (bufferSize - 1) ? bytes->Length : (bufferSize - 1);
            for (int i = 0; i < n; i++) buffer[i] = (char)bytes[i];
            buffer[n] = 0;
        }
        catch (Exception^ ex) {
            System::Diagnostics::Debug::WriteLine("CS_GetMeta: " + ex->ToString());
            buffer[0] = 0;
        }
    }

    void PluginWrapper::CS_GetMetaW(int kind, wchar_t* buffer, int bufferSize) {
        if (bufferSize <= 0) return;
        buffer[0] = 0;
        if (getMeta.get() == nullptr) return;
        try {
            array<Object^>^ args = gcnew array<Object^>(1);
            args[0] = kind;
            Object^ result = getMeta->Invoke(nullptr, args);
            String^ s = (result == nullptr) ? String::Empty : result->ToString();
            int n = s->Length < (bufferSize - 1) ? s->Length : (bufferSize - 1);
            for (int i = 0; i < n; i++) buffer[i] = s[i];
            buffer[n] = 0;
        }
        catch (Exception^ ex) {
            System::Diagnostics::Debug::WriteLine("CS_GetMetaW: " + ex->ToString());
            buffer[0] = 0;
        }
    }

    int PluginWrapper::CS_GetPluginType() {
        if (getPluginType.get() == nullptr) return -1;
        try {
            Object^ result = getPluginType->Invoke(nullptr, nullptr);
            return (result == nullptr) ? -1 : (int)result;
        }
        catch (...) { return -1; }
    }

    void PluginWrapper::CS_CheckDevice() {
        if (checkDevice.get() == nullptr) return;
        try { checkDevice->Invoke(nullptr, nullptr); }
        catch (Exception^ ex) { System::Diagnostics::Debug::WriteLine("CS_CheckDevice: " + ex->ToString()); }
    }

    bool PluginWrapper::CS_IsCheckingAllowed() {
        if (isCheckingAllowed.get() == nullptr) return false;
        try {
            Object^ result = isCheckingAllowed->Invoke(nullptr, nullptr);
            return (result != nullptr) && (bool)result;
        }
        catch (...) { return false; }
    }

    uint64_t PluginWrapper::CS_GetSharedPointer() {
        if (getSharedPointer.get() == nullptr) return 0;
        try {
            Object^ result = getSharedPointer->Invoke(nullptr, nullptr);
            return (result == nullptr) ? 0 : (uint64_t)result;
        }
        catch (...) { return 0; }
    }

    // Native C exports implementation

    bool __stdcall CS_InitializeBridge() {
        return PluginWrapper::InitializeBridge();
    }

    bool __stdcall CS_Init(void* ctxPtr, void* buffers, const wchar_t* path) {
        try
        {
            // The original call is now safely inside the try block
            return PluginWrapper::CS_Init(ctxPtr, buffers, path);
        }
        catch (System::Exception^ ex)
        {
            System::Diagnostics::Debug::WriteLine(ex->ToString());
            return false;
        }
    }

    bool __stdcall CS_Start() {
        try {
            return PluginWrapper::CS_Start();
        }
        catch (Exception^ ex) {
            System::Diagnostics::Debug::WriteLine("Exception in CS_Start: " + ex->ToString());
            return false;
        }
    }

    void __stdcall CS_Pause() {
        try { PluginWrapper::CS_Pause(); }
        catch (Exception^ ex) { System::Diagnostics::Debug::WriteLine("Exception in CS_Pause: " + ex->ToString()); }
    }

    void __stdcall CS_Resume() {
        try { PluginWrapper::CS_Resume(); }
        catch (Exception^ ex) { System::Diagnostics::Debug::WriteLine("Exception in CS_Resume: " + ex->ToString()); }
    }

    bool __stdcall CS_IsRunning() {
        try { return PluginWrapper::CS_IsRunning(); }
        catch (Exception^ ex) { System::Diagnostics::Debug::WriteLine("Exception in CS_IsRunning: " + ex->ToString()); return false; }
    }

    void __stdcall CS_Stop() {
        try {
            PluginWrapper::CS_Stop();
        }
        catch (Exception^ ex) {
            System::Diagnostics::Debug::WriteLine("Exception in CS_Stop: " + ex->ToString());
        }
    }

    bool __stdcall CS_Shutdown() {
        try {
            return PluginWrapper::CS_Shutdown();
        }
        catch (Exception^ ex) {
            System::Diagnostics::Debug::WriteLine("Exception in CS_Shutdown: " + ex->ToString());
            return false;
        }
    }

    void __stdcall CS_OnContextChanged(int reason)
    {
        try {
            PluginWrapper::CS_OnContextChanged(reason);
        }
        catch (Exception^ ex) {
            System::Diagnostics::Debug::WriteLine(
                "Exception in CS_OnContextChanged: " + ex->ToString());
        }
    }

    bool __stdcall CS_IsSettingsReady() {
        try {
            return PluginWrapper::CS_IsSettingsReady();
        }
        catch (Exception^ ex) {
            System::Diagnostics::Debug::WriteLine("Exception in CS_IsSettingsReady: " + ex->ToString());
            return false;
        }
    }

    void __stdcall CS_GetMeta(int kind, char* buffer, int bufferSize) {
        try { PluginWrapper::CS_GetMeta(kind, buffer, bufferSize); }
        catch (Exception^ ex) {
            System::Diagnostics::Debug::WriteLine("Exception in CS_GetMeta: " + ex->ToString());
            if (bufferSize > 0) buffer[0] = 0;
        }
    }

    void __stdcall CS_GetMetaW(int kind, wchar_t* buffer, int bufferSize) {
        try { PluginWrapper::CS_GetMetaW(kind, buffer, bufferSize); }
        catch (Exception^ ex) {
            System::Diagnostics::Debug::WriteLine("Exception in CS_GetMetaW: " + ex->ToString());
            if (bufferSize > 0) buffer[0] = 0;
        }
    }

    int __stdcall CS_GetPluginType() {
        try { return PluginWrapper::CS_GetPluginType(); }
        catch (...) { return -1; }
    }

    void __stdcall CS_CheckDevice() {
        try { PluginWrapper::CS_CheckDevice(); }
        catch (Exception^ ex) { System::Diagnostics::Debug::WriteLine("Exception in CS_CheckDevice: " + ex->ToString()); }
    }

    bool __stdcall CS_IsCheckingAllowed() {
        try { return PluginWrapper::CS_IsCheckingAllowed(); }
        catch (...) { return false; }
    }

    uint64_t __stdcall CS_GetSharedPointer() {
        try { return PluginWrapper::CS_GetSharedPointer(); }
        catch (...) { return 0; }
    }
}
