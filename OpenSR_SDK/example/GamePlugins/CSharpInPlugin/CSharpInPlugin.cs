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
using System;
using System.Threading;
using System.Runtime.InteropServices;
using System.Diagnostics;
using OpenSRPlugin;
using TelemetryAutomation;

namespace CSharpInPlugin
{
    public static class Plugin
    {
        // values MUST match osr::OpenSRContextChange in IOpenSRPlugin.h (by value)
        public enum OpenSRContextChange : int
        {
            ProfilePathChanged = 0,
            SettingsChanged = 1,
            FullContextReloaded = 2,
            CheckDevice = 3,
            Init = 4
        }

        private static string m_pluginPath;
        private static IntPtr m_pContext;
        private static IntPtr m_pBuffersIn;
        private static Thread worker;
        private static volatile bool stopRequested = false;
        private static volatile bool isRunning = false;
        // For pausing
        private static ManualResetEventSlim pauseEvent = new ManualResetEventSlim(true); // true = not paused initially

        private static Action submitFrameCallback;
        public static bool Init(IntPtr ctxPtr, IntPtr inBuf, [MarshalAs(UnmanagedType.LPWStr)] string path)
        {
            //Debugger.Launch();
            m_pContext = ctxPtr;
            m_pBuffersIn = inBuf;
            m_pluginPath = path;

            var context = Marshal.PtrToStructure<OpenSRContext>(ctxPtr);

            if (context.submitFrameCallbackPtr != IntPtr.Zero)
            {
                // 1. Get the delegate for the C-style function pointer
                var nativeCallback = Marshal.GetDelegateForFunctionPointer<SubmitFrameDelegate>(context.submitFrameCallbackPtr);

                // 2. Capture the userData pointer from the context
                IntPtr userData = context.userData;

                // 3. Create a clean, parameter-less C# Action that calls the native delegate with the captured userData.
                // This is the C# equivalent of the C++ lambda's capture.
                submitFrameCallback = () => nativeCallback(userData);
            }
            else
            {
                // Handle the case where the callback is null
                submitFrameCallback = () => { }; // Do nothing
            }
            //// Convert the function pointer into a callable delegate
            //submitFrameCallback = Marshal.GetDelegateForFunctionPointer<SubmitFrameDelegate>(context.submitFrameCallbackPtr);

            Console.WriteLine("[CSharpInPlugin] Init: " + context.osrDocFolder);
            return true;
        }

        public static bool Start()
        {
            if (isRunning) return true; // already running

            stopRequested = false;
            pauseEvent.Set(); // Ensure it's not paused on start

            Console.WriteLine("[CSharpInPlugin] Start");
            isRunning = true;
            worker = new Thread(WorkerThread);
            worker.Start();
            return true;
        }

        public static void Pause()
        {
            pauseEvent.Reset(); // Block the worker thread
            Console.WriteLine("C# Plugin Paused.");
        }

        public static void Resume()
        {
            pauseEvent.Set(); // Unblock the worker thread
            Console.WriteLine("C# Plugin Resumed.");
        }

        public static bool IsRunning()
        {
            return isRunning;
        }

        public static void Stop()
        {
            stopRequested = true;
            pauseEvent.Set(); // Make sure it's not paused, so it can see the stop request

            Console.WriteLine("[CSharpInPlugin] Stop");

            if (worker != null && worker.IsAlive)
                worker.Join();

            isRunning = false;

        }

        public static bool Shutdown()
        {
            Console.WriteLine("[CSharpInPlugin] Shutdown");
            return true;
        }

        // required by the bridge; settings are loaded synchronously, so always ready
        public static bool IsSettingsReady()
        {
            return true;
        }

        public static void ReloadSettings()
        {
            Console.WriteLine("[CSharpInPlugin] Setting Must Be Reloaded.");
        }

        public static void OnContextChanged(OpenSRContextChange reason)
        {
            switch (reason)
            {
                case OpenSRContextChange.SettingsChanged:
                    ReloadSettings();
                    break;
                case OpenSRContextChange.ProfilePathChanged:
                    // reload per-profile data here if needed
                    break;
                case OpenSRContextChange.FullContextReloaded:
                    // re-read OpenSRContext here if needed
                    break;
            }
        }

        // ── metadata (bridge CS_GetMeta / CS_GetPluginType, kind = CSMetaKind
        //    in CSharpPluginWrapper.cpp) ──────────────────────────────────────

        public static string GetPackageName()
        {
            return "com.opensrteam.gameplugin.csnotepad";
        }

        public static string GetPluginName()
        {
            return "C# IN Notepad";
        }

        public static string GetAuthor()
        {
            return "Your Name";
        }

        public static string GetVersion()
        {
            return "0.1";
        }

        public static string GetLicenseType()
        {
            return "MIT";
        }

        public static string GetDescription()
        {
            return "C# IN plugin: plays a recorded lap (demo.lap) as telemetry";
        }

        // IN/game plugins DO target a game
        public static string GetTargetName()
        {
            return "Notepad";
        }

        public static string GetTargetProcessName()
        {
            return "Notepad.exe";
        }

        public static string GetUITabName()
        {
            return "Plugin";
        }

        public static string GetSettingsTabName()
        {
            return "Settings";
        }

        public static string GetPluginGroupName()
        {
            return "";
        }

        // single C-ABI metadata export in the frozen bridge: pure dispatch
        public static string GetMeta(int kind)
        {
            switch (kind)
            {
                case 0: return GetPackageName();
                case 1: return GetPluginName();
                case 2: return GetAuthor();
                case 3: return GetVersion();
                case 4: return GetLicenseType();
                case 5: return GetDescription();
                case 6: return GetTargetName();
                case 7: return GetTargetProcessName();
                case 8: return GetUITabName();
                case 9: return GetSettingsTabName();
                case 10: return GetPluginGroupName();
                default: return "";
            }
        }

        // GAME_PLUGIN_TYPE = 0
        public static int GetPluginType()
        {
            return 0;
        }

        private static void WorkerThread()
        {
            isRunning = true;

            // Get a managed copy of the *outer* structure to find the pointer to the data we need to modify.
            // This is okay to do once at the beginning.
            var bufferIn = Marshal.PtrToStructure<OpenSRBuffersIN>(m_pBuffersIn);
            IntPtr outSimDataPtr = bufferIn.outSimDataIN; // This is the pointer we need to write to.


            var context = Marshal.PtrToStructure<OpenSRContext>(m_pContext);

            string telemetryFilePath = context.osrDocFolder + "\\Map\\demo.lap";
            TelemetryPlayer my_car = new TelemetryPlayer(telemetryFilePath);
            // Set a constant time step for physics updates (in seconds)
            const double Dt = 0.02; // 2 ms

            // In C#, TimeSpan is the equivalent of std::chrono::duration.
            // TimeSpan.FromMilliseconds directly accepts a double, so no cast is needed.
            TimeSpan updateInterval = TimeSpan.FromMilliseconds(Dt * 1000);

            // Alternatively, you can calculate the milliseconds as an integer first.
            // This is often used with Thread.Sleep(int).
            int updateIntervalMs = (int)(Dt * 1000); // Result is 20

            // Using camelCase for local variables is the C# convention
            double totalElapsedTime = 0.016;
            Console.WriteLine("[CSharpInPlugin] Enter WorkerThread...");

            // This loop now operates directly on the unmanaged memory.
            // This loop now operates directly on the unmanaged memory.
            // An unhandled exception here would crash the whole host process
            // (.NET Framework) - contain it
            try
            {
                while (!stopRequested)
                {

                    pauseEvent.Wait(); // Efficiently waits if paused

                    // The entire block that works with pointers must be marked as unsafe.
                    unsafe
                    {
                        if (Marshal.PtrToStructure<OpenSRContext>(m_pContext).isAppRunning == 0) // fresh read
                            break;
                        // 1. Cast the IntPtr to a typed pointer.
                        //    This does NOT copy the data. pData is now a "handle" to the original C++ memory.
                        OutSimData* pData = (OutSimData*)outSimDataPtr;


                        //pData->packetHeader.reportAvailable = 1;
                        pData->packetHeader.paused = 0; // Or whatever the correct state is

                        my_car.Update(totalElapsedTime);

                        pData->packetHeader.paused = 0;
                        int slot = 89;
                        pData->packetHeader.playerSlotIndex = (byte)slot;

                        pData->vehicleData.gear = (sbyte)my_car.Gear;
                        pData->vehicleData.maxGear = 6;
                        pData->vehicleData.speed = (float)my_car.SpeedKph;

                        pData->vehicleData.rpm = (float)my_car.Rpm;
                        pData->vehicleData.maxRpm = 7800.0f;

                        pData->sessionData.sessionTime = (float)(totalElapsedTime % 138.0);

                        // To access the wheels, you still need a pointer to the first element
                        WheelInfo* pWheelsArray = &pData->wheelData.wheels;
                        pWheelsArray[0].wheelSpeed = 0.4f; pWheelsArray[0].tyreWearOverall = 0.1f;
                        pWheelsArray[1].wheelSpeed = 0.8f; pWheelsArray[1].tyreWearOverall = 0.2f;
                        pWheelsArray[3].wheelSpeed = 0.8f; pWheelsArray[3].tyreWearOverall = 0.4f;

                        pData->players.carsCount = 1;

                        PlayerInfo* pFirstPlayer = &pData->players.player;

                        pFirstPlayer[slot].lapsCompleted = my_car.LapCount;
                        pFirstPlayer[slot].currentLap = my_car.LapCount - 1;
                        pFirstPlayer[slot].lapDist = 6.0f;
                        pFirstPlayer[slot].carSpeed = pData->vehicleData.speed;
                        pFirstPlayer[slot].lastLapTime = (float)my_car.LastLapTime;
                        pFirstPlayer[slot].currentLapTime = (float)my_car.CurrentLapTime;

                        pFirstPlayer[slot].worldPositionX = (float)my_car.WorldX;
                        pFirstPlayer[slot].worldPositionY = (float)my_car.WorldY;
                        pFirstPlayer[slot].worldPositionZ = (float)my_car.WorldZ;

                        pData->motionData.localAccelX = 0.02;
                        pData->motionData.localAccelY = 0.05;
                        pData->motionData.localAccelZ = 0.06;

                        totalElapsedTime += Dt;

                        Console.WriteLine("[CSharpInPlugin] Working...");

                        // 4. Publish AFTER the data is written: bump frameId (OUT
                        //    plugins detect new frames by it), fence, then reportAvailable
                        pData->packetHeader.frameId++;
                        Thread.MemoryBarrier();
                        pData->packetHeader.reportAvailable = 1;
                        submitFrameCallback?.Invoke();
                    }
                 
                    Thread.Sleep(updateIntervalMs); 
                }
            }
            catch (Exception e)
            {
                Console.WriteLine("[CSharpInPlugin] Worker crashed: " + e);
                stopRequested = true;
            }

            // When stopping, clear the reportAvailable flag.
            unsafe
            {
                OutSimData* pData = (OutSimData*)outSimDataPtr;
                pData->packetHeader.paused = 1; // set pause before exit
                submitFrameCallback?.Invoke();
                Thread.Sleep(20);

                pData->packetHeader.reportAvailable = 0; // clean report 
                submitFrameCallback?.Invoke(); // Submit one last frame to signal shutdown
                Console.WriteLine("[CSharpInPlugin] Close Report: " + pData->packetHeader.reportAvailable);
            }

            isRunning = false;
            Console.WriteLine("[CSharpInPlugin] Exit WorkerThread...");
        }
    }

}

