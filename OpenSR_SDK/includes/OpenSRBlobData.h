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


#ifndef _OpenSR_BLOBDATA_H
#define _OpenSR_BLOBDATA_H

#include "OpenSROutSimData.h"
#include <cstdint>

#pragma pack(push, 1)

typedef struct Blob_t {
	// Extension blob
    size_t  blobSize /* = 0 */;           // effective buffer size in bytes (optional)
	uint8_t blob[32768];            // opaque binary data buffer for custom usage (third party struct)
} Blob_t;

typedef struct BlobData {
	PacketHeader_t	mPacketHeader;	// size 32
	Blob_t			mBlobData;		// size 32808
} BlobData;

/*
--------------
PacketHeader 32
-------------
BlobData 32808
*/

#pragma pack(pop)

#endif _OpenSR_BLOBDATA_H


