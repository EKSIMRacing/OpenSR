#####################################################################
# Part of Open Sim Relay (OpenSR) - Source Code
# Copyright (c) 2025-2026 Zappadoc - All Rights Reserved
#
# This file is part of the OpenSR SDK.
#
# PURPOSE:
#   Provides shared data structures and interface definitions
#   required to develop third-party OpenSR plugins.
#
# LICENSE:
#   This script is provided under the OpenSR Plugin Interface
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
#   2026-02-17 : Initial creation by Zappadoc
#   2026-03-01 : Integrated into OpenSR project
#####################################################################

import os
import sys

import ctypes
import ctypes.wintypes as wintypes
import threading
import time
import re
import shutil
import msvcrt        # non-blocking key reads (Windows)
import math

k32 = ctypes.WinDLL("kernel32", use_last_error=True)

FILE_MAP_READ = 0x0004
ERROR_FILE_NOT_FOUND = 2

PROCESS_QUERY_LIMITED_INFORMATION = 0x1000
SYNCHRONIZE = 0x00100000
WAIT_TIMEOUT = 0x102

ntdll = ctypes.WinDLL("ntdll")
ntdll.NtQuerySystemInformation.restype = ctypes.c_long
ntdll.NtQuerySystemInformation.argtypes = [wintypes.ULONG, ctypes.c_void_p,
                                           wintypes.ULONG, ctypes.POINTER(wintypes.ULONG)]

SystemProcessInformation = 5
STATUS_INFO_LENGTH_MISMATCH = ctypes.c_long(0xC0000004).value

class UNICODE_STRING(ctypes.Structure):
    _fields_ = [("Length", wintypes.USHORT),
                ("MaximumLength", wintypes.USHORT),
                ("Buffer", ctypes.c_void_p)]

class SYSTEM_PROCESS_INFORMATION(ctypes.Structure):
    _fields_ = [("NextEntryOffset", wintypes.ULONG),
                ("NumberOfThreads", wintypes.ULONG),
                ("WorkingSetPrivateSize", ctypes.c_longlong),
                ("HardFaultCount", wintypes.ULONG),
                ("NumberOfThreadsHighWatermark", wintypes.ULONG),
                ("CycleTime", ctypes.c_ulonglong),
                ("CreateTime", ctypes.c_longlong),
                ("UserTime", ctypes.c_longlong),
                ("KernelTime", ctypes.c_longlong),
                ("ImageName", UNICODE_STRING),
                ("BasePriority", ctypes.c_long),
                ("UniqueProcessId", ctypes.c_void_p)]

def find_pid(exe_name):
    target = exe_name.lower()
    size = 1 << 20
    while True:
        buf = ctypes.create_string_buffer(size)
        ret = wintypes.ULONG()
        status = ntdll.NtQuerySystemInformation(SystemProcessInformation, buf,
                                                size, ctypes.byref(ret))
        if status == STATUS_INFO_LENGTH_MISMATCH:
            size = ret.value + 65536       # process list grew, retry bigger
            continue
        if status < 0:
            return None
        break

    base, off = ctypes.addressof(buf), 0
    while True:
        spi = SYSTEM_PROCESS_INFORMATION.from_address(base + off)
        name = spi.ImageName
        if (name.Buffer and name.Length
                and ctypes.wstring_at(name.Buffer, name.Length // 2).lower() == target):
            return spi.UniqueProcessId or None
        if not spi.NextEntryOffset:
            return None
        off += spi.NextEntryOffset

# ---------------- shared memory

class MEMORY_BASIC_INFORMATION(ctypes.Structure):
    _fields_ = [
        ("BaseAddress", ctypes.c_void_p),
        ("AllocationBase", ctypes.c_void_p),
        ("AllocationProtect", wintypes.DWORD),
        ("PartitionId", wintypes.WORD),
        ("RegionSize", ctypes.c_size_t),
        ("State", wintypes.DWORD),
        ("Protect", wintypes.DWORD),
        ("Type", wintypes.DWORD),
    ]

k32.OpenFileMappingW.restype = wintypes.HANDLE
k32.OpenFileMappingW.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.LPCWSTR]
k32.MapViewOfFile.restype = ctypes.c_void_p
k32.MapViewOfFile.argtypes = [wintypes.HANDLE, wintypes.DWORD, wintypes.DWORD,
                              wintypes.DWORD, ctypes.c_size_t]
k32.UnmapViewOfFile.restype = wintypes.BOOL
k32.UnmapViewOfFile.argtypes = [ctypes.c_void_p]
k32.CloseHandle.restype = wintypes.BOOL
k32.CloseHandle.argtypes = [wintypes.HANDLE]
k32.VirtualQuery.restype = ctypes.c_size_t
k32.VirtualQuery.argtypes = [ctypes.c_void_p,
                             ctypes.POINTER(MEMORY_BASIC_INFORMATION),
                             ctypes.c_size_t]

TARGET_HOST_PROCESS = "OpenSR.exe"
SHARED_MEM_OUTSIMDATA = "Local\\SMOSROUTSIMDATA"
SHARED_MEM_CONTEXT = "Local\SMOSRCONTEXTDATA"
TYRE_INDEX_MAX = 4
MAX_PLAYERS = 96

# ===========================================================================
# ctypes mirrors (pack = 1). Rules:
#   * every struct gets _pack_ = 1 (NOT inherited by nested structs)
#   * C array  T x[N]       ->  ("x", T * N)
#   * nested struct member  ->  ("m", OtherStructClass)
#   * bool/uint8_t          ->  c_uint8, char[N] -> c_char * N
# ===========================================================================
class PacketHeader(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("reportAvailable",   ctypes.c_uint8),
        ("paused",            ctypes.c_uint8),
        ("elapsedTime",       ctypes.c_uint64),
        ("frameId",           ctypes.c_uint64),
        ("frameRateFromGame", ctypes.c_uint32),
        ("majorVersion",      ctypes.c_uint8),
        ("minorVersion",      ctypes.c_uint8),
        ("packetType",        ctypes.c_uint8),
        ("packetIndex",       ctypes.c_uint8),
        ("playerSlotIndex",   ctypes.c_uint8),
        ("_padding",          ctypes.c_uint8 * 5),
    ]

class MotionData(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("localAccelX", ctypes.c_double), ("localAccelY", ctypes.c_double), ("localAccelZ", ctypes.c_double),
        ("localVelocityX", ctypes.c_double), ("localVelocityY", ctypes.c_double), ("localVelocityZ", ctypes.c_double),
        ("angularVelocityX", ctypes.c_double), ("angularVelocityY", ctypes.c_double), ("angularVelocityZ", ctypes.c_double),
        ("angularAccelX", ctypes.c_double), ("angularAccelY", ctypes.c_double), ("angularAccelZ", ctypes.c_double),
        ("pitch", ctypes.c_double), ("yaw", ctypes.c_double), ("roll", ctypes.c_double),
        ("pitchRate", ctypes.c_double), ("yawRate", ctypes.c_double), ("rollRate", ctypes.c_double),
        ("gForceLat", ctypes.c_double), ("gForceVert", ctypes.c_double), ("gForceLong", ctypes.c_double),
        ("velocityX", ctypes.c_double), ("velocityY", ctypes.c_double), ("velocityZ", ctypes.c_double),
        ("carCGLocX", ctypes.c_double), ("carCGLocY", ctypes.c_double), ("carCGLocZ", ctypes.c_double),
        ("rotMat", ctypes.c_double * 9),
        ("upDirY", ctypes.c_double), ("upDirX", ctypes.c_double), ("upDirZ", ctypes.c_double),
        ("rightDirX", ctypes.c_double), ("rightDirY", ctypes.c_double), ("rightDirZ", ctypes.c_double),
        ("frwdDirX", ctypes.c_double), ("frwdDirY", ctypes.c_double), ("frwdDirZ", ctypes.c_double),
        ("worldVelocityX", ctypes.c_double), ("worldVelocityY", ctypes.c_double), ("worldVelocityZ", ctypes.c_double),
        ("suspensionDeflectionLF", ctypes.c_double), ("suspensionDeflectionRF", ctypes.c_double),
        ("suspensionDeflectionLR", ctypes.c_double), ("suspensionDeflectionRR", ctypes.c_double),
        ("suspensionVelocityLF", ctypes.c_double), ("suspensionVelocityRF", ctypes.c_double),
        ("suspensionVelocityLR", ctypes.c_double), ("suspensionVelocityRR", ctypes.c_double),
    ]

class EnvironmentData(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("timeOfDay", ctypes.c_float), ("timeScale", ctypes.c_float),
        ("sunAltitude", ctypes.c_float), ("sunAzimuth", ctypes.c_float),
        ("cloudLevel", ctypes.c_float), ("fogLevel", ctypes.c_float),
        ("airTemp", ctypes.c_float), ("airPressure", ctypes.c_float),
        ("humidity", ctypes.c_float), ("airDensity", ctypes.c_float),
        ("windSpeed", ctypes.c_float), ("windDirection", ctypes.c_float),
        ("rainLevel", ctypes.c_float), ("trackWetness", ctypes.c_float),
        ("puddleLevel", ctypes.c_float), ("snowLevel", ctypes.c_float),
        ("trackTemp", ctypes.c_float), ("trackGrip", ctypes.c_float),
        ("trackDust", ctypes.c_float), ("trackMarbles", ctypes.c_float), ("trackOil", ctypes.c_float),
        ("weatherFlag", ctypes.c_uint8), ("_padding", ctypes.c_uint8 * 3),
    ]

class AidSettings(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("assistAbs", ctypes.c_int8), ("assistTc", ctypes.c_int8), ("assistEsp", ctypes.c_int8),
        ("assistCountersteer", ctypes.c_int8), ("assistCornering", ctypes.c_int8),
        ("assistStability", ctypes.c_int8), ("assistDRS", ctypes.c_int8), ("assistERS", ctypes.c_int8),
        ("assistBrake", ctypes.c_int8), ("aiLevel", ctypes.c_int8), ("penaltiesEnabled", ctypes.c_int8),
        ("setupFuelRate", ctypes.c_int8), ("tyreWearEnabled", ctypes.c_int8),
        ("setupTyreWearRate", ctypes.c_int8), ("damageEnabled", ctypes.c_int8),
        ("setupDamageRate", ctypes.c_int8), ("allowTyreBlankets", ctypes.c_int8),
        ("autoShift", ctypes.c_int8), ("autoClutch", ctypes.c_int8), ("autoPit", ctypes.c_int8),
        ("autoLift", ctypes.c_int8), ("autoBlip", ctypes.c_int8), ("autoReverse", ctypes.c_int8),
        ("HoldClutch", ctypes.c_int8), ("numStarters", ctypes.c_int8), ("standingStart", ctypes.c_int8),
        ("startingGrid", ctypes.c_int8), ("hardcoreLevel", ctypes.c_int8), ("incidentLimit", ctypes.c_int8),
        ("_padding", ctypes.c_uint8 * 3),
    ]

class SessionData(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("sessionType", ctypes.c_int8), ("sessionPhase", ctypes.c_int8), ("sessionMode", ctypes.c_int8),
        ("sessionEventId", ctypes.c_int8), ("sessionFormat", ctypes.c_int8),
        ("sessionUniqueId", ctypes.c_int8), ("sessionFlags", ctypes.c_int8),
        ("category", ctypes.c_int8), ("safetyCarStatus", ctypes.c_int8),
        ("numLaps", ctypes.c_int32), ("numSectors", ctypes.c_int8),
        ("sessionSectorYellow", ctypes.c_uint8 * 16),
        ("pitWindowStatus", ctypes.c_int8),
        ("sessionFrame", ctypes.c_float), ("sessionTimeOfDay", ctypes.c_float),
        ("sessionTimeRemain", ctypes.c_float), ("sessionTime", ctypes.c_float),
        ("lapsRemain", ctypes.c_int32),
        ("trackLength", ctypes.c_float), ("trackAltitude", ctypes.c_float),
        ("trackLatitude", ctypes.c_float), ("trackLongitude", ctypes.c_float),
        ("trackNorthOffset", ctypes.c_float), ("pitSpeedLimit", ctypes.c_float),
        ("trackId", ctypes.c_int32), ("trackLayoutId", ctypes.c_int32), ("numTurns", ctypes.c_int32),
        ("seriesId", ctypes.c_int32), ("seasonId", ctypes.c_int32),
        ("sessionId", ctypes.c_int32), ("subSessionId", ctypes.c_int32),
        ("leagueId", ctypes.c_int32), ("raceWeek", ctypes.c_int32),
        ("numCars", ctypes.c_int8), ("numCarClasses", ctypes.c_int8),
        ("listCarClasses", ctypes.c_uint16), ("numCarTypes", ctypes.c_int8),
        ("listCarTypes", ctypes.c_uint16), ("minDrivers", ctypes.c_int8),
        ("maxDrivers", ctypes.c_int8),
        ("options", AidSettings),
        ("environment", EnvironmentData),
        ("versionStr", ctypes.c_char * 32),
        ("trackName", ctypes.c_char * 64), ("trackLayoutName", ctypes.c_char * 64),
        ("trackCity", ctypes.c_char * 64), ("trackCountry", ctypes.c_char * 64),
    ]

class PlayerInfo(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("isLocalPlayer", ctypes.c_uint8),
        ("driverId", ctypes.c_int32), ("teamId", ctypes.c_int32), ("carLiveryId", ctypes.c_int32),
        ("carNumber", ctypes.c_int32), ("carClassId", ctypes.c_int32), ("carModelId", ctypes.c_int32),
        ("manufacturerId", ctypes.c_int32), ("carClassPerformanceIndex", ctypes.c_int32),
        ("driverStatus", ctypes.c_int8), ("isLastLap", ctypes.c_int8), ("isValidLap", ctypes.c_int8),
        ("lapValidState", ctypes.c_int8), ("isFinished", ctypes.c_int8), ("engineType", ctypes.c_int8),
        ("engineState", ctypes.c_int8), ("drsState", ctypes.c_int8), ("ptpState", ctypes.c_int8),
        ("inGarage", ctypes.c_int8), ("inPitlane", ctypes.c_int8), ("inPitBox", ctypes.c_int8),
        ("pitState", ctypes.c_int8), ("numPitstops", ctypes.c_int8),
        ("pitOptionalRepairLeft", ctypes.c_int8), ("pitRepairLeft", ctypes.c_int8),
        ("pitElapsedTime", ctypes.c_float), ("pitTotalDuration", ctypes.c_float),
        ("driverResult", ctypes.c_int8), ("gridPosition", ctypes.c_int8), ("position", ctypes.c_int8),
        ("classPosition", ctypes.c_int8), ("positionFinal", ctypes.c_int8),
        ("positionClassFinal", ctypes.c_int8), ("sector", ctypes.c_int8),
        ("deltaTimeFromGame", ctypes.c_int8),
        ("lapsCompleted", ctypes.c_int32), ("currentLap", ctypes.c_int32),
        ("lapDist", ctypes.c_float), ("lapDistPct", ctypes.c_float),
        ("worldPositionX", ctypes.c_float), ("worldPositionY", ctypes.c_float),
        ("worldPositionZ", ctypes.c_float),
        ("carSpeed", ctypes.c_float),
        ("bestLapNum", ctypes.c_int32), ("bestLapTime", ctypes.c_float),
        ("lastLapTime", ctypes.c_float), ("currentLapTime", ctypes.c_float),
        ("estimatedLapTime", ctypes.c_float), ("estimatedRaceFinishTime", ctypes.c_float),
        ("deltaTimeVersusPBNative", ctypes.c_float), ("deltaTimeVersusPB", ctypes.c_float),
        ("deltaTimeLastLap", ctypes.c_float), ("deltaTimeSessionBestLap", ctypes.c_float),
        ("deltaTimeOptimalLap", ctypes.c_float), ("deltaTimeSessionOptimalLap", ctypes.c_float),
        ("deltaTimeVersusClassLeader", ctypes.c_float), ("deltaTimeVersusLeader", ctypes.c_float),
        ("deltaTimeVersusFront", ctypes.c_float), ("deltaTimeVersusBehind", ctypes.c_float),
        ("lapsBehindFront", ctypes.c_int32), ("lapsBehindLeader", ctypes.c_int32),
        ("curSectorTime", ctypes.c_float * 16),
        ("lastSectorTime", ctypes.c_float * 16),
        ("bestSectorTime", ctypes.c_float * 16),
        ("numPenalties", ctypes.c_int8), ("numTeamPenalties", ctypes.c_int8),
        ("penaltiesDuration", ctypes.c_float), ("penaltyType", ctypes.c_int32),
        ("penaltyCauseId", ctypes.c_int32),
        ("cutDriveThroughCount", ctypes.c_int8), ("cutStopAndGoCount", ctypes.c_int8),
        ("cutPitStopCount", ctypes.c_int8), ("cutTimeDeductionCount", ctypes.c_int8),
        ("cutSlowDownCount", ctypes.c_int8),
        ("tyreVisualCompound", ctypes.c_int8), ("tyreCompound", ctypes.c_int8),
        ("rcJockerCount", ctypes.c_int8), ("_padding", ctypes.c_uint8 * 1),
        ("driverName", ctypes.c_char * 64), ("driverNickname", ctypes.c_char * 64),
        ("teamName", ctypes.c_char * 64), ("carName", ctypes.c_char * 64),
        ("className", ctypes.c_char * 32),
    ]

class PlayerRecords(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("reserved", ctypes.c_int32),
        ("carCount", ctypes.c_int32),
        ("player", PlayerInfo * 96),
    ]

class WheelInfo(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("tyreBrand", ctypes.c_int8), ("tyreVisualCompound", ctypes.c_int8), ("tyreCompound", ctypes.c_int8),
        ("tyrePressure", ctypes.c_float), ("tyrePressureCold", ctypes.c_float),
        ("tyrePressureLastHot", ctypes.c_float),
        ("tyreTempOverall", ctypes.c_float), ("tyreTempInner", ctypes.c_float),
        ("tyreTempMiddle", ctypes.c_float), ("tyreTempOuter", ctypes.c_float),
        ("tyreTempCarcass", ctypes.c_float), ("tyreTempCore", ctypes.c_float),
        ("tyreTempInnerLayer", ctypes.c_float * 3),
        ("tyreWearOverall", ctypes.c_float), ("tyreWearInner", ctypes.c_float),
        ("tyreWearMiddle", ctypes.c_float), ("tyreWearOuter", ctypes.c_float),
        ("tyreFlatSpot", ctypes.c_int32), ("tyreDirtyLevel", ctypes.c_float),
        ("tyreOnSurfaceType", ctypes.c_float),
        ("wheelSpeed", ctypes.c_float), ("wheelRotationSpeed", ctypes.c_float), ("wheelRadius", ctypes.c_float),
        ("patchVelocityLat", ctypes.c_float), ("patchVelocityLong", ctypes.c_float),
        ("groundVelocityLat", ctypes.c_float), ("groundVelocityLong", ctypes.c_float),
        ("lateralForce", ctypes.c_float), ("longitudinalForce", ctypes.c_float),
        ("wheelSlipRatio", ctypes.c_float), ("wheelSlipAngle", ctypes.c_float),
        ("wheelCombinedSlip", ctypes.c_float),
        ("tyreGrip", ctypes.c_float), ("verticalTyreDeflection", ctypes.c_float),
        ("tyreLoad", ctypes.c_float), ("contactPatch", ctypes.c_float),
        ("contactPoint", ctypes.c_float * 3), ("contactNormal", ctypes.c_float * 3),
        ("contactHeading", ctypes.c_float * 3),
        ("tyreRadius", ctypes.c_float), ("wheelCamberDeg", ctypes.c_float),
        ("wheelToe", ctypes.c_float), ("wheelCornerWeight", ctypes.c_float),
        ("wheelRumblePitch", ctypes.c_float), ("wheelSurfaceId", ctypes.c_int32),
        ("wheelOnRumbleStrip", ctypes.c_int8), ("wheelInPuddleDepth", ctypes.c_int8),
        ("SurfaceRumble", ctypes.c_int8), ("tyreStintIndex", ctypes.c_int8),
        ("tyreFlat", ctypes.c_uint8), ("tyreDetached", ctypes.c_uint8),
        ("setupTyreWearRate", ctypes.c_int8), ("chassisYaw", ctypes.c_float),
        ("_padding", ctypes.c_uint8 * 2),
    ]

class WheelData(ctypes.Structure):
    _pack_ = 1
    _fields_ = [("wheels", WheelInfo * 4)]

class BrakeData(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("brakePressure", ctypes.c_float), ("brakeDiscTemp", ctypes.c_float),
        ("brakePadTemp", ctypes.c_float), ("brakePadWear", ctypes.c_float),
        ("brakeDiscWear", ctypes.c_float), ("brakeTorque", ctypes.c_float),
        ("brakeOverheated", ctypes.c_int8), ("_padding", ctypes.c_uint8 * 7),
    ]

class SuspensionData(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("suspDeflection", ctypes.c_float), ("suspTravelNormalized", ctypes.c_float),
        ("suspDeflectionMax", ctypes.c_float), ("suspDeflectionMin", ctypes.c_float),
        ("suspVelocity", ctypes.c_float), ("damperVelocity", ctypes.c_float),
        ("suspForce", ctypes.c_float), ("rideHeight", ctypes.c_float),
        ("suspCamberDeg", ctypes.c_float), ("suspToeDeg", ctypes.c_float),
        ("suspCasterDeg", ctypes.c_float), ("_padding", ctypes.c_float),
    ]

class CarDamageData(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("damageZone", ctypes.c_int8 * 8),
        ("damageEngine", ctypes.c_int8), ("damageEngineBlown", ctypes.c_int8),
        ("damageEngineSeized", ctypes.c_int8),
        ("damageEngineWear", ctypes.c_float), ("damageEngineOverrev", ctypes.c_float),
        ("damageTransmission", ctypes.c_int8), ("damageGearboxWear", ctypes.c_float),
        ("damageClutch", ctypes.c_int8), ("damageClutchWear", ctypes.c_float),
        ("damageDifferential", ctypes.c_int8),
        ("damageChassis", ctypes.c_float), ("damageAero", ctypes.c_float),
        ("damageFrontWing", ctypes.c_float), ("damageRearWing", ctypes.c_float),
        ("damageFloor", ctypes.c_float),
        ("damageSuspensionLF", ctypes.c_float), ("damageSuspensionRF", ctypes.c_float),
        ("damageSuspensionLR", ctypes.c_float), ("damageSuspensionRR", ctypes.c_float),
        ("damageSteering", ctypes.c_float),
        ("damageBodywork", ctypes.c_int8), ("damageBodyIntegrity", ctypes.c_float),
        ("damageScratch", ctypes.c_float), ("damageDent", ctypes.c_float),
        ("damageBrake", ctypes.c_int8 * 4), ("damageTyre", ctypes.c_int8 * 4),
        ("damageExhaust", ctypes.c_int8), ("spare", ctypes.c_int8 * 4),
    ]

class WarningFlag(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("greenFlagWarning", ctypes.c_int8), ("redFlagWarning", ctypes.c_int8),
        ("yellowFlagWarning", ctypes.c_int8), ("fullYellowFlagWarning", ctypes.c_int8),
        ("redlineWarning", ctypes.c_int8), ("blueFlagWarning", ctypes.c_int8),
        ("blackFlagWarning", ctypes.c_int8), ("whiteFlagWarning", ctypes.c_int8),
        ("checkeredFlagWarning", ctypes.c_int8), ("meatballFlagWarning", ctypes.c_int8),
        ("blackWhiteFlagWarning", ctypes.c_int8), ("surfaceFlagWarning", ctypes.c_int8),
        ("orangeFlagWarning", ctypes.c_int8),
        ("flagsWarning", ctypes.c_int16),
        ("isSafetyCar", ctypes.c_int8), ("isSafetyCarPassing", ctypes.c_int8), ("isVSC", ctypes.c_int8),
        ("padWearWarning", ctypes.c_int8), ("turboStatus", ctypes.c_int8),
        ("headlights", ctypes.c_int8), ("startLights", ctypes.c_int8), ("espSignal", ctypes.c_int8),
        ("carSignals", ctypes.c_uint32),
        ("pitLimiterActive", ctypes.c_int8), ("pitRequest", ctypes.c_int8), ("tcKillSwitch", ctypes.c_int8),
        ("numCutTrackWarning", ctypes.c_int8), ("shiftIndicator", ctypes.c_int8),
        ("engineWarning", ctypes.c_int8), ("engineStallWarning", ctypes.c_int8),
        ("waterWarning", ctypes.c_int8), ("oilWarning", ctypes.c_int8),
        ("overHeatWarning", ctypes.c_int8), ("fuelWarning", ctypes.c_int8),
        ("voltageWarning", ctypes.c_int8), ("damageWarning", ctypes.c_int8),
        ("detachedParts", ctypes.c_int8), ("brakeWarning", ctypes.c_int8),
        ("clutchWarning", ctypes.c_int8), ("gearboxWarning", ctypes.c_int8),
        ("tyreWarning", ctypes.c_int8), ("transmissionWarning", ctypes.c_int8),
        ("penaltyWarning", ctypes.c_int8), ("revLightsPercent", ctypes.c_int8),
        ("m_activeAeroMode", ctypes.c_uint8), ("m_activeAeroAvailable", ctypes.c_uint8),
        ("m_activeAeroActivationDistance", ctypes.c_uint16),
        ("m_overtakeAvailable", ctypes.c_uint8), ("m_overtakeActive", ctypes.c_uint8),
        ("m_overtakeActivationDistance", ctypes.c_uint16),
        ("m_2026Regulations", ctypes.c_uint8), ("m_drivingWrongWay", ctypes.c_uint8),
        ("_padding", ctypes.c_uint8 * 6),
    ]

class EnergySystemData(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("ersStoreEnergy", ctypes.c_float), ("ersStoreEnergyMax", ctypes.c_float),
        ("ersStoreEnergyPercent", ctypes.c_float),
        ("ersDeployPower", ctypes.c_float), ("ersDeployedThisLap", ctypes.c_float),
        ("ersDeployLimitPerLap", ctypes.c_float),
        ("ersDeployMode", ctypes.c_int8), ("ersAllowDeploy", ctypes.c_int8),
        ("ersHarvestedThisLapMGUK", ctypes.c_float), ("ersHarvestedThisLapMGUH", ctypes.c_float),
        ("ersHarvestedThisLapOther", ctypes.c_float),
        ("ersHarvestPowerMGUK", ctypes.c_float), ("ersHarvestPowerMGUH", ctypes.c_float),
        ("ersTorqueMGUK", ctypes.c_float),
        ("kersStoreEnergy", ctypes.c_float), ("kersStoreEnergyMax", ctypes.c_float),
        ("kersStorePercent", ctypes.c_float), ("kersDeployPower", ctypes.c_float),
        ("kersDeployedThisLap", ctypes.c_float),
        ("kersActvated", ctypes.c_int8), ("kersAvailable", ctypes.c_int8),
        ("ersBatteryTemp", ctypes.c_float), ("ersMGUKTemp", ctypes.c_float),
        ("ersMGUHTemp", ctypes.c_float), ("ersEfficiency", ctypes.c_float),
        ("hasERS", ctypes.c_int8), ("ersFault", ctypes.c_int8), ("ersOverheated", ctypes.c_int8),
        ("ersHarvestingActive", ctypes.c_int8), ("ersDeployingActive", ctypes.c_int8),
        ("hasKERS", ctypes.c_int8), ("ersRecoveryMode", ctypes.c_int8),
        ("_padding", ctypes.c_uint8 * 1),
    ]

class CarSetupState(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("diffEntry", ctypes.c_float), ("diffMiddle", ctypes.c_float), ("diffExit", ctypes.c_float),
        ("engineMap", ctypes.c_float), ("engineBraking", ctypes.c_float), ("throttleShape", ctypes.c_float),
        ("frontAntiRollBar", ctypes.c_float), ("rearAntiRollBar", ctypes.c_float),
        ("weightJackerLeft", ctypes.c_float), ("weightJackerRight", ctypes.c_float),
        ("frontWing", ctypes.c_float), ("rearWing", ctypes.c_float),
    ]

class VehicleData(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("speed", ctypes.c_float),
        ("gear", ctypes.c_int8), ("maxGear", ctypes.c_uint8),
        ("gearIndexNeutral", ctypes.c_uint8), ("gearIndexReverse", ctypes.c_uint8),
        ("transmissionSpeed", ctypes.c_float),
        ("throttle", ctypes.c_float), ("throttleRaw", ctypes.c_float), ("throttleTravel", ctypes.c_float),
        ("brake", ctypes.c_float), ("brakeRaw", ctypes.c_float),
        ("handbrake", ctypes.c_float), ("handbrakeRaw", ctypes.c_float),
        ("clutch", ctypes.c_float), ("clutchRaw", ctypes.c_float), ("clutchTravel", ctypes.c_float),
        ("rpm", ctypes.c_float), ("engineTorque", ctypes.c_float), ("idleRpm", ctypes.c_float),
        ("maxRpm", ctypes.c_float), ("engineTemp", ctypes.c_float),
        ("ignitionOn", ctypes.c_uint8), ("driverTrainType", ctypes.c_uint8),
        ("NumCylinders", ctypes.c_uint8), ("antiStallActivated", ctypes.c_uint8),
        ("oilTemp", ctypes.c_float), ("oilPress", ctypes.c_float),
        ("waterTemp", ctypes.c_float), ("waterLevel", ctypes.c_float),
        ("exhaustTemp", ctypes.c_float), ("fuelPressure", ctypes.c_float),
        ("voltage", ctypes.c_float), ("batterySOC", ctypes.c_float),
        ("fuelLevel", ctypes.c_float), ("fuelLevelPct", ctypes.c_float),
        ("fuelCapacity", ctypes.c_float), ("fuelUsePerHour", ctypes.c_float),
        ("fuelPerLap", ctypes.c_float), ("fuelEstimatedLaps", ctypes.c_float),
        ("setupFuelRate", ctypes.c_int8), ("fuelMix", ctypes.c_int8),
        ("turboLevel", ctypes.c_float), ("maxTurbo", ctypes.c_float),
        ("boost", ctypes.c_float), ("maxBoost", ctypes.c_float),
        ("maxTorque", ctypes.c_float), ("maxPower", ctypes.c_float),
        ("antiLockBrakes", ctypes.c_int8), ("absInAction", ctypes.c_int8),
        ("tractionControl", ctypes.c_int8), ("tractionControl2", ctypes.c_int8),
        ("tcInAction", ctypes.c_int8),
        ("brakeBias", ctypes.c_float), ("frontBrakeBias", ctypes.c_float), ("rearBrakeBias", ctypes.c_float),
        ("hasDRS", ctypes.c_int8), ("drsAllowed", ctypes.c_int8), ("drsEnabled", ctypes.c_int8),
        ("drsEngaged", ctypes.c_int8), ("drsState", ctypes.c_int8),
        ("drsNumActivationsLeft", ctypes.c_int8), ("drsNumActivationsTotal", ctypes.c_int8),
        ("drsActivationDistance", ctypes.c_int16),
        ("hasPTP", ctypes.c_int8), ("ptpEngaged", ctypes.c_int8),
        ("ptpActivationLeft", ctypes.c_int8), ("ptpActivationTotal", ctypes.c_int8),
        ("_padding", ctypes.c_uint8 * 3),
        ("currentDownforce", ctypes.c_float), ("frontWingHeight", ctypes.c_float),
        ("frontRideHeight", ctypes.c_float), ("rearRideHeight", ctypes.c_float),
        ("frontRollAngle", ctypes.c_float), ("rearRollAngle", ctypes.c_float),
        ("frontThirdSpringDeflection", ctypes.c_float), ("frontThirdSpringVelocity", ctypes.c_float),
        ("rearThirdSpringDeflection", ctypes.c_float), ("rearThirdSpringVelocity", ctypes.c_float),
        ("cgHeight", ctypes.c_float), ("heightOfCOGAboveGround", ctypes.c_float),
        ("carLength", ctypes.c_float), ("carWidth", ctypes.c_float), ("totalMass", ctypes.c_float),
        ("steer", ctypes.c_float), ("steerLock", ctypes.c_float), ("steerAngle", ctypes.c_float),
        ("steerPeakForceNm", ctypes.c_float), ("steeringTorque", ctypes.c_float),
        ("steeringPctTorque", ctypes.c_uint8),
        ("odometerKm", ctypes.c_float), ("totalDistance", ctypes.c_float),
        ("kerbVibration", ctypes.c_float), ("slipVibrations", ctypes.c_float),
        ("gVibrations", ctypes.c_float), ("absVibrations", ctypes.c_float),
        ("brakes", BrakeData * 4),
        ("suspensions", SuspensionData * 4),
        ("carDamage", CarDamageData),
        ("warningFlag", WarningFlag),
        ("energySystem", EnergySystemData),
        ("carSetupState", CarSetupState),
    ]

class ExtensionData(ctypes.Structure):
    _pack_ = 1
    _fields_ = [("extension", ctypes.c_float * 256)]

class OutSimData(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("mPacketHeader", PacketHeader),
        ("mPlayers",      PlayerRecords),
        ("mMotionData",   MotionData),
        ("mSessionData",   SessionData),
        ("mVehicleData",   VehicleData),
        ("mWheelData",     WheelData),
        ("mExtensionData", ExtensionData),
    ]

# Equivalent of the C++ static_assert: catches layout mistakes immediately
HEADER_SIZE = ctypes.sizeof(PacketHeader)
STRUCT_SIZE = ctypes.sizeof(OutSimData)
assert HEADER_SIZE == 32, "PacketHeader layout is wrong"
assert STRUCT_SIZE == 68336, "OutSimData layout is wrong"

# ===========================================================================
# Reader thread
# ===========================================================================
class SharedMemoryReader(threading.Thread):
    def __init__(self, name=SHARED_MEM_OUTSIMDATA, host_exe=TARGET_HOST_PROCESS,
                 idle_sleep=0.010, retry_sleep=0.5,
                 alive_check_every=0.25, scan_throttle=2.0):
        super().__init__(daemon=True)
        self.mem_name = name
        self.host_exe = host_exe
        self.idle_sleep = idle_sleep
        self.retry_sleep = retry_sleep
        self.alive_check_every = alive_check_every
        self.scan_throttle = scan_throttle

        self.status = "starting"
        self.connected = False
        self.host_pid = None

        self._stop_evt = threading.Event()
        self._lock = threading.Lock()
        self._latest = None
        self._last_frame_id = None
        self._last_alive_check = 0.0
        self._last_scan = 0.0

        self._handle = None     # file mapping handle
        self._view = None       # mapped view address
        self._proc = None       # host process handle

    # ------------------------------------------------------------ public
    def get_latest(self):
        with self._lock:
            return self._latest

    def stop(self):
        self._stop_evt.set()

    # ------------------------------------------------------------ host process
    def _attach_host(self):
        """Find host PID (throttled) and hold a handle on it."""
        now = time.perf_counter()
        if now - self._last_scan < self.scan_throttle:
            return False
        self._last_scan = now

        pid = find_pid(self.host_exe)
        if pid is None:
            return False

        h = k32.OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE,
                            False, pid)
        if not h:
            return False

        # Holding the handle means the PID can't be recycled under us.
        self._proc, self.host_pid = h, pid
        return True

    def host_alive(self):
        # WAIT_TIMEOUT with 0 ms => process has not exited yet.
        return bool(self._proc) and k32.WaitForSingleObject(self._proc, 0) == WAIT_TIMEOUT

        # Alternative (needs only PROCESS_QUERY_LIMITED_INFORMATION):
        #   code = wintypes.DWORD()
        #   if not k32.GetExitCodeProcess(self._proc, ctypes.byref(code)): return False
        #   return code.value == 259   # STILL_ACTIVE
        # (declare GetExitCodeProcess argtypes/restype if you use it)

    # ------------------------------------------------------------ mapping
    def _open(self):
        """Open an EXISTING mapping read-only (never creates), then bind to host."""
        h = k32.OpenFileMappingW(FILE_MAP_READ, False, self.mem_name)
        if not h:
            err = ctypes.get_last_error()
            self.status = (f"waiting for '{self.mem_name}' ..."
                           if err == ERROR_FILE_NOT_FOUND
                           else f"OpenFileMapping failed (error {err})")
            return False

        view = k32.MapViewOfFile(h, FILE_MAP_READ, 0, 0, 0)   # 0 = whole mapping
        if not view:
            err = ctypes.get_last_error()
            k32.CloseHandle(h)
            self.status = f"MapViewOfFile failed (error {err})"
            return False

        # Make sure the mapping is big enough, otherwise we'd read past it.
        mbi = MEMORY_BASIC_INFORMATION()
        if (k32.VirtualQuery(view, ctypes.byref(mbi), ctypes.sizeof(mbi)) == 0
                or mbi.RegionSize < STRUCT_SIZE):
            size = mbi.RegionSize
            k32.UnmapViewOfFile(view)
            k32.CloseHandle(h)
            self.status = f"mapping too small ({size} < {STRUCT_SIZE})"
            return False

        self._handle, self._view = h, view

        # Mapping exists: bind to the host process. If we can't, don't stay
        # attached to a mapping that may be a leftover someone else holds.
        if not self._attach_host():
            self._close()
            self.status = f"'{self.mem_name}' found, waiting for {self.host_exe} ..."
            return False

        self._last_frame_id = None
        self._last_alive_check = time.perf_counter()
        self.connected = True
        self.status = f"connected to {self.host_exe} (pid {self.host_pid})"
        return True

    def _close(self):
        if self._view:
            k32.UnmapViewOfFile(self._view)
            self._view = None
        if self._handle:
            k32.CloseHandle(self._handle)
            self._handle = None
        if self._proc:
            k32.CloseHandle(self._proc)
            self._proc = None
        self.host_pid = None
        self.connected = False

    def _disconnect(self, reason):
        self._close()
        with self._lock:
            self._latest = None            # UI falls back to "waiting"
        self._last_frame_id = None
        self._last_scan = 0.0              # allow immediate re-scan for the new host
        self.status = reason

    def _read(self, cls, size=None):
        """Private copy of the first `size` bytes of the view as `cls`."""
        return cls.from_buffer_copy(
            ctypes.string_at(self._view, size or ctypes.sizeof(cls)))

    # ------------------------------------------------------------ thread
    def run(self):
        try:
            while not self._stop_evt.is_set():
                # ---- (re)connect ------------------------------------------
                if not self.connected:
                    if not self._open():
                        self._stop_evt.wait(self.retry_sleep)
                        continue

                # ---- host liveness ----------------------------------------
                now = time.perf_counter()
                if now - self._last_alive_check >= self.alive_check_every:
                    self._last_alive_check = now
                    if not self.host_alive():
                        self._disconnect(f"{self.host_exe} exited, waiting ...")
                        self._stop_evt.wait(self.retry_sleep)
                        continue

                # ---- header peek ------------------------------------------
                hdr = self._read(PacketHeader, HEADER_SIZE)
                if hdr.reportAvailable != 1 or hdr.frameId == self._last_frame_id:
                    time.sleep(self.idle_sleep)
                    continue

                # ---- full snapshot + torn-read check ------------------------
                snap = self._read(OutSimData, STRUCT_SIZE)
                if (snap.mPacketHeader.reportAvailable != 1
                        or snap.mPacketHeader.frameId != hdr.frameId):
                    continue                # writer changed the frame mid-copy

                self._last_frame_id = hdr.frameId
                with self._lock:
                    self._latest = snap
                time.sleep(self.idle_sleep)
        finally:
            self._close()


# ===========================================================================
# Console display
# ===========================================================================
RESET, DIM, YELLOW, CYAN, GREEN = "\x1b[0m", "\x1b[2m", "\x1b[33;1m", "\x1b[36m", "\x1b[32m"
_ANSI = re.compile(r"(\x1b\[[0-9;?]*[A-Za-z])")


def fit(line, width):
    """Truncate to `width` visible chars, keeping ANSI codes intact."""
    out, visible = [], 0
    for part in _ANSI.split(line):
        if _ANSI.fullmatch(part):
            out.append(part)
        else:
            room = width - visible
            if room <= 0:
                continue
            out.append(part[:room])
            visible += min(len(part), room)
    return "".join(out)


class Screen:
    def __init__(self):
        if os.name == "nt":
            os.system("")                                # enable ANSI on Windows 10+
        self._out = sys.stdout
        self._prev = []
        self._size = None
        self._out.write("\x1b[?1049h\x1b[?25l\x1b[2J")   # alt screen, hide cursor, clear once
        self._out.flush()

    def close(self):
        self._out.write("\x1b[0m\x1b[?25h\x1b[?1049l")   # show cursor, leave alt screen
        self._out.flush()

    def draw(self, lines):
        cols, rows = shutil.get_terminal_size((120, 40))
        if len(lines) > rows - 1:                        # never scroll
            hidden = len(lines) - (rows - 2)
            lines = lines[:rows - 2] + [f"{DIM}... {hidden} more lines (enlarge the window){RESET}"]
        lines = [fit(l, cols - 1) for l in lines]

        buf = []
        if (cols, rows) != self._size:                   # resize -> full repaint
            buf.append("\x1b[2J")
            self._prev = []
            self._size = (cols, rows)

        for i, line in enumerate(lines):
            if i >= len(self._prev) or self._prev[i] != line:
                buf.append(f"\x1b[{i + 1};1H{line}{RESET}\x1b[K")
        for i in range(len(lines), len(self._prev)):     # frame got shorter
            buf.append(f"\x1b[{i + 1};1H\x1b[K")
        self._prev = lines

        if buf:
            self._out.write("\x1b[?2026h" + "".join(buf) + "\x1b[?2026l")
            self._out.flush()

def iter_fields(obj, path="", base=0, max_items=8, show_private=False):
    """Recursively yield (path, value, absolute_offset) for every leaf field."""
    if isinstance(obj, ctypes.Structure):
        cls = type(obj)
        for name, *_ in cls._fields_:
            if name.startswith("_") and not show_private:
                continue
            off = getattr(cls, name).offset
            sub = f"{path}.{name}" if path else name
            yield from iter_fields(getattr(obj, name), sub, base + off,
                                   max_items, show_private)
    elif isinstance(obj, ctypes.Array):
        et = obj._type_
        if et is ctypes.c_char:
            yield path, obj.value.decode(errors="replace"), base
        elif issubclass(et, (ctypes.Structure, ctypes.Array)):
            esz = ctypes.sizeof(et)
            for i in range(min(len(obj), max_items)):
                yield from iter_fields(obj[i], f"{path}[{i}]", base + i * esz,
                                       max_items, show_private)
            if len(obj) > max_items:
                yield f"{path}[...]", f"({len(obj) - max_items} more not shown)", None
        else:
            yield path, list(obj), base
    else:
        yield path, obj, base


def fmt_value(v):
    if isinstance(v, float):
        return f"{v:14.4f}"
    if isinstance(v, list):
        return "[" + ", ".join(f"{x:.3f}" if isinstance(x, float) else str(x) for x in v) + "]"
    if isinstance(v, int):
        return f"{v:14d}"
    return str(v)

def fmt_compact(v):
    if isinstance(v, float):
        return f"{v:.3f}"
    if isinstance(v, list):
        return "[" + ",".join(f"{x:.2f}" if isinstance(x, float) else str(x) for x in v) + "]"
    return str(v)


def left_ellipsis(s, n):
    """Keep the END of the path (the field name is the informative part)."""
    return s if len(s) <= n else "…" + s[-(n - 1):]


class Monitor:
    """Builds fixed-width cells for ONE view (a sub struct, or ALL)."""
    def __init__(self, max_items=8, show_offsets=False, show_private=False,
                 max_name=36, max_val=20):
        self.max_items = max_items
        self.show_offsets = show_offsets
        self.show_private = show_private
        self.max_name = max_name
        self.max_val = max_val
        self._prev = {}
        self._name_w = 0
        self._val_w = 0

        # every member of OutSimData, found automatically (works with 10+ subs)
        self.all_sections = [n for n, _ in OutSimData._fields_]
        self.views = ["ALL"] + self.all_sections
        self.sec_index = 1                      # start on the first sub struct

    def cycle(self, step):
        self.sec_index = (self.sec_index + step) % len(self.views)
        self._prev.clear()                      # no false "changed" flashes
        self._name_w = self._val_w = 0          # re-fit column widths

    def build_cells(self, data):
        view = self.views[self.sec_index]
        names = self.all_sections if view == "ALL" else [view]

        rows = []
        for sec in names:
            sub = getattr(data, sec)
            base = getattr(type(data), sec).offset      # absolute offsets stay correct
            for p, v, off in iter_fields(sub, path=sec, base=base,
                                         max_items=self.max_items,
                                         show_private=self.show_private):
                rows.append((p, fmt_compact(v), v, off))

        # widths only grow, so the layout doesn't jump
        self._name_w = max(self._name_w,
                           min(max((len(p) for p, *_ in rows), default=8), self.max_name))
        self._val_w = max(self._val_w,
                          min(max((len(s) for _, s, *_ in rows), default=4), self.max_val))
        name_w, val_w = self._name_w, self._val_w

        cells = []
        for path, sval, raw, off in rows:
            changed = path in self._prev and self._prev[path] != raw
            self._prev[path] = raw
            color = YELLOW if changed else ""
            name = left_ellipsis(path, name_w).ljust(name_w)
            val = sval[:val_w].rjust(val_w)
            off_s = f"{DIM}{off:>5d}{RESET} " if (self.show_offsets and off is not None) else ""
            cells.append(f"{off_s}{name} {color}{val}{RESET}")
        cell_w = name_w + 1 + val_w + (6 if self.show_offsets else 0)
        return cells, cell_w


def compose(cells, cell_w, header, page):
    """Column-major layout, balanced columns, paginated so it always fits.
    Returns (lines, page_used, page_count)."""
    cols, rows = shutil.get_terminal_size((120, 40))
    sep = f" {DIM}│{RESET} "
    ncols = max(1, (cols - 1 + 3) // (cell_w + 3))

    body_max = max(1, rows - 1 - len(header))        # header lines + 1 footer line
    if len(cells) <= body_max * ncols:               # everything fits: balance columns
        body = max(1, math.ceil(len(cells) / ncols))
    else:
        body = body_max
    per_page = body * ncols
    pages = max(1, math.ceil(len(cells) / per_page))
    page = max(0, min(page, pages - 1))
    chunk = cells[page * per_page:(page + 1) * per_page]

    lines = list(header)
    blank = " " * cell_w
    for r in range(body):
        parts = []
        for c in range(ncols):
            i = c * body + r
            parts.append(chunk[i] if i < len(chunk) else blank)
        if r < len(chunk) or r == 0:                 # skip fully empty rows
            lines.append(sep.join(parts))
    lines.append(f"{DIM}page {page + 1}/{pages}   {len(cells)} fields   {ncols} col(s)   "
                 f"[←/→ struct]  [↑/↓ page]  [o offsets]  [r refit]  [q quit]{RESET}")
    return lines, page, pages

class ConsoleMonitor:
    def __init__(self, max_items=8, show_offsets=True, show_private=False):
        self.max_items = max_items
        self.show_offsets = show_offsets
        self.show_private = show_private
        self._prev = {}
        self._first = True
        if os.name == "nt":
            os.system("")                    # enable ANSI escapes on Windows 10+

    def draw(self, lines):
        text = "\n".join(l + "\x1b[K" for l in lines)
        prefix = "\x1b[2J\x1b[H" if self._first else "\x1b[H"
        self._first = False
        sys.stdout.write(prefix + text + "\n\x1b[J")
        sys.stdout.flush()

    def render(self, data, status_line=""):
        rows = list(iter_fields(data, max_items=self.max_items, show_private=self.show_private))
        width = max((len(p) for p, _, _ in rows), default=10)

        out = [f"{CYAN}=== OpenSR shared memory monitor ==={RESET}  "
               f"{DIM}(Ctrl+C to quit, yellow = changed){RESET}",
               status_line, ""]
        for path, val, off in rows:
            changed = path in self._prev and self._prev[path] != val
            self._prev[path] = val
            color = YELLOW if changed else ""
            off_s = f"{DIM}+{off:<5d}{RESET}" if (self.show_offsets and off is not None) else " " * 6
            out.append(f"{path:<{width}} {off_s} {color}{fmt_value(val)}{RESET}")
        self.draw(out)


# ===========================================================================
if __name__ == "__main__":
    reader = SharedMemoryReader()
    reader.start()
    mon = Monitor(max_items=4, show_offsets=False)
    screen = Screen()
    refresh = 0.05
    page = 0

    last_fid, last_t, rate = None, time.perf_counter(), 0.0
    try:
        running = True
        while running:
            # ---- keys (non-blocking) ----
            while msvcrt.kbhit():
                k = msvcrt.getwch()
                if k in ("\x00", "\xe0"):                 # arrow keys = 2 chars
                    k2 = msvcrt.getwch()
                    if k2 == "M":                         # right
                        mon.cycle(+1); page = 0
                    elif k2 == "K":                       # left
                        mon.cycle(-1); page = 0
                    elif k2 == "H":                       # up
                        page -= 1
                    elif k2 == "P":                       # down
                        page += 1
                elif k in ("q", "Q"):
                    running = False
                elif k in ("o", "O"):
                    mon.show_offsets = not mon.show_offsets
                    mon._name_w = mon._val_w = 0
                elif k in ("r", "R"):
                    mon._name_w = mon._val_w = 0
                elif k in ("d", "D"):
                    page += 1
                elif k in ("a", "A"):
                    page -= 1

            d = reader.get_latest()
            if d is None:
                last_fid, rate = None, 0.0      # reset fr/s after a disconnect
                screen.draw([f"{CYAN}=== shared memory monitor ==={RESET}",
                             f"reader: {reader.status}",
                             f"{DIM}(q to quit){RESET}"])
            else:
                h = d.mPacketHeader
                now = time.perf_counter()
                if last_fid is None:
                    last_fid, last_t = h.frameId, now
                elif now - last_t >= 0.5:
                    rate = (h.frameId - last_fid) / (now - last_t)
                    last_fid, last_t = h.frameId, now

                state = f"{YELLOW}PAUSED{RESET}" if h.paused else f"{GREEN}RUNNING{RESET}"
                header = [
                    f"{CYAN}=== OpenSR monitor ==={RESET}  "
                    f"state: {state}   frameId: {h.frameId}  Report Available: {h.reportAvailable} "
                    f"{rate:5.1f} fr/s   {DIM}{reader.status}{RESET}",
                    f"{CYAN}view: {mon.views[mon.sec_index]}{RESET}  "
                    f"({mon.sec_index}/{len(mon.views) - 1})",
                ]
                cells, cell_w = mon.build_cells(d)
                lines, page, _ = compose(cells, cell_w, header, page)
                screen.draw(lines)
            time.sleep(refresh)
    except KeyboardInterrupt:
        pass
    finally:
        screen.close()
        reader.stop()
        reader.join()