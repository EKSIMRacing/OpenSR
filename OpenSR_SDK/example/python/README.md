# OpenSR Python Shared Memory Example

Python example demonstrating how to interface with **OpenSR** using Windows Shared Memory. This script serves as a foundation for developers who wish to build custom Python applications that read real-time simulation data and application context from OpenSR.

## Overview

The example script `osr_shared_mem_test.py` demonstrates the technical workflow required to access memory mapped by OpenSR. Since OpenSR uses Windows named shared memory, the Python script uses the `ctypes` library to call native Windows API functions (from `kernel32.dll` and `ntdll.dll`) to locate the process and map the memory segments into its own address space.

## Key Technical Concepts

### 1. Shared Memory Segments

OpenSR exposes 3 primary shared memory areas for external applications:

these structures of data are passed to OpenSR plugins (see the doc of plugin interface) but also sync'ed with the shared memory below:

* **Context Data**: Accessed via the name `Local\SMOSRCONTEXTDATA`. This contains application-level state and configuration.
* **Simulation Data**: Accessed via the name `Local\SMOSROUTSIMDATA`. This contains the high-frequency simulation data (e.g., session, vehicle, player data).
* **Blob Data:** Accessed via the name `Local\SMOSRBLOBDATA`This contains the high-frequency multi purpose data for custom solution

### 2. OpenSR Context Data

For developers, the `OpenSRContextData` structure is particularly useful for synchronizing the Python app with the state of the OpenSR application. It includes:

* `isAppRunning`: Boolean indicating if the application is active.
* `targetGamePId`: The Process ID of the game being tracked.
* `osrAppFolder` & `osrDocFolder`: Paths to the OpenSR application and OpenSR document directories.
* `currentProfilePath`: The path to the currently active game profile.

### 3. Implementation Workflow

The provided script follows these steps to establish a connection:

1. **Process Discovery or waiting for shared memory**: It searches for the `OpenSR.exe` process to ensure the host is running.
2. **Opening the Mapping**: It uses `OpenFileMappingW` with `FILE_MAP_READ` permissions to gain access to the named memory segment without modifying it.
3. **Mapping the View**: It calls `MapViewOfFile` to map the shared memory into the Python process's virtual address space.
4. **Data Interpretation**: It uses `ctypes.Structure` (with `_pack_ = 1`) to mirror the C++ memory layout used by OpenSR, allowing Python to read the raw bytes as typed variables exactly like the c++ plugin do.

## Prerequisites

* **Operating System**: Windows 10/11+
* **Python**: 3.8+
* **Dependencies**: The script uses standard libraries (`ctypes`, `threading, re, shutil, msvcrt, math, time, etc.`).

## Usage

1. Start **OpenSR**.

2. Run the example script:
   
   ```bash
   python osr_shared_mem_test.py
   ```

3. The script will attempt to find the OpenSR process, attach to the shared memory, and begin reading the available data when a game is running.

## Developer Notes

When extending this example, ensure that any `ctypes.Structure` you define matches the exact alignment and padding of the OpenSR source code (use `_pack_ = 1` to avoid compiler-added padding). Always open the memory mapping in read-only mode (`FILE_MAP_READ`) to prevent accidental corruption of the simulation data.

## License

This script is provided under the OpenSR Plugin Interface License. Redistribution is permitted *only* as part of developing plugins for OpenSR, and subject to the terms of the license provided with the SDK.