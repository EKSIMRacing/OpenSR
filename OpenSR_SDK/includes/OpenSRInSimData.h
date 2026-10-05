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

#include <cstdint>

#ifndef _OpenSR_INSIMDATA_H
#define _OpenSR_INSIMDATA_H

#pragma pack(push, 1)

//--------------------------------------
// IN PacketHeader (32 bytes aligned)
//--------------------------------------
struct INPacketHeader {
	uint8_t reportAvailable;	// true as soon as this struct is ready to be parsed by the Manager
	uint8_t majorVersion;  		// 1
	uint8_t minorVersion;  		// 0
	uint8_t spare[29];    		// Align to 32 bytes
};

static_assert(sizeof(INPacketHeader) == 32, "PacketHeader must be 32 bytes");

//--------------------------------------
// InSimData
//--------------------------------------
typedef struct _ReportIn {
	int8_t axis[8];                 // 8 axes, analog pot
	uint8_t button[8];             	// 64 buttons 8x8
	uint8_t hat[2];               	// 2 hat
	uint16_t swt[8];     			// 8 switch pos. 1 to 12, axis converted to switch
	uint16_t devClass;
} ReportIn;

struct InSimData {
	INPacketHeader   mInPacketHeader; // usage in network protocol like UDP
	ReportIn         mReportIn;
}; //full struct in inSimData.h

#pragma pack(pop)

#endif _OpenSR_INSIMDATA_H


