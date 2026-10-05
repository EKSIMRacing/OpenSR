#pragma once

#include "IOpenSRPlugin.h"
#include "OpenSRBuffers.h"

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <cmath>

#define _TARGET_APP "notepad.exe"

class NotepadExamplePlugin : public osr::IOpenSRPlugin
{
public:

    NotepadExamplePlugin();
    ~NotepadExamplePlugin();

    // =========================================================
    // OpenSR Lifecycle
    // =========================================================

    bool Init(
        OpenSRContext* context,
        void* inBuf,
        const wchar_t* pluginPath) override;

    bool Start() override;

    void Stop() override;

    void Shutdown() override;

    void Pause() override
    {
        m_isPluginPaused = true;
    }

    void Resume() override
    {
        m_isPluginPaused = false;
        m_pauseCv.notify_one();
    }

    bool IsRunning() const override
    {
        return m_isPluginRunning;
    }

    void OnContextChanged(
        osr::OpenSRContextChange reason) override;

    // =========================================================
    // Mandatory Metadata
    // =========================================================

    void GetPluginName(
        char* buffer,
        size_t bufferSize) const override
    {
        strcpy_s(
            buffer,
            bufferSize,
            "Notepad Example Plugin");
    }

    void GetAuthor(
        char* buffer,
        size_t bufferSize) const override
    {
        strcpy_s(
            buffer,
            bufferSize,
            "AuthorName");
    }

    void GetVersion(
        char* buffer,
        size_t bufferSize) const override
    {
        strcpy_s(
            buffer,
            bufferSize,
            "1.0");
    }

    void GetLicenseType(
        char* buffer,
        size_t bufferSize) const override
    {
        strcpy_s(
            buffer,
            bufferSize,
            "MIT");
    }

    void GetDescription(
        char* buffer,
        size_t bufferSize) const override
    {
        strcpy_s(
            buffer,
            bufferSize,
            "Example OpenSR GAME plugin using Notepad.exe as target process and generating fake telemetry.");
    }

    void GetTargetName(
        char* buffer,
        size_t bufferSize) const override
    {
        strcpy_s(
            buffer,
            bufferSize,
            "Notepad");
    }

    void GetTargetProcessName(
        char* buffer,
        size_t bufferSize) const override
    {
        strcpy_s(
            buffer,
            bufferSize,
            _TARGET_APP);
    }

    void GetPackageName(
        char* buffer,
        size_t bufferSize) const override
    {
        strncpy_s(
            buffer,
            bufferSize,
            "com.authorname.example.notepadplugin",
            _TRUNCATE);
    }

    void GetPluginGroupName(
        wchar_t* buffer,
        size_t bufferSize) const override
    {
        wcscpy_s(
            buffer,
            bufferSize,
            L"");
    }

    int GetType() override
    {
        return GAME_PLUGIN_TYPE;
    }

private:

    // =========================================================
    // Internal Runtime
    // =========================================================

    void WorkerThread();

    void CheckForPause();

    void GenerateFakeTelemetry(
        OutSimData& data,
        float timeSec);

private:

    // =========================================================
    // OpenSR Runtime References
    // =========================================================

    OpenSRContext* m_pContext = nullptr;

    OpenSRBuffersIN* m_pBufferIn = nullptr;

    std::wstring m_pluginPath;

    // =========================================================
    // Threading
    // =========================================================

    std::thread workerThread;

    std::mutex lifecycleMutex;

    std::mutex m_pauseMutex;

    std::condition_variable m_pauseCv;

    // =========================================================
    // Runtime State
    // =========================================================

    std::atomic<bool> m_stopRequested = false;

    std::atomic<bool> m_isPluginRunning = false;

    std::atomic<bool> m_isPluginPaused = false;
};