# Open Sim Relay (OpenSR)   Recently updated !

Any Game In. Any Hardware Out. The open platform that connects your sim racing hardware to every game.

**Open Sim Relay (OpenSR)** A 100% free and open middleware designed to bridge applications that send data or telemetry (such as games) to motion simulators, robotic systems, sim racing hardware, steering wheels, digital dashboards, leaderboard systems, telemetry applications, mobile devices (Android, iOS), IoT systems, USB devices, and more.

OpenSR provides a flexible, plugin-based architecture that collects telemetry from **IN (Source)** plugins and relays it to **OUT (Destination)** plugins.

It is designed for sim racers, telemetry enthusiasts, hardware developers, or anyone who wants to connect virtual data to real-world systems in a fully adaptable way.

> OpenSR is the new free application that replaces everything I have developed for the sim racing community since 2007. You could say this is my legacy project. No licenses, no activation fees, no paid versions. OpenSR is free for everyone.  
> Cheers!  
> Zappadoc

DOWNLOAD

***Status:*** *OpenSR Early Access Available*

### Key Features

- **Modular Plugin System**: Two plugin types: IN plugins capture telemetry from games or applications, OUT plugins send processed data to hardware, dashboards, motion systems, or any external target.

- Built-in **Dashboard**: Central interface to manage Smart Launcher automation, enable or disable plugins, manage per plugin settings, and handle game profiles.

- **Smart Launcher**: Event based automation system to launch programs or scripts on button press, key input, or game start.

- **Macros**: Integrated macro system to simulate keystroke on button/encoder press or on using rotary switch,

- **Game and Device Plugins**: Support for major games and a wide range of hardware including Thrustmaster, Fanatec, Leo Bodnar SLI display devices, SIM Display display devices, VDASH-EMU digital dashboard, Vocore LCD, Stream Deck and more.

- Built-in **Flexible Configuration System**: Each plugin manages its own optional configuration using XML, JSON, INI, or custom DLL based settings.

- **Shared Memory Core**: Lightweight communication layer between plugins and external applications.

- **Shared Blob Data**: Custom data exchange space for advanced or experimental integrations without modifying the core.

- **Hot Plug** Support: Devices can be connected or disconnected at runtime, even during a live game session, without restarting the server or the game.

- **Developer SDK**: Headers, tools, and full source sample plugins to build additional IN and OUT integrations.

### Use Cases

- Forward racing game telemetry to motion platforms, dashboards, or custom hardware.

- Connect multiple external devices to enhance immersion in games.

- Monitor and log real-time data for analysis and performance improvement.

- Develop plugins for custom hardware, protocols, or systems.

### Architecture

- **OpenSR Server**: The core relay handling memory & plugin lifecycle.

- **IN Plugins**: Input sources (games or custom app).

- **Out Plugins**: Output destination (device, network, monitoring, dash, app, etc.).

- **OpenSR Plugins Interface**: Shared headers and SDK for plugin developers.

### Why OpenSR?

Unlike monolithic tools, OpenSR is lightweight, modular, optimized, and extensible. Developers can write plugins with minimal boilerplate, while users benefit from an open ecosystem where telemetry can be routed anywhere.

OpenSR also builds on more than 15 years of experience (since 2008) in bridging games, hardware, and display systems. Users benefit from this expertise as we continue to serve the community with a free, open, and evolving project.

## **General Questions**

**What is Open Sim Relay (OpenSR)?**  
A 100% free and open middleware designed to bridge applications that send data or telemetry (such as games or custom apps) to motion simulators, robotic systems, sim racing hardware, steering wheels, digital dashboards, leaderboard systems, telemetry applications, mobile devices (Android, iOS), IoT systems, USB devices, and more. It captures data from supported games (via *IN plugins*) and relays it to *Out plugins*.

**Is OpenSR free?**  
Yes. OpenSR is free to use (no license or registration needed). The core application is distributed as a closed but freely available binary, while plugins can be open-source or proprietary depending on each developer’s choice. The free version operates at up to 30 Hz update rate, which is enough for normal usage and most hardware setups. Pro Performance Mode (higher update rates up to 500 Hz) is available as a thank-you to donors who help the project. Sign in within the app to access PPM.

**Which games are supported?**  
Any game can be supported by writing a dedicated *Game plugin*. OpenSR will support a wide range of major racing titles, including but not limited to:  
***Studio 397***  
Le Mans Ultimate,  
rFactor Pro**,  
rFactor 2,  
***iRacing***  
iRacing,  
***Kunos Simulazioni***  
Assetto Corsa Pro**,  
Assetto Corsa Rally,  
Assetto Corsa EVO,  
Assetto Corsa Competizione,  
Assetto Corsa,  
***KW Studios (Sector3)***  
RaceRoom Racing Experience,  
***All EA / CodeMasters games***  
*WRC*  
EA SPORTS WRC 2023,  
*F1 Games*  
Codemasters / EA F1 25,  
Codemasters / EA F1 24,  
Codemasters / EA F1 23,  
Codemasters / EA F1 22,  
EA Grid Legends,  
***All CodeMasters games***  
Codemasters F1 2021,  
Codemasters F1 2020,  
Codemasters F1 2019,  
Codemasters DiRT Rally 2.0,  
Codemasters F1 2018,  
Codemasters F1 2017,  
Codemasters DiRT 4,  
Codemasters DiRT Rally,  
Codemasters F1 2016,  
Codemasters F1 2015,  
Codemasters F1 2014,  
Codemasters F1 2013,  
Codemasters F1 2012,  
Codemasters F1 2011,  
Codemasters F1 2010,  
Codemasters GRID Autosport,  
Codemasters GRID2,  
Codemasters DiRT3,  
Codemasters DiRT2,  
Codemasters GRID,  
***ALL REIZA Studios***  
Automobilista 2,  
***SMS Games****  
Project CARS 3,*  
Project CARS 2,  
Project CARS,  
***Others***  
Forza Motorsport 2023,  
Forza Horizon 4,  
Forza Horizon 5,  
Forza Motorsport 7,  
BeamNG Drive,  
KartKraft (Black Delta),  
PiBoSo Kart Racing Pro,  
PiBoSo MX Bikes,  
PiBoSo GP Bikes,  
PiBoSo World Racing Series,  
GRally,  
Live For Speed  
***SCS Software Truck Sim:***  
Euro Truck Simulation 2,  
American Truck Simulator  

*** special version for PRO available on demand* (not free)

**What are the system requirements for OpenSR?**

OpenSR is designed to be extremely lightweight, the **core relay itself typically uses under 1% CPU** and only a few megabytes of memory.  
Actual resource usage may vary depending on the plugins you load, as each plugin manages its own processing independently.

- **Operating System:** Windows 10 or Windows 11 (64-bit)

- **CPU:** Any modern x64 processor

- **Memory:** Minimal (a few MBs for shared buffers and plugins)

- **Storage:** Only a few megabytes required for the core and plugin folders

OpenSR is designed to run quietly in the background without impacting your game performance.

**What can I connect OpenSR to?**  
Almost anything! Dashboards app, leaderboard app, motion simulator platforms, LEDs, displays, data loggers, Arduino/ESP devices, robots, or even hardware with custom network protocols.

## **Functionality & Architecture**

**How does OpenSR work?**  
OpenSR uses a **core relay server** that hosts shared memory structures.

- *Game plugins* push telemetry into the core.

- *Out plugins* read and forward it wherever you want.  
  It’s fast, lightweight, and designed to run quietly in the background.

**What is a “plugin” in OpenSR?**  
A plugin is a dynamic module (DLL) that extends OpenSR.

- *Game Plugins* bring data **in**.

- *Out Plugins* send data **out**.  
  Each lives in its own folder with optional settings and resources.

**How do I configure plugins?**  
Each plugin’s folder can contain:

- settings.xml or settings.dll for configuration

- Or any custom settings format (JSON, INI, etc.)  
  Developers are free to manage settings in their preferred format.

## **Usage & Integration**

**Does OpenSR support running multiple plugins at once?**  
Yes. You can run one *Game plugin* and multiple *Out plugins* simultaneously, for example, sending telemetry both to a dashboard and to a motion platform.

**Can OpenSR run multiple games at once?**  
No. OpenSR is designed to handle one active game at a time, but it can support **any** game, depending on the plugin loaded.

**Does it require admin privileges?**  
Only if a specific plugin does. The core itself runs as a normal user process.

**Can I use OpenSR with custom hardware or drones or robots?**  
Yes. OpenSR isn’t limited to game telemetry, it features **two shared data channels**:

- a **structured telemetry buffer** for game data, and

- a **generic blob buffer** (32 KB) for any custom or non-standard data.

This blob channel lets plugins exchange any kind of information, making OpenSR ideal for **custom hardware, IoT systems, drones, robots, or experimental prototypes**. If your application can produce or consume data, OpenSR can relay it.

## **Development & Customization**

**Can I create my own plugin?**  
Yes. OpenSR provides clean C++ headers and interfaces in OpenSR Plugins Interface/.  
You can build a new plugin using Visual Studio 2015 or later, with minimal dependencies. You can write *Game (IN)* or *Out* plugins; C++ is the recommended/native option (OpenSR exposes the native interfaces IOpenSRPlugin and IOpenSROutPlugin), but you are **not limited** to C++. Plugins can be authored in any language by using a wrapper or interop layer C#, Python, Java, etc. are all possible.

The SDK provides: sample headers and C++ templates, plus example wrappers/templates to help you build plugins in managed or scripted languages. Build the plugin into its folder with optional settings (XML/JSON/INI or settings.dll), drop it into the appropriate game *Plugins/* or *OutPlugins/* directory, and the core will load it.

**Can I use my own configuration format or UI for settings?**  
Absolutely. OpenSR natively manages a settings.xml file associated with each plugin, which allows basic configuration out of the box. However, developers can go further using the SDK to create their **own custom settings UI** (settings.dll) or use any configuration format they prefer such as JSON, INI, or even a database.

This flexibility lets each plugin define its own configuration workflow and user interface while still integrating seamlessly into the OpenSR plugin management system.

## **Community & Contribution**

**Where can I get plugins or share my own?**  
Community plugins will be listed on the project’s GitHub and future OpenSR forum. Developers can easily share precompiled plugins or source code.

**How can I contribute to OpenSR?**

- **Developers** can help by building new plugins or improving existing ones.

- **Enthusiasts and testers** can share feedback, test new versions, and suggest ideas for future features.

- **Supporters** who simply love the project can also donate to help us acquire more hardware, devices, and games for testing, every bit helps make OpenSR more compatible and reliable for everyone.

Every contribution counts, and being part of OpenSR means shaping a tool that connects virtual worlds to the real one.

# OpenSR Early Access

The Early Access version of OpenSR is available now!

The current devices supported by OpenSR Early Access (2026 v0.4.0.4):

**Leo Bodnar** Shift-Lights Interface (SLI)  
USB Shift lights Interface ( SLI-F1, SLI-PRO, SLI-M and SLI-FTEC – Fanatec USB Rim Conversion Kit)

**Thrustmaster**  
TS-PC Racer Ferrari 488 Challenge Edition

**Fanatec rim**  
The software manages all Fanatec hardware supported by the version 4.06 of the official Fanatec SDK.

VDASH-EMU Sim Racing Digital Dash. VDASH-EMU Android/Windows sim racing digital dash. VDASH-EMU For Windows includes the native support of VoCore screens and STREAM DECK.

**SimDisplay** Sim Racing Devices  
SIM RACE LCD, SIM RACE F1, SIM RACE Deluxe or SIM RACE PRO USB Shift lights Display devices from SIM Display

**Steelseries**  
SRW-S1 steering wheel

**VoCore**  
USB Screen 4″, 4.3″, 5″, 6.8″ and Round as backend of VDASH-EMU For Windows

**STREAM DECK** (standalone mode)  
MK2 and XL v1/v2 as backend of VDASH-EMU For Windows

**V2R Controller Based On Adafruit Trinket M0**  
15 RPM LEDs, 6 FLAG LEDs supported as backend of VDASH-EMU For Windows

**AMC Thanos Controller**  
OpenSR now supports the AMC Thanos motion controller as a backend of 3DOF Motion Simulator plugin (still a w.i.p….), enabling direct control of compatible motion platforms with low-latency telemetry and seamless integration through the OpenSR plugin system.

In addition to supporting USB devices and displays, OpenSR also have a native support of VDASH-EMU for Android and Windows.

More devices and apps will be supported. Check the supported devices and applications list on the website for the latest compatibility information.

## Quick Getting Started Guide

OpenSR is designed to be simple and easy to use. Follow these three steps to get started quickly.

### Step 1 – Download and Install

Download the latest full installer and install OpenSR.

### Step 2 – Allow OpenSR Through Windows Firewall

When launching OpenSR for the first time, Windows may display a firewall security prompt.

Click “Allow Access” to let OpenSR communicate with its components and plugins. Some plugins use local network communication to exchange data with dashboard applications and external devices.

### Step 3 – Recommended Initial Configuration

Before going on track with your favorite game, we recommend the following setup:

#### A. Select Your Language

1. Open the Settings page from the left sidebar.

2. Select your preferred language.

3. Save your settings.

#### B. Sign In to Enable Pro Performance Mode (PPM)

1. Open the Sign In page.

2. Enter your account login and password.

To sign in to the OpenSR application, the user must first connect to the support forum at least once. This initial connection is required to validate and activate the account.

By default, OpenSR sends telemetry data at 30 Hz (30 updates per second), which is sufficient for most devices and applications.

Signing in activates Pro Performance Mode (PPM), unlocking telemetry updates at the game’s native rate, typically ranging from 60 Hz to over 500 Hz depending on the simulator.

This feature is our way of rewarding everyone who has supported our projects through donations, contributions, testing, development, and community involvement since 2009.

Thank you all for your continued support. Your contributions help us maintain the project, improve OpenSR, and add support for new hardware and software.

## Additional Notes

- Some game plugins may provide additional settings such as UDP ports, IP addresses, telemetry options, and other game-specific configuration. Refer to the documentation page dedicated to each game plugin for more information.

- Some display and device plugins may include additional settings such as:
  
  - 12-hour or 24-hour time format
  
  - Metric or Imperial units
  
  - km/h or mph speed display
  
  - Device-specific customization options

## You’re Ready to Race

That’s it.

You are now ready to use OpenSR with your favorite racing simulator and enjoy your sim racing setup with maximum compatibility and performance.

## Join the Discussion

We would love to hear your feedback.

Visit the OpenSR forum to:

- Share your thoughts about OpenSR

- Report bugs

- Suggest improvements

- Request new features

- Discuss supported hardware and games

- Help shape the future of the project

Community feedback is extremely valuable during the Early Access period.

## Plugin Developers and Hardware Manufacturers

OpenSR was designed from the beginning to be extensible through plugins.

If you are a developer, hardware manufacturer, dashboard application author, or content creator interested in adding support for your products, contact us to obtain access to the OpenSR SDK.

The SDK is currently under active development, just like the OpenSR Early Access release. It is available on request to registered developers and partners.

The complete public SDK will be released alongside the final OpenSR 1.x release.

## License (Abstract)

### End User License Agreement (EULA)

for **Open Sim Relay (OpenSR)**

Copyright (c) 2025 by Zappadoc and EKSIMRacing Foundation. All rights reserved.

## 1. Definitions

- **“Core Software”** means the Open Sim Relay (OpenSR) core system, including its binaries, executables, libraries, documentation, and all related elements provided by Zappadoc and/or EKSIMRacing Foundation.

- **“Extension Software”** means external modules designed to work with the Core Software, including but not limited to “IN (Game) Plugins” and “OUT Plugins,” whether created by Zappadoc , EKSIMRacing Foundation, or third parties.

- **“You”** or **“End User”** means the individual or entity installing or using the Core Software.

## 2. License Grant

The Core Software is licensed, not sold.  
You are granted a personal, non-exclusive, non-transferable right to install and use the Core Software in binary form only.

This license is granted free of charge for personal and non-commercial use only.  
Commercial use of the Core Software requires a separate written license agreement. Please contact the copyright holder for details.

## 3. Restrictions on the Core Software

Unless expressly authorized in writing by Zappadoc and/or EKSIMRacing Foundation, you may not:

1. Reverse engineer, decompile, or disassemble the Core Software.

2. Modify, adapt, translate, or create derivative works of the Core Software.

3. Redistribute, publish, or share the Core Software binaries.

4. Sell, rent, lease, sublicense, or otherwise commercially exploit the Core Software.

## 4. Extension Software

1. Extension Software may be developed and distributed independently by third parties.

2. Developers of Extension Software retain full ownership and copyright of their work.

3. Extension Software may be released under any license chosen by its author (including open source, proprietary, commercial, or free).

4. The Core Software imposes no licensing restrictions on Extension Software.

5. Extension Software must not interfere with the proper functioning of the Core Software or violate the terms of this Agreement.

## 5. Updates and Termination

1. Zappadoc and/or EKSIMRacing Foundation may, at their discretion, provide updates, patches, or modifications to the Core Software.

2. This license will terminate automatically if you breach any of its terms.

3. Upon termination, you must cease all use of the Core Software and destroy all copies in your possession.

## 6. Ownership

The Core Software, including all associated intellectual property rights, remains the sole property of Zappadoc and EKSIMRacing Foundation.  
No ownership rights are transferred to you under this Agreement.

Extension Software developed by third parties remains the sole property of its respective authors, who may distribute it under their chosen license terms.

## 7. Disclaimer of Warranty

The Core Software is provided “AS IS,” without warranty of any kind, express or implied, including but not limited to warranties of merchantability, fitness for a particular purpose, or non-infringement.

You assume all risks associated with the use of the Core Software.

## 8. Limitation of Liability

In no event shall Zappadoc , EKSIMRacing Foundation, or the author(s) be liable for any damages arising out of or related to the use or inability to use the Core Software, including but not limited to:

- Data loss

- Business interruption

- Loss of profits

- System failure or malfunction

Even if advised of the possibility of such damages.

## 10. Contact

For commercial licensing, permissions, or other inquiries, please contact us.

## Plugin License (Template)

### Plugin Developer License Notice (Template)

```
[Plugin Name] for Open Sim Relay (OpenSR)
Copyright (c) [Year] [Author/Organization]

This plugin is an Extension Software for use with the Open Sim Relay (OpenSR) Core.
The OpenSR Core is proprietary software licensed separately by Zappdoc/ EKSIMRacing Foundation.
This plugin is licensed under the following terms:

[Insert license choice here, e.g. MIT, GPLv3, proprietary license text, or custom terms]

Note: The license for this plugin must remain compatible with the OpenSR End User License Agreement (EULA).
See https://www.eksimracing.org/opensr for details.
```

## Plugin License (Example)

### Example of License for OpenSR Plugins

```
[Plugin Name] for Open Sim Relay (OpenSR)
Copyright (c) [Year] [Author/Organization]

Permission is hereby granted, free of charge, to any person obtaining a copy of this
plugin (the "Extension Software") and associated documentation files, to use, copy,
modify, merge, publish, distribute, sublicense, and/or sell copies of the Extension
Software, subject to the following conditions:

1. The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Extension Software.

2. This license applies only to the Extension Software. The Open Sim Relay (OpenSR)
   Core Software is proprietary software licensed separately by Zappadoc / EKSIMRacing Foundation.
   Nothing in this license grants rights to the OpenSR Core.

3. The Extension Software must remain compatible with the OpenSR End User License
   Agreement (EULA). See https://www.eksimracing.org/opensr for details.

THE EXTENSION SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE EXTENSION SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
EXTENSION SOFTWARE.
```

### Support

Users or developers can check EKSIMRacing forum to get assistance. A quick guide to OpenSR SDK is also available.

### Join the Discussion

We would love to hear your feedback. Visit the OpenSR forum to:

- Share your thoughts about OpenSR

- Report bugs

- Suggest improvements

- Request new features

- Discuss supported hardware and games

- Help shape the future of the project

Community feedback is extremely valuable during the Early Access period.
