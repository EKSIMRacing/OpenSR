#include "pch.h"

#include "NotepadExamplePlugin.h"

#include "OpenSRHelper.h"

#include <Windows.h>
#include <cmath>

// =========================================================
// Exported Factory Functions
// =========================================================

extern "C" __declspec(dllexport)
osr::IOpenSRPlugin * CreatePlugin()
{
    return new NotepadExamplePlugin();
}

extern "C" __declspec(dllexport)
void DestroyPlugin(osr::IOpenSRPlugin * plugin)
{
    if (plugin)
        delete plugin;
}

// =========================================================
// Constructor / Destructor
// =========================================================

NotepadExamplePlugin::NotepadExamplePlugin()
{
}

NotepadExamplePlugin::~NotepadExamplePlugin()
{
    Shutdown();
}

// =========================================================
// Init
// =========================================================

bool NotepadExamplePlugin::Init(
    OpenSRContext* context,
    void* inBuf,
    const wchar_t* pluginPath)
{
    // Store OpenSR runtime context.
    m_pContext = context;

    // Store writable telemetry buffers.
    m_pBufferIn =
        static_cast<OpenSRBuffersIN*>(inBuf);

    // Store plugin path.
    m_pluginPath = pluginPath;

    return true;
}

// =========================================================
// Start
// =========================================================

bool NotepadExamplePlugin::Start()
{
    std::lock_guard<std::mutex>
        lock(lifecycleMutex);

    if (!m_pContext)
        return false;

    // Reset runtime state.
    m_stopRequested = false;

    m_isPluginPaused = false;

    m_isPluginRunning = true;

    // Start telemetry thread.
    workerThread =
        std::thread(
            &NotepadExamplePlugin::WorkerThread,
            this);

    return true;
}

// =========================================================
// Stop
// =========================================================

void NotepadExamplePlugin::Stop()
{
    std::lock_guard<std::mutex>
        lock(lifecycleMutex);

    // Signal worker shutdown.
    m_stopRequested = true;

    // Wake paused thread if needed.
    if (m_isPluginPaused)
    {
        Resume();
    }

    // Wait for thread termination.
    if (workerThread.joinable())
    {
        workerThread.join();
    }

    m_isPluginRunning = false;
}

// =========================================================
// Shutdown
// =========================================================

void NotepadExamplePlugin::Shutdown()
{
    Stop();
}

// =========================================================
// Context Change
// =========================================================

void NotepadExamplePlugin::OnContextChanged(
    osr::OpenSRContextChange reason)
{
    // No runtime reload logic required
    // for this example plugin.
}

// =========================================================
// Pause Synchronization
// =========================================================

void NotepadExamplePlugin::CheckForPause()
{
    std::unique_lock<std::mutex>
        lock(m_pauseMutex);

    m_pauseCv.wait(
        lock,
        [this]
        {
            return !m_isPluginPaused;
        });
}

// =========================================================
// Main Worker Thread
// =========================================================

void NotepadExamplePlugin::WorkerThread()
{
    if (!m_pContext ||
        !m_pBufferIn ||
        !m_pBufferIn->outSimDataIN)
    {
        return;
    }

    // Mark telemetry available.
    m_pBufferIn
        ->outSimDataIN
        ->mPacketHeader
        .reportAvailable = 1;

    auto startTime =
        std::chrono::steady_clock::now();

    while (!m_stopRequested &&
        m_pContext &&
        m_pContext->isAppRunning)
    {
        // Validate target process.
        if (!OpenSRHelper::IsProcessRunningByPIDAndName(
            m_pContext->targetGamePId,
            TEXT(_TARGET_APP)))
        {
            break;
        }

        // Respect OpenSR pause state.
        CheckForPause();

        // Compute running time.
        auto now =
            std::chrono::steady_clock::now();

        float timeSec =
            std::chrono::duration<float>(
                now - startTime).count();

        // Generate fake telemetry.
        GenerateFakeTelemetry(
            *m_pBufferIn->outSimDataIN,
            timeSec);

        // Submit synchronized frame.
        m_pContext->submitFrameCallback(
            m_pContext->userData);

        // 60 Hz update rate.
        std::this_thread::sleep_for(
            std::chrono::milliseconds(16));
    }

    // Mark telemetry unavailable.
    m_pBufferIn
        ->outSimDataIN
        ->mPacketHeader
        .reportAvailable = 0;
}

// =========================================================
// Fake Telemetry Generator
// =========================================================

void NotepadExamplePlugin::GenerateFakeTelemetry(
    OutSimData& data,
    float timeSec)
{
    // =====================================================
    // Packet Header
    // =====================================================

    data.mPacketHeader.paused = false;

    data.mPacketHeader.playerSlotIndex = 0;

    data.mPacketHeader.elapsedTime = timeSec;

    // =====================================================
    // Session
    // =====================================================

    data.mSessionData.sessionTime = timeSec;

    // =====================================================
    // Players
    // =====================================================

    data.mPlayers.carCount = 1;

    strcpy_s(
        data.mPlayers.player[0].carName,
        "Notepad GT3");

    // Fake circular trajectory.
    data.mPlayers.player[0].worldPositionX =
        std::cos(timeSec) * 100.0f;

    data.mPlayers.player[0].worldPositionY =
        std::sin(timeSec) * 100.0f;

    data.mPlayers.player[0].worldPositionZ =
        0.0f;

    // =====================================================
    // Vehicle Data
    // =====================================================

    data.mVehicleData.speed =
        120.0f + std::sin(timeSec) * 20.0f;

    data.mVehicleData.rpm =
        5000.0f + std::sin(timeSec * 2.0f) * 1500.0f;

    data.mVehicleData.maxRpm = 8000.0f;

    data.mVehicleData.gear =
        3 + (int)(std::sin(timeSec) * 2.0f);

    data.mVehicleData.maxGear = 6;

    data.mVehicleData.throttle =
        (std::sin(timeSec) + 1.0f) * 0.5f;

    data.mVehicleData.brake =
        (std::sin(timeSec * 0.5f) + 1.0f) * 0.5f;

    data.mVehicleData.clutch = 0.0f;

    data.mVehicleData.fuelLevel =
        75.0f;

    data.mVehicleData.fuelCapacity =
        100.0f;

    data.mVehicleData.engineTemp =
        92.0f;

    data.mVehicleData.oilTemp =
        105.0f;

    data.mVehicleData.oilPress =
        4.2f;

    data.mVehicleData.turboLevel =
        0.8f;

    data.mVehicleData.absInAction =
        (std::sin(timeSec * 8.0f) > 0.8f);

    data.mVehicleData.tcInAction =
        (std::sin(timeSec * 6.0f) > 0.8f);

    // =====================================================
    // Warning Flags
    // =====================================================

    data.mVehicleData.warningFlag.headlights = 1;

    data.mVehicleData.warningFlag.leftTurnSignal =
        (std::sin(timeSec * 2.0f) > 0.0f);

    data.mVehicleData.warningFlag.rightTurnSignal =
        (std::sin(timeSec * 2.0f) < 0.0f);

    data.mVehicleData.warningFlag.oilWarning = 0;

    data.mVehicleData.warningFlag.pitLimiterActive = 0;

    data.mVehicleData.warningFlag.turboStatus = 1;

    // =====================================================
    // Motion Data
    // =====================================================

    data.mMotionData.velocityX =
        std::cos(timeSec) * 15.0f;

    data.mMotionData.velocityY =
        0.0f;

    data.mMotionData.velocityZ =
        std::sin(timeSec) * 15.0f;

    data.mMotionData.localAccelX =
        std::sin(timeSec * 5.0f);

    data.mMotionData.localAccelY =
        0.1f;

    data.mMotionData.localAccelZ =
        std::cos(timeSec * 5.0f);

    data.mMotionData.pitch =
        std::sin(timeSec) * 5.0f;

    data.mMotionData.roll =
        std::cos(timeSec) * 3.0f;

    data.mMotionData.yaw =
        timeSec * 10.0f;

    data.mMotionData.pitchRate =
        std::cos(timeSec);

    data.mMotionData.rollRate =
        std::sin(timeSec);

    data.mMotionData.yawRate =
        10.0f;

    data.mMotionData.angularVelocityX =
        data.mMotionData.pitchRate;

    data.mMotionData.angularVelocityY =
        data.mMotionData.yawRate;

    data.mMotionData.angularVelocityZ =
        data.mMotionData.rollRate;

    data.mMotionData.angularAccelX =
        std::sin(timeSec * 2.0f);

    data.mMotionData.angularAccelY =
        std::cos(timeSec * 2.0f);

    data.mMotionData.angularAccelZ =
        std::sin(timeSec * 4.0f);

    data.mMotionData.upDirX = 0.0f;

    data.mMotionData.upDirY = 1.0f;

    data.mMotionData.upDirZ = 0.0f;
}
