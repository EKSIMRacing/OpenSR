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
using System.Runtime.InteropServices;
namespace OpenSRPlugin
{
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void SubmitFrameDelegate(IntPtr userData);

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode, Pack = 1)]
    public struct OpenSRContext
    {
        public byte isAppRunning;

        public int targetGamePId;

        // app folder
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
        public string osrAppFolder;

        // OpenSR doc folder
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
        public string osrDocFolder;

        // current profile path loaded
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
        public string profilePath;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 8)]
        public ulong[] reserved;

        public IntPtr submitFrameCallbackPtr;
        public IntPtr userData;
    }
}