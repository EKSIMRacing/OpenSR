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

#ifndef OPENSR_CONTEXT_H
#define OPENSR_CONTEXT_H					
#define NOMINMAX
#include <windows.h>
#include <ctype.h>
#include <string>
#include <stdint.h> // Required for fixed width types (uint8_t, etc)


#pragma pack(push, 1)

struct OpenSRContext {
    uint8_t isAppRunning /*= 0 */;				// flag 1 if OpenSR running
    uint32_t targetGamePId /* =0 */;				// current target process ID, 0 if N/A
	wchar_t osrAppFolder[MAX_PATH];			// OpenSR App folder path
	wchar_t osrDocFolder[MAX_PATH];			// .../Documents/OpenSR/ folder path
	wchar_t currentProfilePath[MAX_PATH];	// current loaded profile path
	uint64_t reserved[8];

	void (*submitFrameCallback)(void* userData);
	void* userData;
 };

#pragma pack(pop)

#endif // OPENSR_CONTEXT_H


