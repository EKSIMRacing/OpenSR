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
    

    //============================================================================
    // Motion data
    //============================================================================

    public enum DataType : byte
	{
		DtMotionData,
		DtSessionData,
		DtVehicleStateData,
		DtWheelData,
		DtPlayersData,
        DtExtensionData
	}
    /*
	switch (type)
	{
		case DataType.DtMotionData:
			// ...
			break;
		case DataType.DtSessionData:
			// ...
			break;
	}
	*/

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public unsafe struct MotionData // Must be 'unsafe' to contain a fixed buffer
    {
        // local acceleration [m/s?]
        public double localAccelX;
        public double localAccelY;
        public double localAccelZ;

        // Local velocity [m/s]
        public double localVelocityX;
        public double localVelocityY;
        public double localVelocityZ;

        // Angular velocity [rad/s]
        public double angularVelocityX;
        public double angularVelocityY;
        public double angularVelocityZ;

        // Angular acceleration [rad/s?]
        public double angularAccelX;
        public double angularAccelY;
        public double angularAccelZ;

        // Orientation (radians)
        public double pitch;
        public double yaw;
        public double roll;

        // Orientation rates [rad/s]
        public double pitchRate;
        public double yawRate;
        public double rollRate;

        // G-forces
        public double gForceLat;       // lateral G
        public double gForceVert;      // vertical G
        public double gForceLong;      // longitudinal G

        // world velocity [m/s]
        public double VelocityX;
        public double VelocityY;
        public double VelocityZ;

        // location of car's center of gravity in world space
        public double carCGLocX;
        public double carCGLocY;
        public double carCGLocZ;

        // Rotation matrix (car orientation in world frame)
        public fixed double RotMat[9]; // Flattened 3x3 - rotMat[3][3]

        // Updated indexer to work with the fixed buffer
        public double this[int row, int col]
        {
            get
            {
                // We must "pin" the fixed buffer to get a pointer to it for safe access.
                fixed (double* pRotMat = RotMat)
                {
                    return pRotMat[row * 3 + col];
                }
            }
            set
            {
                fixed (double* pRotMat = RotMat)
                {
                    pRotMat[row * 3 + col] = value;
                }
            }
        }
        // usage: float pitchComponent = carData[2, 0];


        // World space up direction
        public double upDirY;
        public double upDirX;
        public double upDirZ;

        // World space right direction
        public double rightDirX;
        public double rightDirY;
        public double rightDirZ;

        // World space forward direction
        public double frwdDirX;
        public double frwdDirY;
        public double frwdDirZ;

        // Velocity in world space
        public double worldVelocityX;
        public double worldVelocityY;
        public double worldVelocityZ;

        // Suspension deflection [m]
        public double suspensionDeflectionLF;
        public double suspensionDeflectionRF;
        public double suspensionDeflectionLR;
        public double suspensionDeflectionRR;

        // Suspension velocity [m/s]
        public double suspensionVelocityLF;
        public double suspensionVelocityRF;
        public double suspensionVelocityLR;
        public double suspensionVelocityRR;
    }

    //--------------------------------------
    // EnvironmentData
    //--------------------------------------

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public unsafe struct EnvironmentData // Must be 'unsafe'
    {
        //  Time & lighting
        public float timeOfDay;     // seconds since midnight
        public float timeScale;     // time acceleration factor
        public float sunAltitude;       // sun height angle [deg]
        public float sunAzimuth;        // sun azimuth angle [deg]
        public float cloudLevel;        // fog density [0..1]
        public float fogLevel;          // fog density [0..1]

        //  Atmosphere
        public float airTemp;           // ambient air temperature [?C]
        public float airPressure;       // barometric pressure [kPa or hPa]
        public float humidity;          // relative humidity [%]
        public float airDensity;        // air density [kg/m?]

        //  Wind
        public float windSpeed;     // magnitude [m/s]
        public float windDirection; // heading [deg from north]

        //  Precipitation
        public float rainLevel;     // rain intensity [0..1]
        public float trackWetness;      // water on track [0..1]
        public float puddleLevel;       // puddle accumulation [0..1]
        public float snowLevel;     // snow intensity [0..1] if supported

        //  Track surface condition
        public float trackTemp;     // track surface temperature [?C]
        public float trackGrip;     // grip coefficient / rubber level
        public float trackDust;     // dust level [0..1]
        public float trackMarbles;      // marbles level [0..1]
        public float trackOil;          // oil/dirt contamination [0..1]
        public byte weatherFlag;		// (sim specific) like 0 = clear, 1 = light cloud, 2 = overcast, 3 = light rain, 4 = heavy rain, 5 = storm

        // Replaced the managed byte[] with a fixed buffer
        public fixed byte _padding[3];
    }


    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public unsafe struct AidSettings // Must be 'unsafe'
    {
        public sbyte assitAbs;              // ABS; -1 = N/A, 0 = off, >= 1 currently active
        public sbyte assitTc;                   // TC; -1 = N/A, 0 = off, >= 1 currently active
        public sbyte assitEsp;              // ESP; -1 = N/A, 0 = off, >= 1 currently active
        public sbyte assitCountersteer;     // Countersteer; -1 = N/A, 0 = off, >= 1 currently active
        public sbyte assitCornering;            // Cornering; -1 = N/A, 0 = off, >= 1 currently active
        public sbyte assitStability;            // 0 (off), >= 1 currently active (sim specific)
        public sbyte assistDRS;               // 0 = off, 1 = on
        public sbyte assistERS;               // 0 = off, 1 = on
        public sbyte assistBrake;             // 0 = off, 1 = low, 2 = medium, 3 = high 
        public sbyte aiLevel;					// AI Level from 0 to 255

        public sbyte penaltiesEnabled;      // 0 (off), >= 1 enabled
        public sbyte setupFuelRate;         // consumption rate (sim specific)
        public sbyte tyreWearEnabled;       // Flag: 1 = wear model active, 0 = disabled
        public sbyte setupTyreWearRate;     // tyre wear rate (sim specific)
        public sbyte damageEnabled;         // damages allowed
        public sbyte setupDamageRate;       // car damage rate (sim specific)
        public sbyte allowTyreBlankets;     // starts with hot (optimal temp) tyres
        public sbyte autoShift;             // 0 (off), 1 (upshifts), 2 (downshifts), 3 (all)
        public sbyte autoClutch;            // 0 (off), 1 (on)
        public sbyte autoPit;               // 0 (off), 1 (on)
        public sbyte autoLift;              // 0 (off), 1 (on)
        public sbyte autoBlip;              // 0 (off), 1 (on)
        public sbyte autoReverse;           // 0 (off), 1 (on)
        public sbyte HoldClutch;            // for auto-shifters at start of race: 0 (off), 1 (on)
        public sbyte numStarters;
        public sbyte standingStart;         // 1 standing, 0 rolling
        public sbyte startingGrid;          // enum single file, double file, etc.
        public sbyte hardcoreLevel;         // realism level (sim unit)
        public sbyte incidentLimit;         // -1 = unlimited

        // Replaced the managed byte[] with a fixed buffer
        public fixed byte _padding[3];
    }

    //=====================================================================
    // SESSION DATA  information that applies to the whole event
    //=====================================================================
    //=====================================================================
    // SESSION DATA  information that applies to the whole event
    //=====================================================================
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public unsafe struct SessionData // Must be 'unsafe' to contain fixed buffers
    {
        // Core session identifiers & state
        public sbyte sessionType;           // 0 = Practice, 1 = Qualify, 2 = Race
        public sbyte sessionPhase;          // 0 = Garage, 1 = Gridwalk, 2 = Formation, 3 = Green, etc.
        public sbyte sessionMode;           // enum full, replay, etc.
        public sbyte sessionEventId;            // event id
        public sbyte sessionFormat;         // Laps or Time based or ...
        public sbyte sessionUniqueId;       //
        public sbyte sessionFlags;			// session flags detailed in warningFlags section

        public sbyte category;              // enum Road, Oval, etc.
        public sbyte safetyCarStatus;       // 0 = none, 1 = full, 2 = VSC
        public int numLaps;             // unlimited/time based = -1
        public sbyte numSectors;                //

        // (byte[] replaced with fixed buffer)
        public fixed byte sessionSectorYellow[16];      // sector marked yellow on current track

        public sbyte pitWindowStatus;       // unavailable, close, open, etc.

        // Session timing / tick
        public float sessionFrame;         // frame tick
        public float sessionTimeOfDay;     // seconds since midnight
        public float sessionTimeRemain;    // seconds remaining in session
        public float sessionTime;          // session time total/elapsed
        public int lapsRemain;           // Session Laps Remain

        // Track information (static for the session)
        public float trackLength;          // m
        public float trackAltitude;        // meters
        public float trackLatitude;        // degrees
        public float trackLongitude;       // degrees
        public float trackNorthOffset;     // radians
        public float pitSpeedLimit;        // kph pit limit on this track
        public int trackId;              // unique Id
        public int trackLayoutId;        // layout/config Id
        public int numTurns;

        // Event / weekend identifiers
        public int seriesId;
        public int seasonId;
        public int sessionId;
        public int subSessionId;
        public int leagueId;
        public int raceWeek;

        // Entry-list / car-class information
        public sbyte numCars;               // total cars on track
        public sbyte numCarClasses;
        public ushort listCarClasses;       // bitfield (sim unit)
        public sbyte numCarTypes;
        public ushort listCarTypes;         // bitfield (sim unit)
        public sbyte minDrivers;
        public sbyte maxDrivers;

        // Weekend / rule options / settings
        public AidSettings options;

        // Weather / environmental data
        public EnvironmentData environment;   // size

        // Note: A C# 'char' is 2 bytes (UTF-16), equivalent to a C++ 'wchar_t'.
        // If the original C++ struct uses 'char' (1 byte), change these to 'fixed byte'.

        // Version / meta information
        public fixed byte versionStr[32];           // game version of sharedmem version or both

        // track string
        public fixed byte TrackName[64];          // trackname

        public fixed byte trackLayoutName[64];  // track config or layout name	

        public fixed byte trackCity[64];          // city

        public fixed byte trackCountry[64];          // country
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public unsafe struct WheelInfo // Must be 'unsafe' to contain fixed buffers
    {
        // Tyre Compound / Type 
        public sbyte tyreBrand;				// 0=Pirelli, 1=Michelin, etc.
        public sbyte tyreVisualCompound;        // (sim specific) example: none,prime, option, soft, med, hard
        public sbyte tyreCompound;          // Tyre compound enum (sim unit)
                                            // Example (FIA-style): 0=ultrasoft, 1=supersoft, 2=soft, 3=medium,
                                            // 4=hard, 5=intermediate, 6=wet
                                            // Meaning defined by the target sim/game

        // Pressures (kPa) 
        public float tyrePressure;              // Current live tyre pressure [kPa]
        public float tyrePressureCold;          // Cold setup pressure [kPa] (reference at pit/setup screen)
        public float tyrePressureLastHot;       // Last recorded hot pressure [kPa] (useful post-stint data)

        // Temperatures (?C) 
        public float tyreTempOverall;           // Average of inner/middle/outer surface temps [?C]
        public float tyreTempInner;         // Inner edge surface temperature [?C]
        public float tyreTempMiddle;            // Middle tread surface temperature [?C]
        public float tyreTempOuter;         // Outer edge surface temperature [?C]
        public float tyreTempCarcass;              // Carcass bulk ("core") temperature [?C]; deeper than surface temps
        public float tyreTempCore;              // Average of tyreTempInnerLayer temps [?C]
        public fixed float tyreTempInnerLayer[3];	// Left/center/right inner layer temps [?C]

        // Tyre Wear / Tread 
        public float tyreWearOverall;           // Normalised wear 0.0(new) ? 1.0(worn out)
        public float tyreWearInner;         // Segment wear ? inner [0..1]
        public float tyreWearMiddle;            // Segment wear ? middle [0..1]
        public float tyreWearOuter;         // Segment wear ? outer [0..1]
        public int tyreFlatSpot;            // Flatspot severity level (sim-defined metric)
        public float tyreDirtyLevel;            // Dirt/marble pickup level [0.0 clean ? 1.0 fully dirty]
        public float tyreOnSurfaceType;     // surface type under car tyre,tarmac, dirt, grass, gravel, sand, rumble,...

        // Wheel Dynamics 
        public float wheelSpeed;                // Linear ground speed of the wheel [m/s]
        public float wheelRotationSpeed;       // Wheel rotation rate [rad/s]
        public float wheelRadius;               // Effective rolling radius under load [m] (dynamic)

        // Contact patch velocities
        public float patchVelocityLat;         // Lateral velocity at contact patch [m/s]
        public float patchVelocityLong;        // Longitudinal velocity at contact patch [m/s]

        // Ground velocities
        public float groundVelocityLat;        // Lateral velocity at contact patch relative to ground [m/s]
        public float groundVelocityLong;       // Longitudinal velocity at contact patch relative to ground [m/s]
        public float lateralForce;             // lateral force [N]
        public float longitudinalForce;        // longitudinal force [N]
         
        // Slip metrics
        public float wheelSlipRatio;           // Longitudinal slip ratio (dimensionless)
        public float wheelSlipAngle;           // Lateral slip angle [rad]
        public float wheelCombinedSlip;        // Combined slip metric (model-dependent)
         
        // Grip and deflection
        public float tyreGrip;                 // Current grip coefficient (effective ?)
        public float verticalTyreDeflection;   // Tire deflection from unloaded radius [m]

        // Load and contact geometry
        public float tyreLoad;                 // Vertical load on tyre [N]
        public float contactPatch;             // Contact patch area [m?]
        public fixed float contactPoint[3];    // World position of contact [x,y,z]
        public fixed float contactNormal[3];   // Contact surface normal vector [x,y,z]
        public fixed float contactHeading[3];  // Forward direction of contact patch [x,y,z]

        // Alignment / Geometry 
        public float tyreRadius;               // Nominal static tyre radius [m] (construction/reference size)
        public float wheelCamberDeg;           // Camber angle at wheel [deg]
        public float wheelToe;                 // Toe angle [rad]
        public float wheelCornerWeight;        // Static sprung weight on this corner [N]

        // Surface / Usage / state
		public float wheelRumblePitch;		  // frequency in Hz of the tire vibrating over a rumble strip
        public int wheelSurfaceId;            // Track surface material ID (sim-specific mapping)
        public sbyte wheelOnRumbleStrip;      // on rumble strip
        public sbyte wheelInPuddleDepth;      // in puddle strip normalized depth 
        public sbyte SurfaceRumble;           // typical normalized 0.0 to 1.0 surface rumble (asphalt, kerb, gravel, grass, airborne, etc.)
        public sbyte tyreStintIndex;          // Stint index (increments when tyre set changes)
        public byte tyreFlat;                 // tyre flat 0/1
        public byte tyreDetached;             // tyre detached 0/1

        // settings
        public sbyte setupTyreWearRate;       // tyre wear rate (sim specific)
        public float chassisYaw;				// Yaw angle of the chassis relative to the direction of motion - radians

        public fixed byte _padding[2];
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public unsafe struct WheelData // Marked unsafe for clarity as it contains an unsafe struct
    {
        // The C# language does not allow "fixed" arrays of struct types (e.g., fixed WheelInfo[4]).
        // The correct pattern is to declare a single instance of the struct.
        // This field marks the memory location of the VERY FIRST wheel in the C++ array.
        // You will get a pointer to this field and use pointer arithmetic (e.g., pointer[i])
        // to access all 4 wheels in the buffer.
        public WheelInfo wheels;

        // This is the explicit padding to make the struct size correct
        public fixed byte _padding[648]; // (4 - 1) x sizeof(WheelInfo) 216 = 648
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct BrakeData // Must be 'unsafe' to contain a fixed buffer
    {
        // pressure
        public float brakePressure;     //  pressure [kPa or percent]

        // Temperatures
        public float brakeDiscTemp;         // current disc temp [?C]
        public float brakePadTemp;          // pad temp [?C]

        // Wear
        public float brakePadWear;          // normalized 0..1
        public float brakeDiscWear;         // normalized 0..1

        // Torque / force
        public float brakeTorque;           // torque applied at wheel [Nm]

        // Status / flags
        public sbyte brakeOverheated;       // 1 = overheated
     
        // Replaced the managed byte[] with a fixed buffer
        public unsafe fixed byte _padding[7];
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct SuspensionData // This struct is already blittable. No 'unsafe' or 'fixed' needed.
    {
        // Deflection / position
        public float suspDeflection;       // suspension travel [m]
		public float suspTravelNormalized; // suspension travel normalized [0.0 to 1.0]
		public float suspDeflectionMax;    // max deflection [m]
        public float suspDeflectionMin;    // min deflection [m]

        // Velocities
        public float suspVelocity;         // suspension velocity [m/s]
        public float damperVelocity;       // damper shaft velocity [m/s]

        // Forces
        public float suspForce;            // vertical suspension force [N]

        // Ride height
        public float rideHeight;           // ride height at this wheel [m]

        // Alignment
        public float suspCamberDeg;        // camber [?]
        public float suspToeDeg;           // toe [?]
        public float suspCasterDeg;        // caster [?]
		
		public float _padding;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public unsafe struct CarDamageData // Must be 'unsafe' to contain a fixed buffer
    {
        // Replaced the managed sbyte[] with a fixed buffer
        public fixed sbyte damageZone[8];          // example: front 0, rear 1, left 2, right 3, centre 4

        // Engine & drivetrain damage
        public sbyte damageEngine;          // engine state 0=ok, >0 damaged
        public sbyte damageEngineBlown;     // 1 if blown
        public sbyte damageEngineSeized;    // 1 if seized
        public float damageEngineWear;      // wear 0..1
        public float damageEngineOverrev;   // accumulated over-rev wear
       
        public sbyte damageTransmission;    // gearbox state
        public float damageGearboxWear;     // gearbox wear 0..1
        public sbyte damageClutch;          // clutch damage state
        public float damageClutchWear;      // wear 0..1
        public sbyte damageDifferential;    // differential state

        // Chassis / aero
        public float damageChassis;         // global chassis integrity 0..1
        public float damageAero;            // aero integrity 0..1
        public float damageFrontWing;       // front aero element damage %
        public float damageRearWing;        // rear aero element damage %
        public float damageFloor;           // floor damage %

        // Suspension / steering damage
        public float damageSuspensionLF;    // LF suspension integrity %
        public float damageSuspensionRF;    // RF suspension integrity %
        public float damageSuspensionLR;    // LR suspension integrity %
        public float damageSuspensionRR;    // RR suspension integrity %
        public float damageSteering;        // steering column/rack damage %

        // Body / cosmetic
        public sbyte damageBodywork;        // bodywork state
        public float damageBodyIntegrity;   // normalized 0..1
        public float damageScratch;         // scratch level %
        public float damageDent;            // dent level %

        public fixed sbyte damageBrakes[4]; // brake damage
        public fixed sbyte damageTyres[4];  // tyre damage
        public sbyte damageExhaust;		    // 0=ok, 1=damaged, 2=broken
        public fixed byte _padding[4];
    }

	[Flags]
	public enum CarSignal : uint
	{
		None = 0,

		// -------------------------------------------------
		// Exterior Lighting & Visibility
		// -------------------------------------------------
		LeftTurn        = 1u << 0,  // Left turn indicator active
		RightTurn       = 1u << 1,  // Right turn indicator active
		Hazard          = 1u << 2,  // Hazard warning flashers active
		Horn            = 1u << 3,  // Horn / audible warning active
		LowBeam         = 1u << 4,  // Dipped beam headlights active
		HighBeam        = 1u << 5,  // Main beam (brights) toggled on
		FogFront        = 1u << 6,  // Front fog lamps enabled
		FogRear         = 1u << 7,  // Rear high-intensity fog lamp enabled
		ParkingLights   = 1u << 8,  // Position / side lights active
		Drl             = 1u << 9,  // Daytime Running Lights active
		BrakeLight      = 1u << 10, // Main brake lamps currently illuminated
		ReverseLight    = 1u << 11, // Reverse gear indicator lamp active
		WipersActive    = 1u << 12, // Front windshield wipers operating
		RearWiper       = 1u << 13, // Rear window wiper operating
		Beacon          = 1u << 14, // Roof-mounted warning beacon active

		// -------------------------------------------------
		// Driving & Cruise Assists
		// -------------------------------------------------
		CruiseControlActive = 1u << 15, // Speed cruise control system engaged
		SpeedLimitWarning   = 1u << 16, // Vehicle has exceeded set dash speed limit
		AutopilotActive     = 1u << 17, // Semi-autonomous steering/driving active
		LaneAssistActive    = 1u << 18, // Lane departure/keeping system active

		// -------------------------------------------------
		// EV / Hybrid Specific (Lamp Status)
		// -------------------------------------------------
		ChargingConnected = 1u << 19, // EV charging cable is physically plugged in
		ChargingActive    = 1u << 20, // Battery is currently drawing power from grid
		ChargeComplete    = 1u << 21, // Charging cycle is finished
		ChargeFault       = 1u << 22, // Malfunction detected in charging hardware
		PowerLimitActive  = 1u << 23, // Performance capped (low SoC or battery heat)

		// -------------------------------------------------
		// Heavy Vehicle / Truck Drivetrain
		// -------------------------------------------------
		EngineBrakeActive = 1u << 24, // Engine/Exhaust brake system currently braking
		RetarderActive    = 1u << 25, // Driveline retarder (hydraulic/electric) active
		DiffLockActive    = 1u << 26, // Differential lock engaged for traction
		LiftAxleActive    = 1u << 27, // Tag/Mid-lift axle currently in raised position
		AdblueLow         = 1u << 28, // SCR fluid (AdBlue) is low indicator

		// -------------------------------------------------
		// Mechanical / Safety (Core Indicators)
		// -------------------------------------------------
		ParkingBrake   = 1u << 29, // Mechanical/Electronic parking brake applied
		IgnitionOn     = 1u << 30, // Vehicle ignition/electrics powered on
		HighBeamFlash  = 1u << 31  // Flash-to-pass (momentary high beam) active
	}

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public unsafe struct WarningFlag // Must be 'unsafe' to contain a fixed buffer
    {
        //  Race control flags
        public sbyte greenFlagWarning;      // Green flag shown
        public sbyte redFlagWarning;        // Red flag shown
        public sbyte yellowFlagWarning;     // Yellow flag shown
        public sbyte fullYellowFlagWarning; // Full Yellow flag shown (Safety Car/FYC)
        public sbyte redlineWarning;        // Engine at/over redline
        public sbyte blueFlagWarning;       // Blue flag (faster car approaching / lapping)
        public sbyte blackFlagWarning;      // Black flag (disqualified / pit immediately)
        public sbyte whiteFlagWarning;      // White flag (slow vehicle on track / final lap)
        public sbyte checkeredFlagWarning;  // Checkered flag (end of race/session)
        public sbyte meatballFlagWarning;   // Black with orange circle (mechanical issue, pit required)
        public sbyte blackWhiteFlagWarning; // Black/white diagonal (unsportsmanlike conduct warning)
        public sbyte surfaceFlagWarning;        // Yellow/red striped (slippery surface, debris, fluids)
        public sbyte orangeFlagWarning;     // orange flag / debris flag

        public short flagsWarning;          // Other flags (bitfield, extended flags)													   

        // Safety car / race management
        public sbyte isSafetyCar;       // safety car deployed
        public sbyte isSafetyCarPassing;// permission to pass safety car
        public sbyte isVSC;             // virtual safety car active

        // Systems / safety aids / signals
        public sbyte padWearWarning;            // Brake pad wear threshold exceeded

        public sbyte turboStatus;           // Turbo status/flag (sim specific)
        public sbyte headlights;                // 1 = headlights on
        public sbyte startLights;           // Start light state (0 = off, 1..n = lights on)
        public sbyte espSignal;             // Electronic stability program (ESP) active
		public CarSignal carSignals;			// check CarSignal enum

        // pit
        public sbyte pitLimiterActive;    // pit speed limiter active
        public sbyte pitRequest;            // driver requested pit stop flag

		public sbyte tcKillSwitch;			// master traction control kill switch on/off
        public sbyte numCutTrackWarning;
        public sbyte shiftIndicator;		// OSP normalized 0.0 to 1.0,  0.98-1.0 == optimal shiftpoint

        // General warnings
        public sbyte engineWarning;         // Engine warning (overheat, malfunction, etc.)
        public sbyte engineStallWarning;
        public sbyte waterWarning;          // Coolant/water temperature warning
        public sbyte oilWarning;            // Oil pressure/temperature warning
        public sbyte overHeatWarning;	    // other than engine
        public sbyte fuelWarning;           // Low fuel warning
        public sbyte voltageWarning;        // battery Low/high voltage warning
        public sbyte damageWarning;         // General damage warning
        public sbyte detachedParts;         // Car has detached parts (wings, wheels, etc.)
        public sbyte brakeWarning;          // Brake system warning
        public sbyte clutchWarning;         // clutch warning
        public sbyte gearboxWarning;        // gearbox failure warning
        public sbyte tyreWarning;           // tyre warning
        public sbyte transmissionWarning;   // transmission warning
        public sbyte penaltyWarning;        // official warning flag

        public sbyte revLightsPercent;		// rev lights in percent

        public byte  activeAeroMode;                   // 0 = Corner mode, 1 = Straight mode
        public byte activeAeroAvailable;               // 0 = not available, 1 = available
        public short activeAeroActivationDistance;     // 0 = Active aero not available, non-zero - Active aero will be available in [X] metres
        public byte overtakeAvailable;                // 0 = not available, 1 = available
        public byte overtakeActive;                   // 0 = not active, 1 = active
        public short overtakeActivationDistance;       // 0 = Overtake Mode not available, non-zero - Overtake Mode will be available in [X] metres
        public byte m2026Regulations;                  // 0 = vehicle conforms to pre-2026, 1 = 2026 regulations applicable
        public byte drivingWrongWay;                  // Whether the car is driving the wrong way

        // Replaced the managed byte[] with a fixed buffer
        public fixed byte _padding[6];
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public unsafe struct EnergySystemData // Must be 'unsafe' to contain a fixed buffer
    {
        // Generic ERS/KERS state
        public float ersStoreEnergy;                // J - current stored energy in ES (battery/supercap)
        public float ersStoreEnergyMax;         // J - maximum storable energy
        public float ersStoreEnergyPercent;     // % - convenience value (0?100), derived if not provided

        // Deployment
        public float ersDeployPower;                // W - current deploy power output (instantaneous)
        public float ersDeployedThisLap;            // J - total deployed energy this lap
        public float ersDeployLimitPerLap;          // J - max deployable energy per lap (if rule-limited)
        public sbyte ersDeployMode;             // mode selector (0=auto, 1=hotlap, 2=qualy, etc.)
        public sbyte ersAllowDeploy;                   // flag: deployment available/allowed right now

        // Harvesting
        public float ersHarvestedThisLapMGUK;       // J - harvested via MGU-K this lap
        public float ersHarvestedThisLapMGUH;       // J - harvested via MGU-H this lap
        public float ersHarvestedThisLapOther;      // J - harvested via regen braking, etc.
        public float ersHarvestPowerMGUK;           // W - current harvest power from MGU-K
        public float ersHarvestPowerMGUH;           // W - current harvest power from MGU-H

        public float ersTorqueMGUK;

        // KERS (legacy systems, pre-ERS era)
        public float kersStoreEnergy;               // J - current KERS energy store
        public float kersStoreEnergyMax;            // J - max KERS capacity (e.g., 400 kJ in F1 2009?2013)
        public float kersStorePercent;              // % - derived (0?100)
        public float kersDeployPower;               // W - current deploy power
        public float kersDeployedThisLap;           // J - deployed via KERS this lap
        public sbyte kersActvated;              // is driver pressing KERS button
        public sbyte kersAvailable;             // true if usable at this moment (not cooling/locked out)

        // Thermal & Limits
        public float ersBatteryTemp;            // ?C - battery temperature
        public float ersMGUKTemp;               // ?C
        public float ersMGUHTemp;               // ?C
        public float ersEfficiency;             // ratio (0?1), system efficiency if provided

        // Flags
        public sbyte hasERS;                    // car has ERS (0/1)
        public sbyte ersFault;                  // fault/error present
        public sbyte ersOverheated;             // limited due to temperature
        public sbyte ersHarvestingActive;       // currently harvesting
        public sbyte ersDeployingActive;        // currently deploying
        public sbyte hasKERS;                   // car has KERS (0/1)
        public sbyte ersRecoveryMode;			// (0?13 usually) indicating the setting of the MGU-K regen (how much drag the motor gives under braking)

        // Replaced the managed byte[] with a fixed buffer
        public fixed byte _padding[1];
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public unsafe struct CarSetupState // Must be 'unsafe' to contain a fixed buffer
    {
        // Differential (normalized or raw index)
        public float diffEntry;        // 0..1 or setup index
        public float diffMiddle;       // 0..1 or setup index
        public float diffExit;         // 0..1 or setup index
         
         // Engine / ECU
        public float engineMap;        // engine power / ECU map index
        public float engineBraking;    // engine braking map index
        public float throttleShape;    // throttle map / pedal curve (dcThrottleShape)
         
         // Anti-roll bars
        public float frontAntiRollBar; // setup index or normalized
        public float rearAntiRollBar;
         
         // Weight jacker
        public float weightJackerLeft;  // turns or mm
        public float weightJackerRight;
         
         // Aero
        public float frontFlap;        // flap angle / step
        public float rearFlap;
    }

        //=====================================================================
        // VEHICLE DATA, telemetry of the car you are currently driving
        //=====================================================================
        [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public unsafe struct VehicleData // Must be 'unsafe' to contain fixed buffers and other unsafe structs
    {
        public float speed;                    // car speed [m/s]

        // Transmission / gearbox
        public sbyte gear;                   // -1 = reverse, 0 = neutral, >0 forward
        public byte maxGear;                // number of forward gears
        public byte gearIndexNeutral;       // 1 if neutral index flagged
        public byte gearIndexReverse;       // 1 if reverse index flagged
        public float transmissionSpeed;      // transmission output speed / ratio

        // Pedals / driver inputs
        public float throttle;                 // normalized [0..1]
        public float throttleRaw;              // raw input
        public float throttleTravel;           // pedal travel
        public float brake;                    // normalized brake input [0..1]
        public float brakeRaw;                 // raw brake
        public float handbrake;                // normalized handbrake
        public float handbrakeRaw;             // raw handbrake input
        public float clutch;                   // normalized clutch [0..1]
        public float clutchRaw;                // raw clutch
        public float clutchTravel;             // clutch travel (if available)

        //  Engine / powertrain
        public float rpm;                    // engine RPM
        public float engineTorque;           // Nm (if provided)
        public float idleRpm;                // idle RPM
        public float maxRpm;                 // redline
        public float engineTemp;             // temperature [C]
        public byte ignitionOn;             // 0/1
        public byte driverTrainType;        // (sim unit) example: 0 = FWD, 1 = RWD, 2 = AWD
        public byte NumCylinders;           // number of cylinders in the engine
        public byte antiStallActivated;		// anti-stall activated 0/1

        // Temperatures & pressures
        public float oilTemp;                  // Oil temperature [C]
        public float oilPress;                 // Oil pressure (kPa)
        public float waterTemp;                // Water / coolant temp [C]
        public float waterLevel;               // Water level [L or unit provided]
        public float exhaustTemp;              // Exhaust temperature [C]
        public float fuelPressure;             // Fuel pressure [kPa]
        public float voltage;                  // Electrical system voltage [V]
        public float batterySOC;               // battery state-of-charge (0..1)
    
        // Fuel system
        public float fuelLevel;                // liters
        public float fuelLevelPct;             // 0..1
        public float fuelCapacity;             // liters max
        public float fuelUsePerHour;           // L/h
        public float fuelPerLap;               // estimated L per lap
        public float fuelEstimatedLaps;        // remaining laps estimate
        public sbyte setupFuelRate;          // consumption rate (sim specific)
        public sbyte fuelMix;					// Fuel mixture modes (lean, normal, rich, etc.)

        // Turbo / boost / forced-induction
        public float turboLevel;               // current turbo pressure (bar or sim unit)
        public float maxTurbo;                 // max turbo capability (sim-specific)
        public float boost;                    // current effective boost (generic)
        public float maxBoost;                 // absolute max effective boost (generic cross-tech ceiling)
        public float maxTorque;                // highest torque peak anywhere on the torque curve
        public float maxPower;                 // the highest power peak

        //  ABS / Traction Control / stability aids
        public sbyte antiLockBrakes;       // ABS setup value
        public sbyte absInAction;          // ABS real time currently active (0/1)

        public sbyte tractionControl;       // TC setup value
        public sbyte tractionControl2;      // TC aux setup value
        public sbyte tcInAction;			// TC real time currently acting (0/1) or TCCUT counter/value Real-time TC value [0/100%]


        // Brake bias
        public float brakeBias;             // overall brake bias (front/rear or percent)
        public float frontBrakeBias;            // explicit front bias value if available
        public float rearBrakeBias;         // explicit rear bias value if available

        // DRS
        public sbyte hasDRS;                    // car supports DRS (0/1)
        public sbyte drsAllowed;                // allowed to use DRS (0/1)
        public sbyte drsEnabled;                // DRS system enabled flag
        public sbyte drsEngaged;                // currently engaged
        public sbyte drsState;              // drs state (enum)
        public sbyte drsNumActivationsLeft; // activations left
        public sbyte drsNumActivationsTotal;    // total activations
        public short drsActivationDistance; // activation distance (m)

        // PUSH-TO-PASS
        public sbyte hasPTP;                    // car supports Push-To-Pass
        public sbyte ptpEngaged;                // currently engaged
        public sbyte ptpActivationLeft;     // activations left
        public sbyte ptpActivationTotal;        // ptp total activations 

        // spare fixed buffer
        public fixed byte _padding[3];

        //  Global aerodynamics / chassis geometry
        public float currentDownforce;              // total downforce (sim value)
        public float frontWingHeight;               // aero setup scalar
        public float frontRideHeight;               // global front ride height (m)
        public float rearRideHeight;                // global rear ride height (m)
        public float frontRollAngle;                // Roll angle of the front suspension
        public float rearRollAngle;             // Roll angle of the rear suspension
        public float frontThirdSpringDeflection;    // [m] deflection at front 3rd spring
        public float frontThirdSpringVelocity;      // [m/s] velocity at front 3rd spring (if available)
        public float rearThirdSpringDeflection; // [m] deflection at rear 3rd spring
        public float rearThirdSpringVelocity;       // [m/s] velocity at front 3rd spring (if available)
        public float cgHeight;                      // center of gravity height (m)
        public float heightOfCOGAboveGround;        // alternate naming, if present
        public float carLength;                 // [m]
        public float carWidth;                      // [m]
        public float totalMass;                 // [kg] Car + penalty weight + fuel
                                                // location of car's center of gravity in world space (X, Y, Z)

        //  Steering / wheel
        public float steer;                    // steering input [-1..1]
        public float steerLock;                // current lock angle full left to right (radians or degrees per sim)
        public float steerAngle;               // current wheel angle (radians or degrees per sim)
        public float steerPeakForceNm;         // peak force reading (Nm)
        public float steeringTorque;			// torque (Nm)
        public byte steeringPctTorque;        // torque (percent)

        //  Mileage / counters
        public float odometerKm;               // total km (if available)
        public float totalDistance;            // total Distance (sim reports public float)

        // motion platform vibration helpers
        public float kerbVibration; // vibrations sent to the FFB, could be used for motion rigs
        public float slipVibrations; // vibrations sent to the FFB, could be used for motion rigs
        public float gVibrations; // vibrations sent to the FFB, could be used for motion rigs
        public float absVibrations; // vibrations sent to the FFB, could be used for motion rigs

        // Struct arrays converted to the "First Element Placeholder" pattern
        // Sub-systems (arrays)  keep exactly as in the original file
        public BrakeData brakes; // x4
        public fixed byte _brakes_padding[96]; // 3x 32
        public SuspensionData suspensions; // x4
        public fixed byte _susp_padding[144]; // 3x 48

        // These are OK because they are single instances of already-corrected structs
        public CarDamageData carDamage;
        public WarningFlag warningFlag;
        public EnergySystemData energySystem;   // embed energy system (ERS/KERS/MGU-K/MGU-H) size
        public CarSetupState carSetupState;
    }

    // =====================================================================
    // PLAYER DATA per-driver / per-car static & dynamic info
    // =====================================================================
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public unsafe struct PlayerInfo // Must be 'unsafe' to contain fixed buffers
    {
        // Identification
        public byte isLocalPlayer;              // 1 if record is for local player

        public int driverId;                    // Unique driver ID
        public int teamId;                      // Team ID
        public int carLiveryId;
        public int carNumber;                   // Raw numeric car number
        public int carClassId;                  // Class identifier
        public int carModelId;                  // Model identifier
        public int manufacturerId;              // manufacturer
        public int carClassPerformanceIndex;    // (sim specific range)

        // Status flags (binary / boolean)
        public sbyte driverStatus;          // (sim specific) example: 0 = in garage, 1 = flying lap, 2 = in lap, 3 = out lap, 4 = on track, spectactor, controlled by AI
        public sbyte isLastLap;
        public sbyte isValidLap;
        public sbyte lapValidState;
        public sbyte isFinished;
        public sbyte engineType;                // (sim specific) combustion, eletric, hybrid, ...
        public sbyte engineState;               // (sim specific) example: -1 unavailable, 0 = ignition off, 1 = ignition on but not running, 2 = ignition on and running
        public sbyte drsState;               // drs state
        public sbyte ptpState;               // push-to-pass state
        public sbyte inGarage;               // in garage
        public sbyte inPitlane;              // If current vehicle is in pitlane (-1 = N/A)
        public sbyte inPitBox;               // 1 = in own pit box, 0 = otherwise
        public sbyte pitState;               // Enum: none, entering, in-lane, in-stall, exiting, etc.
        public sbyte numPitstops;            // Number of pitstops the current vehicle has performed (-1 = N/A)
        public sbyte pitOptionalRepairLeft;
        public sbyte pitRepairLeft;

        public float pitElapsedTime;         // Current vehicle pitstop actions duration
        public float pitTotalDuration;

        // Race-progress fields
        public sbyte driverResult;      // (sim specific) example: 0 = invalid, 1 = inactive, 2 = active, 3 = finished, 4 = didnotfinish, 5 = disqualified, 6 = not classified, 7 = retired
        public sbyte gridPosition;      // Grid position the car started the race in
        public sbyte position;            // Current race position
        public sbyte classPosition;       // Current class position
        public sbyte positionFinal;       // race position result
        public sbyte positionClassFinal;  // race class position result
        public sbyte sector;              // current sector
		public sbyte deltaTimeFromGame;   // game provide a delta time
        public int lapsCompleted;       // Completed laps
        public int currentLap;          // Current lap number
        public float lapDist;           // Distance traveled in current lap [m]
        public float lapDistPct;        // Distance traveled in current lap [percent]

        // position in world space
        public float worldPositionX;
        public float worldPositionY;
        public float worldPositionZ;

        // car basic
        public float carSpeed;                  // car speed [m/s]

        // Timing  best / last / current laps and deltas
        public int bestLapNum;                  // Lap number of best lap
        public float bestLapTime;               // Best lap time
        public float lastLapTime;               // Last completed lap time
        public float currentLapTime;            // Current ongoing lap time
        public float estimatedLapTime;          // Estimated time to complete the current lap 
        public float estimatedRaceFinishTime;   // Estimated time to finish the race

        public float deltaTimeVersusPBNative;       // Seconds. native delta between current time and best lap time (PB) from the game
        public float deltaTimeVersusPB;             // Seconds. delta between current time and personal best (from OpenSR not the game)
         
        public float deltaTimeLastLap;              // Seconds. Delta Time to session last lap
        public float deltaTimeSessionBestLap;       // Seconds. Delta Time to session best
        public float deltaTimeOptimalLap;           // Seconds. Delta Time to optimal lap
        public float deltaTimeSessionOptimalLap;    // Seconds. Delta Time to session optimal lap
         
        public float deltaTimeVersusClassLeader;    // Seconds. delta Time between class leader time or last lap time of current class leader
        public float deltaTimeVersusLeader;         // Seconds. delta Time between leader time or last lap time of current leader
        public float deltaTimeVersusFront;          // Seconds. delta Time between current time and front driver time
        public float deltaTimeVersusBehind;		    // Seconds. delta Time between current time and driver behind time

		public int lapsBehindFront;				// laps behind vehicle in front (next higher place)
		public int lapsBehindLeader;			// laps behind leader
		
        // Sector times (current / last / best)
        public fixed float curSectorTime[16];   // current sectors
        public fixed float lastSectorTime[16];  // last sectors
        public fixed float bestSectorTime[16];  // best sectors

        // Penalties / infractions
        public sbyte numPenalties;          // Number of penalties applied to this driver
        public sbyte numTeamPenalties;      // Team incident count
        public float penaltiesDuration;     // total penalties accumulated in sec
        public int penaltyType;             // penalties (sim specific bitfield)
                                            //      drive-through penalty,
                                            //      stop & go penalty,
                                            //      time penalty,
                                            //      disqualified,
                                            //      penalty cleared flag, etc.
        public int penaltyCauseId;          // (sim specific) ID of Infringement type
        public sbyte cutDriveThroughCount;   // accumulated cut-track penalties
        public sbyte cutStopAndGoCount;      // accumulated cut-track penalties
        public sbyte cutPitStopCount;        // accumulated cut-track penalties
        public sbyte cutTimeDeductionCount;  // accumulated cut-track penalties
        public sbyte cutSlowDownCount;       // accumulated cut-track penalties
		
		public sbyte tyreVisualCompound;	// (sim specific) example: none,prime, option, soft, med, hard
		public sbyte tyreCompound;			// Tyre compound enum (sim unit)
											// Example (FIA-style): 0=ultrasoft, 1=supersoft, 2=soft, 3=medium,
											// 4=hard, 5=intermediate, 6=wet
											// Meaning defined by the target sim/game
       	public sbyte rcJockerCount;			// rally cross number of jocker already done

		public fixed byte _padding[1];

        // (string fields replaced with fixed buffers)
        // Note: A C# 'char' is 2 bytes (UTF-16), equivalent to a C++ 'wchar_t'.
        // If the original C++ struct uses 'char' (1 byte), change these to 'fixed byte'.
        // Human-readable strings
        public fixed byte driverName[64];             // player name 

        public fixed byte driverNickname[64];             // player nickname

        public fixed byte teamName[64];             // player teamName name 

        public fixed byte CarName[64];                // car name

        public fixed byte ClassName[32];              // car class name

    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public unsafe struct PlayerRecords // Marked unsafe for clarity and consistency
    {
        public int reserved;
        public int carsCount;

        // The C# language does not allow "fixed" arrays of struct types (e.g., fixed PlayerInfo[96]).
        // The correct pattern is to declare a single instance of the struct.
        // This field marks the memory location of the VERY FIRST player in the C++ array.
        // You will get a pointer to this field and use pointer arithmetic (e.g., pointer[i])
        // to access all 96 players in the buffer.
        public PlayerInfo player;
        // This is the explicit padding to make the struct size correct
        public fixed byte _padding[63840]; // 95 x 672 sizeof(PlayerInfo) = 63840

    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public unsafe struct ExtensionData // Marked unsafe for clarity and consistency
    {
        public fixed float extension[256];

    }

    //--------------------------------------
    // PacketHeader (32 bytes aligned)
    //--------------------------------------
    [StructLayout(LayoutKind.Sequential, Pack = 1, Size = 32)]
    public struct PacketHeader
    {
        public byte reportAvailable;
        public byte paused;
        public ulong elapsedTime;       // Seconds
        public ulong frameId;           //  Incrementing frame ID reference, incremented on send packet to network
        public int frameRateFromGame;	// frame rate from game telemetry
        public byte majorVersion;
        public byte minorVersion;
        public byte packetType;
        public byte packetIndex;
        public byte playerSlotIndex;

        public unsafe fixed byte spare[5];
    }

    public struct DummyData { public int testValue; public int testValue2; }
    //--------------------------------------
    // main struct
    //--------------------------------------
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct OutSimData
    {
        public PacketHeader packetHeader;
        public PlayerRecords players;
        public MotionData motionData;
        public SessionData sessionData;
        public VehicleData vehicleData;
        public WheelData wheelData;
        public ExtensionData extensionData;
       // public DummyData dummy;                 // fix a c# bug, prevent structure to be troncated before the end
    }

}


/*
PacketHeader: 32
-------------
MotionData: 448
SessionData: 528
PlayerInfo: 672
PlayerRecords: 64520
VehicleData: 920
WheelData: 864
ExtensionData: 1024
-------- SUB --------
WheelInfo: 216
EnergySystemData: 96
CarDamageData: 96
BrakeData: 32
SuspensionData: 48
warningFlag: 64
EnvironmentData: 88
Total OutSimData: 68336 
BlobData 32808
*/



