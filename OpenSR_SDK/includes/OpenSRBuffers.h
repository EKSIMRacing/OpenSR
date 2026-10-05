#pragma once
#include "OpenSROutSimData.h"
#include "OpenSRInSimData.h"
#include "OpenSRBlobData.h"

// Buffers visible to IN plugins
typedef struct OpenSRBuffersIN {
    OutSimData* outSimDataIN;   // plugin writes telemetry here
    BlobData* blobData;         // plugin writes custom data struct (optional)
} OpenSRBuffersIN;

// Buffers visible to OUT plugins
typedef struct OpenSRBuffersOUT {
    InSimData* inSimData;            // plugin writes input signals
    OutSimData* outSimDataOUT; // plugin reads telemetry
    BlobData* blobData;              // plugin reads custom data struct (optional)
} OpenSRBuffersOUT;
