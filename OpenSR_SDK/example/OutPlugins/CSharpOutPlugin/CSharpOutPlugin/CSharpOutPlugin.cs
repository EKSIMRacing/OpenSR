/*
* #############################################################################################
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
#   A (sim unit) or (sim specific) is a simulation field with a fixed role in the data structure.
#   Its internal values(bitfield, enum, or scaling) are defined by developer for the target game.
#
# Change History:
#   2025-10-01 : Initial public release
#################################################################################################

*/
using System;
using System.Threading;
using System.Runtime.InteropServices;
using System.IO;
using System.Xml.Linq;
using OpenSRPlugin;


// CSharpOutPlugin/Plugin.cs
//
// Generic OUT device plugin - full IOpenSRPlugin surface:
//   * callbacks      : Init/Start/Pause/Resume/Stop/Shutdown/OnContextChanged/IsSettingsReady
//   * settings.xml   : created with defaults on first run, loaded, reloaded on SettingsChanged
//   * CheckDevice    : IsCheckingAllowed + stub probe, results mirrored into SharedData
//   * GetSharedPointer: shared state block consumed by the UI plugin (host is agnostic)
//
// The C++/CLI bridge reflects into the static class Plugin below:
// do NOT rename the class, its namespace, or its static methods.


namespace CSharpOutPlugin
{
    // values MUST match osr::OpenSRContextChange in IOpenSRPlugin.h (by value):
    // the bridge passes the raw int, a wrong mapping silently mis-routes callbacks
    public enum OpenSRContextChange : int
    {
        ProfilePathChanged = 0,
        SettingsChanged = 1,
        FullContextReloaded = 2,
        CheckDevice = 3,
        Init = 4
    }

    // device state, also mirrored to the UI plugin through SharedData
    public enum DeviceState : int
    {
        Idle = 0, Running = 1, Paused = 2, Error = 3, Checking = 4, Ok = 5
    }

    // Internal, plugin-COMPUTED state handed to the UI plugin via
    // GetSharedPointer(). The host is agnostic: it copies the pointer from
    // this plugin to the UI plugin.
    // Raw telemetry (speed, lap time, ...) does NOT belong here: the UI
    // plugin reads OutSimData itself through the context.
    // The C++ UI plugin declares the identical pack(1) layout:
    //
    //   #pragma pack(push, 1)
    //   struct SharedData {
    //       uint32_t magic;      // 0x44564544 'DEVD'
    //       uint32_t version;    // 2
    //       int32_t  state;      // DeviceState
    //       int32_t  errorCode;  // last device error (0 = none)
    //       uint32_t channels;   // active output channel count
    //       wchar_t  status[64]; // NUL-terminated, shown in the UI window
    //   };
    //   #pragma pack(pop)
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct SharedData
    {
        public const uint Magic = 0x44564544; // 'DEVD'
        public const uint Version = 2;

        public uint magic;
        public uint version;
        public int state;       // DeviceState - internal plugin state
        public int errorCode;   // last device error code (0 = none)
        public uint channels;    // active output channel count
        public string status;     // wchar_t[64], NUL-terminated
    }

    public static class Plugin
    {
        // ── host-provided (shared memory, owned by the host) ─────────────────
        private static IntPtr m_pContext;
        private static IntPtr m_pBuffersOut;
        private static string m_pluginPath;
        private static string m_settingsPath;

        // ── worker ────────────────────────────────────────────────────────────
        private static Thread worker;
        private static volatile bool stopRequested = false;
        private static volatile bool isRunning = false;
        private static ManualResetEventSlim pauseEvent = new ManualResetEventSlim(true); // true = not paused
        private static ulong lastFrameId = 0;
        private static IntPtr m_outSimDataPtr = IntPtr.Zero; // cached once: the buffer never moves
        private static volatile bool simPaused = false;      // edge-detect pause, log the transition only

        // ── settings (values live in settings.xml) ────────────────────────────
        private static volatile int rateMs = 16;

        // ── shared state block for the UI plugin ──────────────────────────────
        private static IntPtr sharedPtr = IntPtr.Zero;

        // ══════════════════════════════════════════════════════════════════════
        // lifecycle
        // ══════════════════════════════════════════════════════════════════════

        public static bool Init(IntPtr ctxPtr, IntPtr outBuf, string path)
        {
            m_pContext = ctxPtr;
            m_pBuffersOut = outBuf;
            m_pluginPath = path;
            if (m_pContext == IntPtr.Zero || m_pBuffersOut == IntPtr.Zero)
                return false;
            // cache the OutSimData pointer once - it never moves
            m_outSimDataPtr = Marshal.PtrToStructure<OpenSRBuffersOUT>(m_pBuffersOut).outSimDataOUT;
#if DEBUG
            Console.WriteLine("\n[CSharpOutPlugin] SizeOf OutSimData");
            Console.WriteLine("PacketHeader: " + Marshal.SizeOf<PacketHeader>());
            Console.WriteLine("-------------");
            Console.WriteLine("MotionData: " + Marshal.SizeOf<MotionData>());
            Console.WriteLine("SessionData: " + Marshal.SizeOf<SessionData>());
            Console.WriteLine("PlayerInfo: " + Marshal.SizeOf<PlayerInfo>());
            Console.WriteLine("PlayerRecords: " + Marshal.SizeOf<PlayerRecords>());
            Console.WriteLine("VehicleData: " + Marshal.SizeOf<VehicleData>());
            Console.WriteLine("WheelData: " + Marshal.SizeOf<WheelData>());
            Console.WriteLine("ExtensionData: " + Marshal.SizeOf<ExtensionData>());
            Console.WriteLine("-------- SUB --------");
            Console.WriteLine("WheelInfo: " + Marshal.SizeOf<WheelInfo>());
            Console.WriteLine("EnergySystemData: " + Marshal.SizeOf<EnergySystemData>());
            Console.WriteLine("CarDamageData: " + Marshal.SizeOf<CarDamageData>());
            Console.WriteLine("BrakeData: " + Marshal.SizeOf<BrakeData>());
            Console.WriteLine("SuspensionData: " + Marshal.SizeOf<SuspensionData>());
            Console.WriteLine("warningFlag: " + Marshal.SizeOf<WarningFlag>());
            Console.WriteLine("EnvironmentData: " + Marshal.SizeOf<EnvironmentData>());

            Console.WriteLine("Total OutSimData: " + Marshal.SizeOf<OutSimData>());

#endif

            var context = Marshal.PtrToStructure<OpenSRContext>(m_pContext);
            // same composition as the C++ example: osrDocFolder\pluginPath\settings.xml
            m_settingsPath = Path.Combine(context.osrDocFolder, path, "settings.xml");

            // load settings synchronously: the plugin is the only reader of
            // this file, and the host always saves it BEFORE firing
            // SettingsChanged - there is nothing to race against
            LoadSettings();
            return true;
        }

        public static bool Start()
        {
            if (isRunning)
                return true; // already running

            stopRequested = false;
            lastFrameId = 0;
            pauseEvent.Set(); // ensure not paused on start
            isRunning = true;
            worker = new Thread(WorkerThread) { Name = "DeviceWorker" };
            worker.Start();
            SetSharedState(DeviceState.Running, "Running");
            return true;
        }

        public static void Pause()
        {
            pauseEvent.Reset(); // block the worker thread
            SetSharedState(DeviceState.Paused, "Paused");
        }

        public static void Resume()
        {
            pauseEvent.Set(); // unblock the worker thread
            if (!stopRequested)
                SetSharedState(DeviceState.Running, "Running");
        }

        public static bool IsRunning()
        {
            // false as soon as the worker exits (normally or after a caught
            // crash) - this is how the host detects an unexpected death
            return isRunning;
        }

        public static void Stop()
        {
            stopRequested = true;
            pauseEvent.Set(); // wake a paused worker so it can see the stop
            if (worker != null && worker.IsAlive)
                worker.Join();
            isRunning = false;
            SetSharedState(DeviceState.Idle, "Stopped");
        }

        public static bool Shutdown()
        {
            Stop();
            if (sharedPtr != IntPtr.Zero)
            {
                Marshal.FreeHGlobal(sharedPtr);
                sharedPtr = IntPtr.Zero;
            }
            return true;
        }

        // ══════════════════════════════════════════════════════════════════════
        // callbacks
        // ══════════════════════════════════════════════════════════════════════

        public static void OnContextChanged(int reason)
        {
            switch ((OpenSRContextChange)reason)
            {
                case OpenSRContextChange.SettingsChanged:
                    LoadSettings(); // re-read the file the user edited in the UI
                    if (isRunning)
                    {
                        Stop();
                        Start(); // apply the new settings (rates, ...)
                    }
                    break;

                case OpenSRContextChange.ProfilePathChanged:
                    Console.WriteLine("[CSharpOutPlugin] Profile changed: reload per-profile config here");
                    break;

                case OpenSRContextChange.FullContextReloaded:
                    {
                        // re-read anything cached from OpenSRContext
                        var ctx = Marshal.PtrToStructure<OpenSRContext>(m_pContext);
                        m_settingsPath = Path.Combine(ctx.osrDocFolder, m_pluginPath, "settings.xml");
                        break;
                    }

                case OpenSRContextChange.CheckDevice:
                    CheckDevice();
                    break;

                case OpenSRContextChange.Init:
                    Console.WriteLine("[CSharpOutPlugin] Context initialized");
                    break;
            }
        }

        // settings are loaded synchronously in Init (and reloaded in
        // OnContextChanged(SettingsChanged)) - always ready.
        // Only a plugin with a long async init before the settings UI
        // would return false here while busy.
        public static bool IsSettingsReady()
        {
            return true;
        }

        // ══════════════════════════════════════════════════════════════════════
        // metadata - one method per field, exactly like the C++ plugins
        // ══════════════════════════════════════════════════════════════════════

        public static string GetPackageName()
        {
            return "com.zappadoc.outplugin.csdevice";
        }

        public static string GetPluginName()
        {
            return "C# OUT Device";
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
            return "Generic C# OUT device plugin";
        }

        // OUT plugins target no game: empty, like the C++ example
        public static string GetTargetName()
        {
            return "";
        }

        public static string GetTargetProcessName()
        {
            return "";
        }

        public static string GetUITabName()
        {
            return "Device";
        }

        public static string GetSettingsTabName()
        {
            return "Settings";
        }

        public static string GetPluginGroupName()
        {
            return "";
        }

        // The frozen bridge exposes a single C-ABI metadata export,
        // CS_GetMeta(kind, ...) with kind = CSMetaKind from
        // CSharpPluginWrapper.cpp. This is pure mechanical dispatch to the
        // natural methods above - the real values live there.
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

        // OUT_PLUGIN_TYPE = 1 (game/IN plugins return GAME_PLUGIN_TYPE = 0)
        public static int GetPluginType()
        {
            return 1;
        }

        // ══════════════════════════════════════════════════════════════════════
        // device check (stub)
        // ══════════════════════════════════════════════════════════════════════

        public static bool IsCheckingAllowed()
        {
            return true; // host enables the "check device" button
        }

        public static void CheckDevice()
        {
            // called from the host thread - keep it short or run the probe async
            SetSharedState(DeviceState.Checking, "Checking device...");

            bool ok = true;
            // TODO: real probe (open the serial port, ping the device,
            //       read the firmware id, verify the outputs, ...)
            Thread.Sleep(100);

            SetSharedState(ok ? DeviceState.Ok : DeviceState.Error,
                ok ? "Device OK" : "Device not found");
            SharedData d = ReadShared();
            d.errorCode = ok ? 0 : 1;
            WriteShared(d);
        }

        // ══════════════════════════════════════════════════════════════════════
        // shared state for the UI plugin
        // ══════════════════════════════════════════════════════════════════════

        // C++ equivalent:
        //   uint64_t GetSharedPointer() { return reinterpret_cast<uint64_t>(&m_sharedData); }
        // The block lives in unmanaged memory from GetSharedPointer until
        // Shutdown - the C++ UI plugin reads it directly.
        public static ulong GetSharedPointer()
        {
            if (sharedPtr == IntPtr.Zero)
            {
                sharedPtr = Marshal.AllocHGlobal(Marshal.SizeOf<SharedData>());
                WriteShared(new SharedData
                {
                    magic = SharedData.Magic,
                    version = SharedData.Version,
                    state = (int)DeviceState.Idle
                });
            }
            return (ulong)sharedPtr;
        }

        private static void SetSharedState(DeviceState state, string text)
        {
            if (sharedPtr == IntPtr.Zero)
                return; // UI not attached yet
            SharedData d = ReadShared();
            d.state = (int)state;
            d.status = text ?? "";
            if (d.status.Length > 63)
                d.status = d.status.Substring(0, 63);
            WriteShared(d);
        }

        private static SharedData ReadShared()
        {
            if (sharedPtr == IntPtr.Zero)
                return new SharedData();
            return Marshal.PtrToStructure<SharedData>(sharedPtr);
        }

        private static void WriteShared(SharedData d)
        {
            if (sharedPtr != IntPtr.Zero)
                Marshal.StructureToPtr(d, sharedPtr, false);
        }

        // ══════════════════════════════════════════════════════════════════════
        // worker
        // ══════════════════════════════════════════════════════════════════════

        private static void WorkerThread()
        {
            Console.WriteLine("[CSharpOutPlugin] Enter WorkerThread...");
            isRunning = true;
            try
            {
                while (!stopRequested)
                {
                    // efficiently wait here if Pause() has been called
                    pauseEvent.Wait();
                    if (stopRequested)
                        break;

                    // stop if the game app is gone (fresh read every tick)
                    var context = Marshal.PtrToStructure<OpenSRContext>(m_pContext);
                    if (context.isAppRunning == 0)
                        break;
                        
                   DoWork();

                    // rate tick: waits rateMs,
                    Thread.Sleep(rateMs);
                }
            }
            catch (Exception e)
            {
                // must never escape: an unhandled exception on this thread
                // crashes the whole host process (.NET Framework)
                Console.WriteLine("[CSharpOutPlugin] Worker crashed: " + e);
                SetSharedState(DeviceState.Error, "Worker crashed");
                SharedData d = ReadShared();
                d.errorCode = 2;
                WriteShared(d);
            }
            isRunning = false;
            Console.WriteLine("[CSharpOutPlugin] Exit WorkerThread...");
        }

        private static void DoWork()
        {
            unsafe
            {
                if (m_outSimDataPtr == IntPtr.Zero)
                    return;
                OutSimData* pData = (OutSimData*)m_outSimDataPtr;

                if (pData->packetHeader.reportAvailable < 1)
                    return;
                ulong f1 = pData->packetHeader.frameId;
                if (f1 == lastFrameId)
                    return; // no new frame

                // copy what you need (a few hundred bytes, cheap)
                MotionData motion = pData->motionData;
                VehicleData vehicle = pData->vehicleData;

                lastFrameId = f1;

                if (pData->packetHeader.paused > 0)
                {
                    if (!simPaused)
                        Console.WriteLine("[CSharpOutPlugin] Sim paused");
                    simPaused = true;
                    return; // hold the device while the sim is paused
                }
                if (simPaused)
                {
                    Console.WriteLine("[CSharpOutPlugin] Sim resumed");
                    simPaused = false;
                }
               
                int slot = pData->packetHeader.playerSlotIndex;
                PlayerInfo* pPlayer = &pData->players.player;
                double pitch = motion.pitch;
                double roll = motion.roll;

                float speed = vehicle.speed;
                float lpTime = pPlayer[slot].currentLapTime;
                float carSpeed = pPlayer[slot].carSpeed;
                Console.WriteLine("[CSharpOutPlugin] Working: FrameId:" + lastFrameId + " LpTime:" + lpTime + " | Speed:" + speed + " | pitch:" + pitch);
                

                // publish the computed values for the UI window
                SharedData d = ReadShared();
                // d.channels = activeChannelCount; // from your device config
                d.channels = 1;                  // TODO: last computed output, channel 0
                WriteShared(d);
            }
        }

        // ══════════════════════════════════════════════════════════════════════
        // settings.xml  (host UI schema: <settings><option ...>value</option>)
        // ══════════════════════════════════════════════════════════════════════

        private static void LoadSettings()
        {
            try
            {
                if (File.Exists(m_settingsPath))
                {
                    XDocument doc = XDocument.Load(m_settingsPath);
                    if (doc.Root != null)
                    {
                        foreach (XElement option in doc.Root.Elements("option"))
                        {
                            var id = option.Attribute("id");
                            if (id == null)
                                continue;
                            switch (id.Value)
                            {
                                case "device-update-delay": int r; if (int.TryParse(option.Value, out r)) rateMs = r; break;
                                    // one case per option added below
                            }
                        }
                        Console.WriteLine("[CSharpOutPlugin] Settings loaded: " + m_settingsPath);
                    }
                }
                else
                {
                    // first run: create the file with defaults; the host UI edits
                    // it and then fires OnContextChanged(SettingsChanged)
                    XDocument doc = new XDocument(
                        new XDeclaration("1.0", "UTF-8", null),
                        new XElement("settings",
                            new XAttribute("type", "unique"),
                            new XElement("option",
                                new XAttribute("name", "device update delay (ms)"),
                                new XAttribute("id", "device-update-delay"),
                                new XAttribute("type", "slider"),
                                new XAttribute("range", "1,1000"),
                                rateMs.ToString())));
                    Directory.CreateDirectory(Path.GetDirectoryName(m_settingsPath));
                    doc.Save(m_settingsPath);
                    Console.WriteLine("[CSharpOutPlugin] Settings created with defaults: " + m_settingsPath);
                }
            }
            catch (Exception e)
            {
                Console.WriteLine("[CSharpOutPlugin] LoadSettings failed (using defaults): " + e.Message);
            }
        }
    }
}
