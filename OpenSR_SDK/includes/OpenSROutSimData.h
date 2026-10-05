#pragma once
/*
* #############################################################################################
# Open Sim Relay (OpenSR) - Plugin Interface & Context Data
# Copyright (c) 2025 Zdc - All Rights Reserved
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

#ifndef _OpenSR_OUTSIMDATA_H
#define _OpenSR_OUTSIMDATA_H

#include <cstdint>


#define TYRE_INDEX_MAX 4
#define OPENSR_MAX_PLAYERS 96

#pragma pack(push, 1)

enum dataType : uint8_t {
	dtMotionData,
	dtSessionData,
	dtVehicleStateData,
	dtWheelData,
	dtPlayersData,
	dtExtensionData
};


//--------------------------------------
// PacketHeader (32 bytes aligned)
//--------------------------------------
typedef struct PacketHeader_t {
	uint8_t reportAvailable;	// true as soon as this struct is ready to be parsed by the Manager
	uint8_t paused;				// sim paused true or false (Player in or out car)
	uint64_t elapsedTime;       // Seconds
	uint64_t frameId;  			// [READ ONLY] Incrementing frame ID reference, incremented on send packet to network
	uint32_t frameRateFromGame;	// frame rate from game telemetry
	uint8_t majorVersion;  		// 1
	uint8_t minorVersion;  		// 0 (minor change is backward compatible with the existing struct)
	uint8_t packetType;        	// Type: 0 MotionData, 1 VehicleStateData, etc. see dataType
	uint8_t packetIndex;		// current packet index (only used for dtPlayersData)	   
	uint8_t playerSlotIndex;	// slot index of local car in player records [] array
	uint8_t _padding[5];		// padding								   
} PacketHeader_t;														   

static_assert(sizeof(PacketHeader_t) == 32, "PacketHeader must be 32 bytes size/alignment is incorrect");

typedef struct MotionData_t {
    double localAccelX;			// local acceleration [m/s²]
    double localAccelY;			// local acceleration [m/s²]
    double localAccelZ;			// local acceleration [m/s²]

    double localVelocityX;		// Local velocity [m/s]
    double localVelocityY;		// Local velocity [m/s]
    double localVelocityZ;		// Local velocity [m/s]

    double angularVelocityX;	// Local angular velocity [rad/s]
    double angularVelocityY;	// Local angular velocity [rad/s]
    double angularVelocityZ;	// Local angular velocity [rad/s]

    double angularAccelX;		// Local angular acceleration [rad/s²]
    double angularAccelY;		// Local angular acceleration [rad/s²]
    double angularAccelZ;		// Local angular acceleration [rad/s²]

    double pitch;				// pitch Orientation (radians)
    double yaw;					// yaw Orientation (radians)
    double roll;				// roll Orientation (radians)

  
    double pitchRate;		    // pitch rates [rad/s]
	double yawRate;			    // yaw rates [rad/s]
	double rollRate;		    // roll rates [rad/s]

    // G-forces
    double gForceLat;			// lateral G
    double gForceVert;			// vertical G
    double gForceLong;			// longitudinal G

	double velocityX;			// world velocity [m/s]
	double velocityY;			// world velocity [m/s]
	double velocityZ;			// world velocity [m/s]

	double carCGLocX;			// location of car's center of gravity in world space (X, Y, Z)
	double carCGLocY;			// location of car's center of gravity in world space (X, Y, Z)
	double carCGLocZ;			// location of car's center of gravity in world space (X, Y, Z)

	 // Rotation matrix (car orientation in world frame)
	double rotMat[3][3];

	// World space up direction
	double upDirY;
	double upDirX;
	double upDirZ;

	// World space right direction
	double rightDirX;
	double rightDirY;
	double rightDirZ;

	// World space forward direction
	double frwdDirX;
	double frwdDirY;
	double frwdDirZ;

	// Velocity in world space
	double worldVelocityX;
	double worldVelocityY;
	double worldVelocityZ;

	// Suspension deflection [m]
	double suspensionDeflectionLF;
	double suspensionDeflectionRF;
	double suspensionDeflectionLR;
	double suspensionDeflectionRR;

	// Suspension velocity [m/s]
	double suspensionVelocityLF;
	double suspensionVelocityRF;
	double suspensionVelocityLR;
	double suspensionVelocityRR;
} MotionData_t;

static_assert(sizeof(MotionData_t) == 448, "MotionData must be 448 bytes size/alignment is incorrect");

//--------------------------------------
// EnvironmentData
//--------------------------------------

typedef struct EnvironmentData_t {
	// --- Time & lighting ---
	float     timeOfDay;		// seconds since midnight
	float     timeScale;		// time acceleration factor
	float     sunAltitude;		// sun height angle [deg]
	float     sunAzimuth;		// sun azimuth angle [deg]
	float     cloudLevel;		// fog density [0..1]
	float     fogLevel;			// fog density [0..1]

	// --- Atmosphere ---
	float     airTemp;			// ambient air temperature [°C]
	float     airPressure;		// barometric pressure [kPa or hPa]
	float     humidity;			// relative humidity [%]
	float     airDensity;		// air density [kg/m³]

	// --- Wind ---
	float     windSpeed;		// magnitude [m/s]
	float     windDirection;	// heading [deg from north]

	// --- Precipitation ---
	float     rainLevel;		// rain intensity [0..1]
	float     trackWetness;		// water on track [0..1]
	float     puddleLevel;		// puddle accumulation [0..1]
	float     snowLevel;		// snow intensity [0..1] if supported

	// --- Track surface condition ---
	float     trackTemp;		// track surface temperature [°C]
	float     trackGrip;		// grip coefficient / rubber level
	float     trackDust;		// dust level [0..1]
	float     trackMarbles;		// marbles level [0..1]
	float     trackOil;			// oil/dirt contamination [0..1]
	uint8_t	  weatherFlag;		// (sim specific) like 0 = clear, 1 = light cloud, 2 = overcast, 3 = light rain, 4 = heavy rain, 5 = storm
	uint8_t   _padding[3];
} EnvironmentData_t;

static_assert(sizeof(EnvironmentData_t) == 88, "EnvironmentData must be 88 bytes size/alignment is incorrect");


typedef struct AidSettings_t {
	int8_t assistAbs;				// ABS; -1 = N/A, 0 = off, >= 1 currently active
	int8_t assistTc;					// TC; -1 = N/A, 0 = off, >= 1 currently active
	int8_t assistEsp;				// ESP; -1 = N/A, 0 = off, >= 1 currently active
	int8_t assistCountersteer;		// Countersteer; -1 = N/A, 0 = off, >= 1 currently active
	int8_t assistCornering;			// Cornering; -1 = N/A, 0 = off, >= 1 currently active
	int8_t assistStability;			// 0 (off), >= 1 currently active (sim specific)
	int8_t assistDRS;               // 0 = off, 1 = on
	int8_t assistERS;               // 0 = off, 1 = on
	int8_t assistBrake;				// 0 = off, 1 = low, 2 = medium, 3 = high 
	int8_t aiLevel;					// AI Level from 0 to 255
	int8_t penaltiesEnabled;		// 0 (off), >= 1 enabled
	int8_t setupFuelRate;			// consumption rate (sim specific)
	int8_t tyreWearEnabled;			// Flag: 1 = wear model active, 0 = disabled
	int8_t setupTyreWearRate;		// tyre wear rate (sim specific)
	int8_t damageEnabled;			// damages allowed
	int8_t setupDamageRate;			// car damage rate (sim specific)
	int8_t allowTyreBlankets;		// starts with hot (optimal temp) tyres
	int8_t autoShift;				// 0 (off), 1 (upshifts), 2 (downshifts), 3 (all)
	int8_t autoClutch;				// 0 (off), 1 (on)
	int8_t autoPit;					// 0 (off), 1 (on)
	int8_t autoLift;				// 0 (off), 1 (on)
	int8_t autoBlip;				// 0 (off), 1 (on)
	int8_t autoReverse;				// 0 (off), 1 (on)
	int8_t HoldClutch;				// for auto-shifters at start of race: 0 (off), 1 (on)
	int8_t numStarters;
	int8_t standingStart;			// 1 standing, 0 rolling
	int8_t startingGrid;			// enum single file, double file, etc.
	int8_t hardcoreLevel;			// realism level (sim unit)
	int8_t incidentLimit;			// -1 = unlimited
	uint8_t _padding[3];
} AidSettings_t;

static_assert(sizeof(AidSettings_t) == 32, "AidSettings must be 32 bytes size/alignment is incorrect");

//=====================================================================
// SESSION DATA  information that applies to the whole event
//=====================================================================
typedef struct SessionData_t {
	// Core session identifiers & state
	int8_t  sessionType;			// 0 = Practice, 1 = Qualify, 2 = Race
	int8_t  sessionPhase;			// 0 = Garage, 1 = Gridwalk, 2 = Formation, 3 = Green, etc.
	int8_t  sessionMode;			// enum full, replay, etc.
	int8_t  sessionEventId;			// event id
	int8_t  sessionFormat;			// Laps or Time based or ...
	int8_t  sessionUniqueId;		//
	int8_t  sessionFlags;			// session flags detailed in warningFlags section
	int8_t  category;				// enum Road, Oval, etc.
	int8_t  safetyCarStatus;		// 0 = none, 1 = full, 2 = VSC
	int32_t numLaps;				// unlimited/time based = -1
	int8_t  numSectors;				//
	uint8_t sessionSectorYellow[16];		// sector marked yellow on current track
	int8_t  pitWindowStatus;		// unavailable, close, open, etc.

	// Session timing / tick
	float    sessionFrame;         // frame tick
	float    sessionTimeOfDay;     // seconds since midnight
	float    sessionTimeRemain;    // seconds remaining in session
	float    sessionTime;          // session time total/elapsed
	int32_t  lapsRemain;           // Session Laps Remain

	// Track information (static for the session)
	float   trackLength;          // m
	float   trackAltitude;        // meters
	float   trackLatitude;        // degrees
	float   trackLongitude;       // degrees
	float   trackNorthOffset;     // radians
	float   pitSpeedLimit;        // kph pit limit on this track
	int32_t trackId;              // unique Id
	int32_t trackLayoutId;        // layout/config Id
	int32_t numTurns;

	// Event / weekend identifiers
	int32_t seriesId;
	int32_t seasonId;
	int32_t sessionId;
	int32_t subSessionId;
	int32_t leagueId;
	int32_t raceWeek;

	// Entry-list / car-class information
	int8_t  numCars;               // total cars on track
	int8_t  numCarClasses;
	uint16_t listCarClasses;       // bitfield (sim unit)
	int8_t  numCarTypes;
	uint16_t listCarTypes;         // bitfield (sim unit)
	int8_t  minDrivers;
	int8_t  maxDrivers;

	// Weekend / rule options / settings
	AidSettings_t options;

	// Weather / environmental data
	EnvironmentData_t environment;   // size

	// Version / meta information
	char versionStr[32];			// game version of sharedmem version or both

	// track string
	char trackName[64];
	char trackLayoutName[64];		// track config or layout name
	char trackCity[64];
	char trackCountry[64];
} SessionData_t;

static_assert(sizeof(SessionData_t) == 528, "SessionData_t must be 528 bytes size/alignment is incorrect");

// =====================================================================
// PLAYER DATA per-driver / per-car static & dynamic info
// =====================================================================
typedef struct PlayerInfo_t {
	// Identification
	uint8_t isLocalPlayer;				// 1 if record is for local player

	int32_t driverId;					// Unique driver identifier
	int32_t teamId;						// Team identifier
	int32_t carLiveryId;
	int32_t carNumber;					// Raw numeric car number
	int32_t carClassId;					// Class identifier
	int32_t carModelId;					// Model identifier
	int32_t manufacturerId;				// manufacturer identifier
	int32_t carClassPerformanceIndex;	// (sim specific range)
	
	// Status flags (binary / boolean)
	int8_t driverStatus;			// (sim specific) example: 0 = in garage, 1 = flying lap, 2 = in lap, 3 = out lap, 4 = on track, spectactor, controlled by AI
	int8_t isLastLap;
	int8_t isValidLap;
	int8_t lapValidState;
	int8_t isFinished;
	int8_t engineType;				// (sim specific) combustion, eletric, hybrid, ...
	int8_t engineState;				// (sim specific) example: -1 unavailable, 0 = ignition off, 1 = ignition on but not running, 2 = ignition on and running
	int8_t drsState;				// drs state
	int8_t ptpState;				// push-to-pass state
	int8_t inGarage;				// car in garage
	int8_t inPitlane;				// If current vehicle is in pitlane (-1 = N/A)
	int8_t inPitBox;				// 1 = in own pit box, 0 = otherwise
	int8_t pitState;				// Enum: none, entering, in-lane, in-stall, exiting, etc.
	int8_t numPitstops;				// Number of pitstops the current vehicle has performed (-1 = N/A)
	int8_t pitOptionalRepairLeft;
	int8_t pitRepairLeft;	
	float  pitElapsedTime;			// Current vehicle pitstop actions duration
	float  pitTotalDuration;

	// Race-progress fields
	int8_t driverResult;		// (sim specific) example: 0 = invalid, 1 = inactive, 2 = active, 3 = finished, 4 = didnotfinish, 5 = disqualified, 6 = not classified, 7 = retired
	int8_t gridPosition;		// Grid position the car started the race in
	int8_t position;            // Current race position
	int8_t classPosition;       // Current class position
	int8_t positionFinal;       // race position result
	int8_t positionClassFinal;  // race class position result
	int8_t sector;              // current sector

	int8_t deltaTimeFromGame;   // game provide a delta time
	int32_t lapsCompleted;		// Completed laps
	int32_t currentLap;			// Current lap number
	float   lapDist;			// Distance traveled in current lap [m]
	float   lapDistPct;			// Distance traveled in current lap [Percent]

	// position in world space
	float	worldPositionX;	   // Right (+)
	float	worldPositionY;	   // Up (+)
	float	worldPositionZ;	   // Forward (+)

	// car basic
	float carSpeed;						// car speed [m/s]  of participants

	// Timing  best / last / current laps and deltas
	int32_t bestLapNum;					// Lap number of best lap
	float   bestLapTime;				// Best lap time (PB)
	float   lastLapTime;				// Last completed lap time

	float   currentLapTime;				// Current ongoing lap time
	float   estimatedLapTime;			// Estimated time to complete the current lap 
	float   estimatedRaceFinishTime;    // Estimated time to finish the race

	float   deltaTimeVersusPBNative;	// Seconds. native delta between current time and best lap time (PB) from the game
	float   deltaTimeVersusPB;			// Seconds. delta between current time and personal best (from OpenSR not the game)

	float   deltaTimeLastLap;			// Seconds. Delta Time to session last lap
	float   deltaTimeSessionBestLap;	// Seconds. Delta Time to session best
	float   deltaTimeOptimalLap;		// Seconds. Delta Time to optimal lap
	float   deltaTimeSessionOptimalLap;	// Seconds. Delta Time to session optimal lap

	float   deltaTimeVersusClassLeader;	// Seconds. delta between class leader time or last lap time of current class leader
	float   deltaTimeVersusLeader;		// Seconds. delta between leader time or last lap time of current leader
	float   deltaTimeVersusFront;		// Seconds. delta between current time and front driver time
	float   deltaTimeVersusBehind;		// Seconds. delta between current time and driver behind time

	int32_t lapsBehindFront;			// laps behind vehicle in front (next higher place)
	int32_t lapsBehindLeader;			// laps behind leader

	// Sector times (current / last / best)
	float curSectorTime[16];   // current sectors
	float lastSectorTime[16];  // last sectors
	float bestSectorTime[16];  // best sectors
	
	// Penalties / infractions
	int8_t numPenalties;			// Number of penalties applied to this driver
	int8_t numTeamPenalties;		// Team incident count
	float   penaltiesDuration;		// total penalties accumulated in sec
	int32_t penaltyType;			// penalties (sim specific bitfield)
									//      drive-through penalty,
									//      stop & go penalty,
									//      time penalty,
									//      disqualified,
									//      penalty cleared flag, etc.
	int32_t penaltyCauseId;			// (sim specific) ID of Infringement type
	int8_t cutDriveThroughCount;	// accumulated cut-track penalties
	int8_t cutStopAndGoCount;		//
	int8_t cutPitStopCount;			//
	int8_t cutTimeDeductionCount;	//
	int8_t cutSlowDownCount;		//

	int8_t tyreVisualCompound;		// (sim specific) example: none,prime, option, soft, med, hard
	int8_t tyreCompound;			// Tyre compound enum (sim unit)
									// Example (FIA-style): 0=ultrasoft, 1=supersoft, 2=soft, 3=medium,
									// 4=hard, 5=intermediate, 6=wet
									// Meaning defined by the target sim/game
	int8_t rcJockerCount;			// rally cross number of jocker already done
	uint8_t _padding[1];

	// Human-readable strings
	char driverName[64];            // Driver name
	char driverNickname[64];        // Nickname
	char teamName[64];              // Team name
	char carName[64];               // Full car name
	char className[32];             // car class name
} PlayerInfo_t;

static_assert(sizeof(PlayerInfo_t) == 672, "PlayerInfo_t must be 672 bytes size/alignment is incorrect");

typedef struct PlayerRecords_t {
    int32_t reserved;
	int32_t carCount;
    PlayerInfo_t player[OPENSR_MAX_PLAYERS];
} PlayerRecords_t;

static_assert(sizeof(PlayerRecords_t) == 64520, "PlayerInfo_t must be 64520 bytes size/alignment is incorrect");


typedef struct WheelInfo_t {
	// Tyre Compound / Type 
	int8_t tyreBrand;				// 0=Pirelli, 1=Michelin, etc.
	int8_t tyreVisualCompound;		// (sim specific) example: none,prime, option, soft, med, hard
	int8_t tyreCompound;			// Tyre compound enum (sim unit)
									// Example (FIA-style): 0=ultrasoft, 1=supersoft, 2=soft, 3=medium,
									// 4=hard, 5=intermediate, 6=wet
									// Meaning defined by the target sim/game

	// Pressures (kPa) 
	float tyrePressure;				// Current live tyre pressure [kPa]
	float tyrePressureCold;			// Cold setup pressure [kPa] (reference at pit/setup screen)
	float tyrePressureLastHot;		// Last recorded hot pressure [kPa] (useful post-stint data)

	// Temperatures (°C) 
	float tyreTempOverall;			// Average of inner/middle/outer surface temps [°C]
	float tyreTempInner;			// Inner edge surface temperature [°C]
	float tyreTempMiddle;			// Middle tread surface temperature [°C]
	float tyreTempOuter;			// Outer edge surface temperature [°C]
	float tyreTempCarcass;			// Carcass bulk ("core") temperature [°C]; deeper than surface temps
	float tyreTempCore;				// Average of tyre Inner Layer temps [°C]
	float tyreTempInnerLayer[3];	// Left/center/right inner layer temps [°C]

	// Tyre Wear / Tread 
	float tyreWearOverall;			// Normalised wear 0.0(new) → 1.0(worn out)
	float tyreWearInner;			// Segment wear – inner [0..1]
	float tyreWearMiddle;			// Segment wear – middle [0..1]
	float tyreWearOuter;			// Segment wear – outer [0..1]
	int32_t tyreFlatSpot;			// Flatspot severity level (sim-defined metric)
	float tyreDirtyLevel;			// Dirt/marble pickup level [0.0 clean → 1.0 fully dirty]
	float tyreOnSurfaceType;		// surface type under car tyre,tarmac, dirt, grass, gravel, sand, rumble,...

	// Wheel state
	float wheelSpeed;				// Linear ground speed of the wheel [m/s]
	float wheelRotationSpeed;		// Wheel rotation rate [rad/s]
	float wheelRadius;				// Effective rolling radius under load [m]

	// Contact patch velocities
	float patchVelocityLat;			// Lateral velocity at contact patch [m/s]
	float patchVelocityLong;		// Longitudinal velocity at contact patch [m/s]

	// Ground velocities
	float groundVelocityLat;		// Lateral velocity at contact patch relative to ground [m/s]
	float groundVelocityLong;		// Longitudinal velocity at contact patch relative to ground [m/s]
	float lateralForce;				// lateral force [N]
	float longitudinalForce;		// longitudinal force [N]

	// Slip metrics
	float wheelSlipRatio;			// Longitudinal slip ratio (dimensionless)
	float wheelSlipAngle;			// Lateral slip angle [rad]
	float wheelCombinedSlip;		// Combined slip metric (model-dependent)

	// Grip and deflection
	float tyreGrip;					// Current grip coefficient (effective µ)
	float verticalTyreDeflection;	// Tire deflection from unloaded radius [m]

	// Load and contact geometry
	float tyreLoad;					// Vertical load on tyre [N]
	float contactPatch;				// Contact patch area [m²]
	float contactPoint[3];			// World position of contact [x,y,z]
	float contactNormal[3];			// Contact surface normal vector [x,y,z]
	float contactHeading[3];		// Forward direction of contact patch [x,y,z]

	// Alignment / Geometry 
	float tyreRadius;				// Nominal static tyre radius [m] (construction/reference size)
	float wheelCamberDeg;			// Camber angle at wheel [deg]
	float wheelToe;					// Toe angle [deg or rad depending on sim]
	float wheelCornerWeight;		// Static sprung weight on this corner [N]

	// Surface / Usage / state
	float wheelRumblePitch;			// frequency in Hz of the tire vibrating over a rumble strip
	int32_t wheelSurfaceId;			// Track surface material ID (sim-specific mapping)
	int8_t wheelOnRumbleStrip;		// on rumble strip
	int8_t wheelInPuddleDepth;		// in puddle strip normalized depth 
	int8_t SurfaceRumble;			// typical normalized 0.0 to 1.0 surface rumble (asphalt, kerb, gravel, grass, airborne, etc.)
	int8_t tyreStintIndex;			// Stint index (increments when tyre set changes)
	uint8_t tyreFlat;				// tyre flat 0/1
	uint8_t tyreDetached;			// tyre detached 0/1

									// settings
	int8_t setupTyreWearRate;		// tyre wear rate (sim specific)
	float  chassisYaw;				// Yaw angle of the chassis relative to the direction of motion - radians
	uint8_t _padding[2];
} WheelInfo_t;

static_assert(sizeof(WheelInfo_t) == 216, "wheelInfo must be 216 bytes size/alignment is incorrect");

typedef struct WheelData_t {
	WheelInfo_t wheels[TYRE_INDEX_MAX];
} WheelData_t;

static_assert(sizeof(WheelData_t) == 864, "WheelData must be 832 bytes size/alignment is incorrect");

typedef struct BrakeData_t {
	// pressure
	float     brakePressure;		//  pressure [kPa or percent]

	// Temperatures
	float     brakeDiscTemp;         // current disc temp [°C]
	float     brakePadTemp;          // pad temp [°C]

	// Wear
	float     brakePadWear;          // normalized 0..1
	float     brakeDiscWear;         // normalized 0..1

	// Torque / force
	float     brakeTorque;           // torque applied at wheel [Nm]

	// Status / flags
	int8_t   brakeOverheated;       // 1 = overheated
	uint8_t _padding[7];
} BrakeData_t;

static_assert(sizeof(BrakeData_t) == 32, "WheelData must be 32 bytes size/alignment is incorrect");

typedef struct SuspensionData_t {
	// Deflection / position
	float suspDeflection;       // suspension travel [m]
	float suspTravelNormalized; // suspension travel normalized [0.0 to 1.0]
	float suspDeflectionMax;    // max deflection [m]
	float suspDeflectionMin;    // min deflection [m]

	// Velocities
	float suspVelocity;         // suspension velocity [m/s]
	float damperVelocity;       // damper shaft velocity [m/s]

	// Forces
	float suspForce;            // vertical suspension force [N]
	
	// Ride height
	float rideHeight;           // ride height at this wheel [m]

	// Alignment
	float suspCamberDeg;        // camber [°]
	float suspToeDeg;           // toe [°]
	float suspCasterDeg;        // caster [°]
	float _padding;
} SuspensionData_t;

static_assert(sizeof(SuspensionData_t) == 48, "SuspensionData must be 48 bytes size/alignment is incorrect");

typedef struct CarDamageData_t {
	int8_t damageZone[8];		  // example: front 0, rear 1, left 2, right 3, centre 4
	// Engine & drivetrain damage
	int8_t damageEngine;          // engine state 0=ok, >0 damaged
	int8_t damageEngineBlown;     // 1 if blown
	int8_t damageEngineSeized;    // 1 if seized
	float  damageEngineWear;      // wear 0..1
	float  damageEngineOverrev;   // accumulated over-rev wear
	
	int8_t damageTransmission;    // gearbox state
	float  damageGearboxWear;     // gearbox wear 0..1
	int8_t damageClutch;          // clutch damage state
	float  damageClutchWear;      // wear 0..1
	int8_t damageDifferential;    // differential state

	// Chassis / aero
	float damageChassis;         // global chassis integrity 0..1
	float damageAero;            // aero integrity 0..1
	float damageFrontWing;       // front aero element damage %
	float damageRearWing;        // rear aero element damage %
	float damageFloor;           // floor damage %

	// Suspension / steering damage
	float damageSuspensionLF;    // LF suspension integrity %
	float damageSuspensionRF;    // RF suspension integrity %
	float damageSuspensionLR;    // LR suspension integrity %
	float damageSuspensionRR;    // RR suspension integrity %
	float damageSteering;        // steering column/rack damage %

	// Body / cosmetic
	int8_t damageBodywork;        // bodywork state
	float  damageBodyIntegrity;   // normalized 0..1
	float  damageScratch;         // scratch level %
	float  damageDent;            // dent level %
	// brakes
	int8_t damageBrake[4];         // 1 = failed / damaged
	int8_t damageTyre[4];		  // Tyre damage (percentage)
	int8_t damageExhaust;		// 0=ok, 1=damaged, 2=broken
	int8_t spare[4];
} CarDamageData_t;

static_assert(sizeof(CarDamageData_t) == 96, "CarDamageData must be 88 bytes size/alignment is incorrect");

enum CarSignal : uint32_t {
	CS_NONE = 0,

	// -------------------------------------------------
	// Exterior Lighting & Visibility
	// -------------------------------------------------
	CS_LEFT_TURN = 1u << 0,  // Left turn indicator active
	CS_RIGHT_TURN = 1u << 1,  // Right turn indicator active
	CS_HAZARD = 1u << 2,  // Hazard warning flashers active
	CS_HORN = 1u << 3,  // Horn / audible warning active
	CS_LOW_BEAM = 1u << 4,  // Dipped beam headlights active
	CS_HIGH_BEAM = 1u << 5,  // Main beam (brights) toggled on
	CS_FOG_FRONT = 1u << 6,  // Front fog lamps enabled
	CS_FOG_REAR = 1u << 7,  // Rear high-intensity fog lamp enabled
	CS_PARKING_LIGHTS = 1u << 8,  // Position / side lights active
	CS_DRL = 1u << 9,  // Daytime Running Lights active
	CS_BRAKE_LIGHT = 1u << 10, // Main brake lamps currently illuminated
	CS_REVERSE_LIGHT = 1u << 11, // Reverse gear indicator lamp active
	CS_WIPERS_ACTIVE = 1u << 12, // Front windshield wipers operating
	CS_REAR_WIPER = 1u << 13, // Rear window wiper operating
	CS_BEACON = 1u << 14, // Roof-mounted warning beacon active

	// -------------------------------------------------
	// Driving & Cruise Assists
	// -------------------------------------------------
	CS_CRUISE_CONTROL_ACTIVE = 1u << 15, // Speed cruise control system engaged
	CS_SPEED_LIMIT_WARNING = 1u << 16, // Vehicle has exceeded set dash speed limit
	CS_AUTOPILOT_ACTIVE = 1u << 17, // Semi-autonomous steering/driving active
	CS_LANE_ASSIST_ACTIVE = 1u << 18, // Lane departure/keeping system active

	// -------------------------------------------------
	// EV / Hybrid Specific (Lamp Status)
	// -------------------------------------------------
	CS_CHARGING_CONNECTED = 1u << 19, // EV charging cable is physically plugged in
	CS_CHARGING_ACTIVE = 1u << 20, // Battery is currently drawing power from grid
	CS_CHARGE_COMPLETE = 1u << 21, // Charging cycle is finished
	CS_CHARGE_FAULT = 1u << 22, // Malfunction detected in charging hardware
	CS_POWER_LIMIT_ACTIVE = 1u << 23, // Performance capped (low SoC or battery heat)

	// -------------------------------------------------
	// Heavy Vehicle / Truck Drivetrain
	// -------------------------------------------------
	CS_ENGINE_BRAKE_ACTIVE = 1u << 24, // Engine/Exhaust brake system currently braking
	CS_RETARDER_ACTIVE = 1u << 25, // Driveline retarder (hydraulic/electric) active
	CS_DIFF_LOCK_ACTIVE = 1u << 26, // Differential lock engaged for traction
	CS_LIFT_AXLE_ACTIVE = 1u << 27, // Tag/Mid-lift axle currently in raised position
	CS_ADBLUE_LOW = 1u << 28, // SCR fluid (AdBlue) is low indicator

	// -------------------------------------------------
	// Mechanical / Safety (Core Indicators)
	// -------------------------------------------------
	CS_PARKING_BRAKE = 1u << 29, // Mechanical/Electronic parking brake applied
	CS_IGNITION_ON = 1u << 30, // Vehicle ignition/electrics powered on
	CS_HIGH_BEAM_FLASH = 1u << 31  // Flash-to-pass (momentary high beam) active
};

typedef struct WarningFlag_t {
	// --- Race control flags ---
	int8_t	 greenFlagWarning;		// Green flag shown
	int8_t	 redFlagWarning;			// Red flag shown
	int8_t	 yellowFlagWarning;		// Yellow flag shown
	int8_t	 fullYellowFlagWarning;	// Full Yellow flag shown (Safety Car/FYC)
	int8_t	 redlineWarning;			// Engine at/over redline
	int8_t	 blueFlagWarning;		// Blue flag (faster car approaching / lapping)
	int8_t	 blackFlagWarning;		// Black flag (disqualified / pit immediately)
	int8_t	 whiteFlagWarning;		// White flag (slow vehicle on track / final lap)
	int8_t	 checkeredFlagWarning;	// Checkered flag (end of race/session)
	int8_t	 meatballFlagWarning;	// Black with orange circle (mechanical issue, pit required)
	int8_t	 blackWhiteFlagWarning;	// Black/white diagonal (unsportsmanlike conduct warning)
	int8_t	 surfaceFlagWarning;		// Yellow/red striped (slippery surface, debris, fluids)
	int8_t	 orangeFlagWarning;		// orange flag / debris flag

	int16_t flagsWarning;			// Other flags (bitfield, extended flags)													   

	// --- Safety car / race management ---
	int8_t isSafetyCar;       // safety car deployed
	int8_t isSafetyCarPassing;// permission to pass safety car
	int8_t isVSC;             // virtual safety car active

	// Systems / safety aids / signals
	int8_t padWearWarning;			// Brake pad wear threshold exceeded

	int8_t turboStatus;			// Turbo status/flag (sim specific)
	int8_t headlights;				// 1 = headlights on
	int8_t startLights;			// Start light state (0 = off, 1..n = lights on)
	int8_t espSignal;
	uint32_t carSignals;			// check CarSignal enum
	// pit
	int8_t pitLimiterActive;    // pit speed limiter active
	int8_t pitRequest;			// driver requested pit stop flag

	int8_t tcKillSwitch;			// master traction control kill switch on/off
	int8_t numCutTrackWarning;
	int8_t shiftIndicator;			// OSP normalized 0.0 to 1.0,  0.98-1.0 == optimal shiftpoint

	// General warnings
	int8_t engineWarning;			// Engine warning 0/1 or bitfield (overheat, malfunction, etc.)
	int8_t engineStallWarning;
	int8_t waterWarning;			// Coolant/water temperature warning
	int8_t oilWarning;				// Oil pressure/temperature warning
	int8_t overHeatWarning;			// other than engine
	int8_t fuelWarning;				// Low fuel warning
	int8_t voltageWarning;			// battery Low/high voltage warning
	int8_t damageWarning;			// General damage warning
	int8_t detachedParts;			// Car has detached parts (wings, wheels, etc.)
	int8_t brakeWarning;			// Brake system warning
	int8_t clutchWarning;			// clutch warning
	int8_t gearboxWarning;			// gearbox failure warning
	int8_t tyreWarning;			// tyre warning
	int8_t transmissionWarning;	// transmission warning
	int8_t penaltyWarning;     		// official warning flag

	int8_t revLightsPercent;		// rev lights in percent

	uint8_t     m_activeAeroMode;                   // 0 = Corner mode, 1 = Straight mode
	uint8_t     m_activeAeroAvailable;              // 0 = not available, 1 = available
	uint16_t    m_activeAeroActivationDistance;     // 0 = Active aero not available, non-zero - Active aero will be available in [X] metres
	uint8_t     m_overtakeAvailable;                // 0 = not available, 1 = available
	uint8_t     m_overtakeActive;                   // 0 = not active, 1 = active
	uint16_t    m_overtakeActivationDistance;       // 0 = Overtake Mode not available, non-zero - Overtake Mode will be available in [X] metres
	uint8_t     m_2026Regulations;                  // 0 = vehicle conforms to pre-2026, 1 = 2026 regulations applicable
	uint8_t     m_drivingWrongWay;                  // Whether the car is driving the wrong way
	uint8_t _padding[6];
} WarningFlag_t;

static_assert(sizeof(WarningFlag_t) == 64, "WarningFlag must be 58 bytes size/alignment is incorrect");

typedef struct EnergySystemData_t {
	// Generic ERS/KERS state
	float ersStoreEnergy;				// J - current stored energy in ES (battery/supercap)
	float ersStoreEnergyMax;			// J - maximum storable energy
	float ersStoreEnergyPercent;		// % - convenience value (0–100), derived if not provided

	// Deployment
	float ersDeployPower;				// W - current deploy power output (instantaneous)
	float ersDeployedThisLap;			// J - total deployed energy this lap
	float ersDeployLimitPerLap;			// J - max deployable energy per lap (if rule-limited)
	int8_t ersDeployMode;				// mode selector (0=auto, 1=hotlap, 2=qualy, etc.)
	int8_t ersAllowDeploy;				   // flag: deployment available/allowed right now

	// Harvesting
	float ersHarvestedThisLapMGUK;		// J - harvested via MGU-K this lap
	float ersHarvestedThisLapMGUH;		// J - harvested via MGU-H this lap
	float ersHarvestedThisLapOther;		// J - harvested via regen braking, etc.
	float ersHarvestPowerMGUK;			// W - current harvest power from MGU-K
	float ersHarvestPowerMGUH;			// W - current harvest power from MGU-H

	float ersTorqueMGUK;

	// KERS (legacy systems, pre-ERS era)
	float kersStoreEnergy;				// J - current KERS energy store
	float kersStoreEnergyMax;			// J - max KERS capacity (e.g., 400 kJ in F1 2009–2013)
	float kersStorePercent;				// % - derived (0–100)
	float kersDeployPower;				// W - current deploy power
	float kersDeployedThisLap;			// J - deployed via KERS this lap
	int8_t kersActvated;				// is driver pressing KERS button
	int8_t kersAvailable;				// true if usable at this moment (not cooling/locked out)

	// Thermal & Limits
	float ersBatteryTemp;				// °C - battery temperature
	float ersMGUKTemp;					// °C
	float ersMGUHTemp;					// °C
	float ersEfficiency;				// ratio (0–1), system efficiency if provided

	// Flags
	int8_t hasERS;						// car has ERS (0/1)
	int8_t ersFault;					// fault/error present
	int8_t ersOverheated;				// limited due to temperature
	int8_t ersHarvestingActive;		// currently harvesting
	int8_t ersDeployingActive;			// currently deploying
	int8_t hasKERS;					// car has KERS (0/1)
	int8_t ersRecoveryMode;			// (0–13 usually) indicating the setting of the MGU-K regen (how much drag the motor gives under braking)
	uint8_t _padding[1];
} EnergySystemData_t;

static_assert(sizeof(EnergySystemData_t) == 96, "EnergySystemData must be 96 bytes size/alignment is incorrect");

typedef struct CarSetupState_t {
	// Differential (normalized or raw index)
	float diffEntry;		// 0..1 or setup index
	float diffMiddle;		// 0..1 or setup index
	float diffExit;			// 0..1 or setup index

	// Engine / ECU
	float engineMap;		// engine power / ECU map index
	float engineBraking;	// engine braking map index
	float throttleShape;	// throttle map / pedal curve (dcThrottleShape)

	// Anti-roll bars
	float frontAntiRollBar; // setup index or normalized
	float rearAntiRollBar;

	// Weight jacker
	float weightJackerLeft;  // turns or mm
	float weightJackerRight;

	// Aero
	float frontWing;        // flap angle / step
	float rearWing;

} CarSetupState_t;

static_assert(sizeof(CarSetupState_t) == 48, "CarSetupControl_t must be 48 bytes size/alignment is incorrect");

//=====================================================================
// VEHICLE DATA, telemetry of the car you are currently driving
//=====================================================================
typedef struct VehicleData_t {
	float speed;                    // car speed [m/s]

	// Transmission / gearbox
	int8_t  gear;                   // -1 = reverse, 0 = neutral, >0 forward
	uint8_t maxGear;                // number of forward gears
	uint8_t gearIndexNeutral;       // 1 if neutral index flagged
	uint8_t gearIndexReverse;       // 1 if reverse index flagged
	float   transmissionSpeed;      // transmission output speed / ratio

	// Pedals / driver inputs
	float throttle;                 // normalized [0..1]
	float throttleRaw;              // raw input
	float throttleTravel;           // pedal travel
	float brake;                    // normalized brake input [0..1]
	float brakeRaw;                 // raw brake
	float handbrake;                // normalized handbrake
	float handbrakeRaw;             // raw handbrake input
	float clutch;                   // normalized clutch [0..1]
	float clutchRaw;                // raw clutch (rpm)
	float clutchTravel;             // clutch travel (if available)


	//  Engine / powertrain
	float   rpm;                    // engine RPM
	float   engineTorque;           // Nm (if provided)
	float   idleRpm;                // idle RPM
	float   maxRpm;                 // redline
	float   engineTemp;             // temperature [C]
	uint8_t ignitionOn;             // 0/1
	uint8_t driverTrainType;        // (sim unit) example: 0 = FWD, 1 = RWD, 2 = AWD
	uint8_t NumCylinders;			// number of cylinders in the engine
	uint8_t antiStallActivated;		// anti-stall activated 0/1

	// Temperatures & pressures
	float oilTemp;                  // Oil temperature [C]
	float oilPress;                 // Oil pressure (kPa)
	float waterTemp;                // Water / coolant temp [C]
	float waterLevel;               // Water level [L or unit provided]
	float exhaustTemp;              // Exhaust temperature [C]
	float fuelPressure;             // Fuel pressure [kPa]
	float voltage;                  // Electrical system voltage [V]
	float batterySOC;               // battery state-of-charge (0..1)


	// Fuel system
	float fuelLevel;                // liters
	float fuelLevelPct;             // 0..1
	float fuelCapacity;             // liters max
	float fuelUsePerHour;           // L/h
	float fuelPerLap;               // estimated L per lap
	float fuelEstimatedLaps;        // remaining laps estimate
	int8_t setupFuelRate;			// consumption rate (sim specific)
	int8_t fuelMix;					// Fuel mixture modes (lean, normal, rich, etc.)

	// Turbo / boost / forced-induction
	float turboLevel;               // current turbo pressure (bar or sim unit)
	float maxTurbo;                 // max turbo capability (sim-specific)
	float boost;                    // current effective boost (generic)
	float maxBoost;                 // absolute max effective boost (generic cross-tech ceiling)
	float maxTorque;                // highest torque peak anywhere on the torque curve
	float maxPower;                 // the highest power peak

	//  ABS / Traction Control / stability aids
	int8_t   antiLockBrakes;       // ABS setup value
	int8_t   absInAction;          // ABS real time currently active (0/1)

	int8_t   tractionControl;		// TC setup value
	int8_t   tractionControl2;		// TC aux setup value
	int8_t   tcInAction;			// TC real time currently acting (0/1) or TCCUT counter/value Real-time TC value [0/100%]


	// Brake bias
	float   brakeBias;				// overall brake bias (front/rear or percent)
	float   frontBrakeBias;			// explicit front bias value if available
	float   rearBrakeBias;			// explicit rear bias value if available

	// DRS
	int8_t hasDRS;					// car supports DRS (0/1)
	int8_t drsAllowed;				// allowed to use DRS (0/1)
	int8_t drsEnabled;				// DRS system enabled flag
	int8_t drsEngaged;				// currently engaged
	int8_t drsState;				// drs state (enum)
	int8_t drsNumActivationsLeft;	// activations left
	int8_t drsNumActivationsTotal;	// total activations
	int16_t drsActivationDistance;	// activation distance (m)

	// PUSH-TO-PASS
	int8_t hasPTP;					// car supports Push-To-Pass
	int8_t ptpEngaged;				// currently engaged
	int8_t ptpActivationLeft;		// activations left
	int8_t ptpActivationTotal;		// ptp total activations 
	uint8_t _padding[3];

	//  Global aerodynamics / chassis geometry
	float currentDownforce;				// total downforce (sim value)
	float frontWingHeight;				// aero setup scalar
	float frontRideHeight;				// global front ride height (m)
	float rearRideHeight;				// global rear ride height (m)
	float frontRollAngle;				// Roll angle of the front suspension
	float rearRollAngle;				// Roll angle of the rear suspension
	float frontThirdSpringDeflection;	// [m] deflection at front 3rd spring
	float frontThirdSpringVelocity;		// [m/s] velocity at front 3rd spring (if available)
	float rearThirdSpringDeflection;	// [m] deflection at rear 3rd spring
	float rearThirdSpringVelocity;		// [m/s] velocity at front 3rd spring (if available)
	float cgHeight;						// center of gravity height (m)
	float heightOfCOGAboveGround;		// alternate naming, if present
	float carLength;					// [m]
	float carWidth;						// [m]
	float totalMass;					// [kg] Car + penalty weight + fuel

	//  Steering / wheel
	float steer;                    // steering input [-1..1]
	float steerLock;                // current lock angle full left to right (radians or degrees per sim)
	float steerAngle;               // current wheel angle (radians or degrees per sim)
	float steerPeakForceNm;         // peak force reading (Nm)
	float steeringTorque;			// torque (Nm)
	uint8_t steeringPctTorque;		// torque percent (0/100)

	//  Mileage / counters
	float odometerKm;               // total km (if available)
	float totalDistance;            // totalDistance

	// motion platform vibration helpers
	float kerbVibration; // vibrations sent to the FFB, could be used for motion rigs
	float slipVibrations; // vibrations sent to the FFB, could be used for motion rigs
	float gVibrations; // vibrations sent to the FFB, could be used for motion rigs
	float absVibrations; // vibrations sent to the FFB, could be used for motion rigs

	// Sub-systems (arrays)  keep exactly as in the original file
	BrakeData_t			brakes[TYRE_INDEX_MAX];
	SuspensionData_t	suspensions[TYRE_INDEX_MAX];
	CarDamageData_t		carDamage;
	WarningFlag_t		warningFlag;
	EnergySystemData_t	energySystem;   // embed energy system (ERS/KERS/MGU-K/MGU-H) size
	CarSetupState_t		carSetupState;
} VehicleData_t;

static_assert(sizeof(VehicleData_t) == 920, "VehicleData must be 896 bytes size/alignment is incorrect");

typedef struct ExtensionData_t {
	float extension[256];
} ExtensionData_t;

static_assert(sizeof(ExtensionData_t) == 1024, "Extension must be 1024 bytes size/alignment is incorrect");

typedef struct OutSimData {
	// general info, status and flags

	PacketHeader_t			mPacketHeader;		
	PlayerRecords_t			mPlayers;			
	MotionData_t			mMotionData;		
	SessionData_t			mSessionData;		
	VehicleData_t			mVehicleData;		
	WheelData_t				mWheelData;			
	ExtensionData_t			mExtensionData;		
} OutSimData; //full struct in OutData.h

static_assert(sizeof(OutSimData) == 68336, "OutSimData bytes size / alignment is incorrect");


/*
PacketHeader 32
-------------
MotionData 448
SessionData 528
PlayerInfo 688
PlayerRecords 64520
VehicleData 920
Wheels x4 864
ExtensionData 1024
------ SUB -------
WheelInfo 216
mEnergySystem 96
CarDamageData 96
BrakeData 32
SuspensionData 48
warningFlag 64
EnvironmentData 88
OutSimData 68336
BlobData 32808
*/

#pragma pack(pop)

#endif _OpenSR_OUTSIMDATA_H


