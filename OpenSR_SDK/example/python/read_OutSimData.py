# IMPORTANT: Do NOT use the native 'mmap' module. Native mmap calls CreateFileMapping 
# internally, which will create a new shared memory segment if it doesn't exist.
# To avoid corrupting or creating stale buffers, we use kernel32 calls to 
# specifically OPEN an existing mapping and wait for its availability.

import ctypes
from ctypes import wintypes
import time

# Windows API Constants
FILE_MAP_READ = 0x04
ERROR_FILE_NOT_FOUND = 2

k32 = ctypes.windll.kernel32

# --- Kernel32 Function Declarations from osr_shared_mem_test.py [1] ---
k32.OpenFileMappingW.restype = wintypes.HANDLE
k32.OpenFileMappingW.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.LPCWSTR]

k32.MapViewOfFile.restype = ctypes.c_void_p
k32.MapViewOfFile.argtypes = [wintypes.HANDLE, wintypes.DWORD, wintypes.DWORD, wintypes.DWORD, ctypes.c_size_t]

k32.UnmapViewOfFile.restype = wintypes.BOOL
k32.UnmapViewOfFile.argtypes = [ctypes.c_void_p]

k32.CloseHandle.restype = wintypes.BOOL
k32.CloseHandle.argtypes = [wintypes.HANDLE]

# --- Data Structures ---
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

# Minimal placeholders for OutSimData sub-structures to keep example clean
class PlayerRecords(ctypes.Structure): _pack_ = 1; _fields_ = [("dummy", ctypes.c_byte * 64520)]
class MotionData(ctypes.Structure): _pack_ = 1; _fields_ = [("dummy", ctypes.c_byte * 448)]
class SessionData(ctypes.Structure): _pack_ = 1; _fields_ = [("dummy", ctypes.c_byte * 528)]
class VehicleData(ctypes.Structure): 
    _pack_ = 1
    _fields_ = [("speed", ctypes.c_float), ("gear", ctypes.c_int8), ("dummy", ctypes.c_byte * 915)]
class WheelData(ctypes.Structure): _pack_ = 1; _fields_ = [("dummy", ctypes.c_byte * 864)]
class ExtensionData(ctypes.Structure): _pack_ = 1; _fields_ = [("extension", ctypes.c_byte * 1024)]

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

HEADER_SIZE = ctypes.sizeof(PacketHeader)
STRUCT_SIZE = ctypes.sizeof(OutSimData)
assert HEADER_SIZE == 32, "PacketHeader layout is wrong"
assert STRUCT_SIZE == 68336, "OutSimData layout is wrong"

def main():
    shared_mem_name = "Local\\SMOSROUTSIMDATA"
    print(f"Waiting for {shared_mem_name}...")

    while True:
        # 1. Open existing mapping [1]
        h_map = k32.OpenFileMappingW(FILE_MAP_READ, False, shared_mem_name)
        if not h_map:
            if ctypes.get_last_error() == ERROR_FILE_NOT_FOUND:
                time.sleep(1)
                continue
            else:
                print(f"Error opening mapping: {ctypes.get_last_error()}")
                return

        # 2. Map the view [1]
        ptr = k32.MapViewOfFile(h_map, FILE_MAP_READ, 0, 0, 0)
        if not ptr:
            k32.CloseHandle(h_map)
            return

        print("Connected. Reading data...")
        try:
            # Map the pointer to the structure [1]
            # data = OutSimData.from_address(ptr)
            
            while True:
                 # 1. Get the snapshot
                data = OutSimData.from_buffer_copy(ctypes.string_at(ptr, ctypes.sizeof(OutSimData)))

                # Check if report is available and not a torn read [1]
                if data.mPacketHeader.reportAvailable == 1:
                    # Check if game is paused
                    status = "PAUSED" if data.mPacketHeader.paused else "RUNNING"
                    print(f"[{status}] FRAME ID: {data.mPacketHeader.frameId} | SPEED: {data.mVehicleData.speed*3.6:.0f}  km/h | GEAR: {(data.mVehicleData.gear):.0f}", end="\r")
                else:
                    print("Data not available or updating...", end="\r")
                
                time.sleep(0.01)
        except KeyboardInterrupt:
            print("\nStopping...")
        finally:
            k32.UnmapViewOfFile(ptr)
            k32.CloseHandle(h_map)
            break

if __name__ == "__main__":
    main()