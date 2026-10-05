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

using System.Runtime.InteropServices;

namespace OpenSRPlugin
{
    [StructLayout(LayoutKind.Sequential, Pack = 1)] // Add Pack=1
    public struct INPacketHeader
    {
        public byte reportAvailable;
        public byte majorVersion;
        public byte minorVersion;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 29)]
        public byte[] spare;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct ReportIn
    {
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 8)]
        public sbyte[] axis;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 8)]
        public byte[] button;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 2)]
        public byte[] hat;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 8)]
        public ushort[] swt;
		public ushort devClass;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)] // Add Pack=1 and remove CharSet
    public struct InSimData
    {
        public INPacketHeader mInPacketHeader;
        public ReportIn mReportIn;
    }

}
