# OpenSR SDK

The OpenSR SDK provides the interfaces, structures, and conventions required to create plugins for the OpenSR ecosystem.
OpenSR is a modular, host-driven platform: telemetry producers (games), hardware outputs, dashboards, overlays, and configuration systems are all implemented as independent plugins. 

The OpenSR SDK is now available for developers building plugins for OpenSR. During this development phase, you can get the SDK here on this repo and support to registered developer is available on our forum.

With the SDK you can build:

- **Game (IN) plugins** — produce telemetry

- **OUT plugins** — consume telemetry and drive devices / inputs

- **Dashboard UI and Settings plugins**

- Device integrations, telemetry bridges, motion systems, network protocols
  
  ## Design

- Pure Win32 / C++ — no managed runtime required

- Compatible with older Visual Studio versions

- Interoperable with any language that can call native DLL exports
  (C++, C#, Rust, Python, …) included an official C# wrapper for .NET
  
  ## Documentation
  
  See the [doc/](doc/) folder for the full documentation: introduction, SDK download,
  architecture overview, the plugin UI & settings system, and plugin creation guides.
  
  ## Discussion
  
  Project discussion happens on our forum: https://www.eksimracing.org/forum/index.php#c69
