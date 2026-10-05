/*
#####################################################################
# Part of Open Sim Relay (OpenSR) - Source Code
# Copyright (c) 2025-2026 Zappadoc - All Rights Reserved
#
# This file is part of the OpenSR Plugins SDK.
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
# - This code, modules, and libraries are subject to change
#   without notice.
# - By using this code, you agree to the terms and conditions
#   of the OpenSR license provided with the software.
#
# Change History:
#   2025-01-07 : Initial creation by Zappadoc
#   2025-10-01 : Integrated into OpenSR project
#####################################################################
*/

// MonitorUIPlugin.cpp
#include <windows.h>
#include "MonitorUIPlugin.h"
#include <windowsx.h>
#include <shlwapi.h>
#include <string>
#include <vector>
#include <chrono>
#include <iostream>
#include <CommCtrl.h>
#include "tinyxml2.h"

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "shlwapi.lib")

using namespace tinyxml2;


// Exports

extern "C" __declspec(dllexport) IPluginUI * CreatePluginUI()
{
    return new MonitorUIPlugin();
}

extern "C" __declspec(dllexport) void DestroyPluginUI(IPluginUI * pUI)
{
    if (pUI) delete static_cast<MonitorUIPlugin*>(pUI);
}

// Implementation

MonitorUIPlugin::MonitorUIPlugin()
    : m_pContext(nullptr), m_pBuffersOut(nullptr), m_hWnd(nullptr)
{
    // Initialize GDI+
    GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplusToken_, &gdiplusStartupInput, NULL);
}

MonitorUIPlugin::~MonitorUIPlugin()
{
    // Ensure thread is stopped and cleaned
    Shutdown();

    // Shutdown GDI+
    Gdiplus::GdiplusShutdown(gdiplusToken_);
}

BOOL MonitorUIPlugin::Initialize(HWND hWindow, OpenSRContext* context, void* outBuf, const wchar_t* pluginPath, const uint64_t sharedbuffer)
{
    m_hWnd = hWindow;
    m_pContext = context;
    m_pBuffersOut = static_cast<OpenSRBuffersOUT*>(outBuf);
    
    if (m_pContext) {
        // create Map folder inside plugin directory, 
        // reminder a global Map folder exists already in parent directory /OpenSR/Map/

        // get the path
        m_pluginFolderPath = StringUtils::createWString(context->osrDocFolder, L"\\", pluginPath);
        m_mapPath = StringUtils::createWString(m_pluginFolderPath, "\\Map");
        // create if not exist
        _wmkdir(m_mapPath.c_str());

        // settings stuff
        m_pluginPath = pluginPath;
        std::wstring spath = StringUtils::createWString(m_pContext->osrDocFolder, L"\\", m_pluginPath, L"\\settings.xml");
        LoadPluginSettings(spath, m_pluginSettings);
    
        // Create controls inside provided host HWND
        CreateControls();

        // Populate initial values (if any)
        PopulateInitialState();

        // Subclass host window to intercept WM_PAINT, WM_COMMAND, WM_SIZE, etc.
        // Use 'this' as refData so subclass proc can access instance.
        SetWindowSubclass(m_hWnd, MonitorUIPlugin::SubclassWndProc, 1, (DWORD_PTR)this);

        // Start the render/update thread
        m_stopRequested = false;
        m_isRunning = true;
        renderThread_ = std::thread(&MonitorUIPlugin::RenderThreadFunc, this);
    }
    return TRUE;
}

bool MonitorUIPlugin::LoadPluginSettings(const std::wstring& filepath, PluginSettings& s) {
    tinyxml2::XMLDocument doc;

    FILE* file = _wfsopen(filepath.c_str(), L"rb", _SH_DENYNO);
    if (!file) {
        std::wcerr << L"Failed to open file: " << filepath << std::endl;
        return false;
    }

    tinyxml2::XMLError error = doc.LoadFile(file);
    fclose(file);
    if (error != XML_SUCCESS)
        return false; // File missing or error, defaults remain

    XMLElement* root = doc.FirstChildElement("settings");
    if (!root) return false;

    for (XMLElement* opt = root->FirstChildElement("option"); opt; opt = opt->NextSiblingElement("option")) {
        const char* name = opt->Attribute("name");
        const char* value = opt->GetText();
        if (!name || !value) continue;

        if (strcmp(name, "plugin update delay (ms)") == 0) {
            int idd = atoi(value);
            if (idd >= 10 && idd <= 1000) {
                s.deviceRate = idd;
            }
            else
                s.deviceRate = 200;
        }
    }
    return true;
}

// The implementation of Shutdown must restore the original Window Procedure for the HWND provided by the host.
// Failure to un subclass the window before this function returns will result in host application instability and crashes.
//
BOOL MonitorUIPlugin::Shutdown()
{
    // Stop thread
    if (m_isRunning) {
        m_stopRequested = true;
        {
            std::lock_guard<std::mutex> lock(m_pauseMutex);
            m_isPaused = false;
        }
        m_pauseCv.notify_all();
        m_sleepCv.notify_all();

        if (renderThread_.joinable()) renderThread_.join();
        m_isRunning = false;
    }

    // Remove subclass
    if (m_hWnd) {
        RemoveWindowSubclass(m_hWnd, MonitorUIPlugin::SubclassWndProc, 1);
    }

    // Return TRUE if dumper or other state indicates changes (here follow original dumper flag)
    return dumper.m_dumpReady;
}

// Create child controls similar to OpenSRMonitorPlugin's CreateWindow controls
void MonitorUIPlugin::CreateControls()
{
    if (!m_hWnd) return;

    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(m_hWnd, GWLP_HINSTANCE);

    // Using layout similar to the original monitor plugin but placed inside provided HWND
    const int buttonWidth = 80;
    const int buttonHeight = 28;

    const int buttonMargin = 2;

    m_hDumpButton = CreateWindowExW(0, L"BUTTON", L"Dump Lap",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        25, 5, buttonWidth - buttonMargin, buttonHeight - buttonMargin, m_hWnd,
        (HMENU)IDOK, hInstance, NULL);

    m_hSelectDumpButton = CreateWindowExW(0, L"BUTTON", L"Select Dump Lap",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        250, 5, buttonWidth + 45 - buttonMargin, buttonHeight - buttonMargin, m_hWnd,
        (HMENU)IDCANCEL, hInstance, NULL);

    if (m_hDumpButton) {
        SetWindowSubclass(m_hDumpButton, MonitorUIPlugin::ButtonSubclassWndProc, 2, (DWORD_PTR)this);
    }
    if (m_hSelectDumpButton) {
        SetWindowSubclass(m_hSelectDumpButton, MonitorUIPlugin::ButtonSubclassWndProc, 3, (DWORD_PTR)this);
    }
    // Note: Further UI (if you want) could be added, but original monitor only had these two buttons plus painted content.
    // Force initial paint
    InvalidateRect(m_hWnd, NULL, TRUE);
}

// Populate initial UI state if needed
void MonitorUIPlugin::PopulateInitialState()
{
    if (m_hDumpButton) {
        if (dumper.m_dumpReady) {
            SetWindowText(m_hDumpButton, L"Stop");
        }
        else {
            SetWindowText(m_hDumpButton, L"Dump Lap");
        }
    }
}

// Dump handlers (copied/adapted from OpenSRMonitorPlugin)
void MonitorUIPlugin::OnSelectDump(HWND /*ctrl*/)
{
    if (m_pContext) {
        OPENFILENAME ofn;
        TCHAR szFile[260] = { 0 };

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = m_hWnd;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = L".lap Files\0*.lap\0All Files\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrInitialDir = m_mapPath.c_str();
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        if (GetOpenFileName(&ofn) == TRUE) {
            std::wstring fullPath(ofn.lpstrFile);
            size_t lastSlash = fullPath.find_last_of(L"\\/");
            std::wstring pathOnly = fullPath.substr(0, lastSlash);
            std::wstring filename = L"\\" + fullPath.substr(lastSlash + 1);

            if (filename.size() < 4 || filename.substr(filename.size() - 4) != L".lap") {
                std::wcout << L"Selected file is not a .lap file." << std::endl;
                return;
            }

            m_trackRenderer.m_fullPath = fullPath;
            m_trackRenderer.init(pathOnly.c_str(), filename.c_str());
        }
    }
}

void MonitorUIPlugin::OnDump(HWND ctrl)
{
    if (dumper.m_dumpReady) {
        dumper.Stop();
    }
    else if (m_pContext && !dumper.m_dumpReady) {
        OPENFILENAME ofn;
        TCHAR szFile[260] = { 0 };

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = m_hWnd;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = L".lap Files\0*.lap\0All Files\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrInitialDir = m_mapPath.c_str();
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

        if (GetSaveFileName(&ofn) == TRUE) {
            std::wstring fullPath(ofn.lpstrFile);
            size_t lastSlash = fullPath.find_last_of(L"\\/");
            std::wstring pathOnly = fullPath.substr(0, lastSlash);
            std::wstring filename = L"\\" + fullPath.substr(lastSlash + 1);
            if (filename.size() < 4 || filename.substr(filename.size() - 4) != L".lap") {
                filename += L".lap";
                fullPath = pathOnly + L"\\" + filename;
            }
            dumper.Init(pathOnly.c_str(), filename.c_str());
        }
        else {
            return;
        }
    }

    // Toggle button text
    if (ctrl) {
        if (dumper.m_dumpReady) {
            SetWindowText(ctrl, L"Stop");
        }
        else {
            SetWindowText(ctrl, L"Dump Lap");
        }
        InvalidateRect(ctrl, NULL, FALSE);
    }
}
template<typename... Args>
std::wstring wfmt(const wchar_t* fmt, Args... args)
{
    wchar_t buf[256];
    swprintf_s(buf, fmt, args...);   // safe formatting into buffer
    return std::wstring(buf);        // return it as std::wstring
}

void MonitorUIPlugin::RenderTelemetry(HDC hdc)
{
    if (!m_pContext || !m_pBuffersOut) return;

    RECT clientRect;
    GetClientRect(m_hWnd, &clientRect);
    int width = clientRect.right;
    int height = clientRect.bottom;

    // 1. Double Buffering & Save State
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
    HGDIOBJ oldBitmap = SelectObject(memDC, memBitmap);
    int savedDC = SaveDC(memDC);

    // 2. High-Speed Fill
    // PatBlt is often faster than FillRect for solid colors
    BitBlt(memDC, 0, 0, width, height, NULL, 0, 0, WHITENESS);

    OutSimData* outSim = m_pBuffersOut->outSimDataOUT;
    if (!outSim || !outSim->mPacketHeader.reportAvailable) {
        DrawText(memDC, L"Telemetry Not Available", -1, &clientRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
    else {
        SelectObject(memDC, GetStockObject(DEFAULT_GUI_FONT));
        SetBkMode(memDC, TRANSPARENT); // Vital for performance with TextOut

        TEXTMETRIC tm;
        GetTextMetrics(memDC, &tm);
        const int lineHeight = tm.tmHeight + 2;
        const int margin = 10;
        const int colWidth = (width - (margin * 2)) / ((width > 1000) ? 3 : (width > 600 ? 2 : 1));
        const int labelValueGap = 135;

        int count = 0;
        wchar_t vB[128]; // Value Buffer
        wchar_t lB[128]; // Label Buffer

        // Ultra-fast Drawing Lambda using ExtTextOut
        auto drawRow = [&](const wchar_t* lbl, const wchar_t* val) {
            int col = count / (110 / ((width > 1000) ? 3 : (width > 600 ? 2 : 1))); // Approximate rows
            // Simplified positioning logic for speed
            int x = margin + (count % ((width > 1000) ? 3 : (width > 600 ? 2 : 1))) * colWidth;
            int y = margin + 40 + (count / ((width > 1000) ? 3 : (width > 600 ? 2 : 1))) * lineHeight;

            SetTextColor(memDC, RGB(60, 60, 60));
            ExtTextOutW(memDC, x, y, 0, NULL, lbl, (UINT)wcslen(lbl), NULL);

            SetTextColor(memDC, RGB(0, 0, 0));
            ExtTextOutW(memDC, x + labelValueGap, y, 0, NULL, val, (UINT)wcslen(val), NULL);
            count++;
        };

        // --- PACKET HEADER ---
        swprintf_s(vB, L"%d", outSim->mPacketHeader.paused); drawRow(L"Paused", vB);
        swprintf_s(vB, L"%d", outSim->mPacketHeader.reportAvailable); drawRow(L"Report Available", vB);
        swprintf_s(vB, L"%d", outSim->mPacketHeader.frameId); drawRow(L"Frame Id", vB);
        swprintf_s(vB, L"%d", outSim->mPacketHeader.playerSlotIndex); drawRow(L"Player Slot", vB);

        // --- PLAYER DATA ---
        int slot = outSim->mPacketHeader.playerSlotIndex;
        Point3D car_pos = { 0,0,0 };
        if (slot > -1) {
            auto& p = outSim->mPlayers.player[slot];
            car_pos = { p.worldPositionX, p.worldPositionY, p.worldPositionZ };
            wchar_t wD[64]; MultiByteToWideChar(CP_UTF8, 0, p.driverName, -1, wD, 64);
            drawRow(L"Driver", wD);
            swprintf_s(vB, L"%.3f s", p.currentLapTime); drawRow(L"Lap Time", vB);
            swprintf_s(vB, L"%.2f", p.bestLapTime); drawRow(L"Best Lap Time", vB);
            swprintf_s(vB, L"%.2f m", p.lapDist); drawRow(L"Lap Dist", vB);
            swprintf_s(vB, L"%d", p.currentLap); drawRow(L"Lap", vB);
            swprintf_s(vB, L"P%d", p.position); drawRow(L"Position", vB);
            swprintf_s(vB, L"%d", p.sector); drawRow(L"Sector", vB);
            swprintf_s(vB, L"%.2f", car_pos.x); drawRow(L"World Pos X", vB);
            swprintf_s(vB, L"%.2f", car_pos.y); drawRow(L"World Pos Y", vB);
            swprintf_s(vB, L"%.2f", car_pos.z); drawRow(L"World Pos Z", vB);
        }

        // --- VEHICLE DATA ---
        auto& v = outSim->mVehicleData;
        swprintf_s(vB, L"%.1f km/h", v.speed); drawRow(L"Speed", vB);
        swprintf_s(vB, L"%d", v.gear); drawRow(L"Gear", vB);
        swprintf_s(vB, L"%.0f", v.rpm); drawRow(L"RPM", vB);
        swprintf_s(vB, L"%.0f", v.maxRpm); drawRow(L"Max RPM", vB);
        swprintf_s(vB, L"%.0f %%", v.throttle); drawRow(L"Throttle", vB);
        swprintf_s(vB, L"%.0f %%", v.brake); drawRow(L"Brake", vB);
        swprintf_s(vB, L"%.0f %%", v.clutch); drawRow(L"Clutch", vB);
        swprintf_s(vB, L"%.0f %%", v.handbrake); drawRow(L"Handbrake", vB);
        swprintf_s(vB, L"%.1f°", v.steer); drawRow(L"Steer Angle", vB);
        swprintf_s(vB, L"%.1f %%", v.brakeBias); drawRow(L"Brake Bias", vB);
        swprintf_s(vB, L"%.1f °C", v.engineTemp); drawRow(L"Engine Temp", vB);
        swprintf_s(vB, L"%.1f °C", v.oilTemp); drawRow(L"Oil Temp", vB);
        swprintf_s(vB, L"%.1f bar", v.oilPress); drawRow(L"Oil Pressure", vB);
        swprintf_s(vB, L"%.1f °C", v.waterTemp); drawRow(L"Water Temp", vB);
        swprintf_s(vB, L"%.1f bar", v.waterLevel); drawRow(L"Water Level", vB);
        swprintf_s(vB, L"%.1f bar", v.fuelPressure); drawRow(L"Fuel Pressure", vB);
        swprintf_s(vB, L"%.1f bar", v.boost); drawRow(L"Boost", vB);
        swprintf_s(vB, L"%.1f L", v.fuelLevel); drawRow(L"Fuel Level", vB);
        swprintf_s(vB, L"%.1f L", v.fuelCapacity); drawRow(L"Fuel Capacity", vB);
        swprintf_s(vB, L"%.1f L/h", v.fuelUsePerHour); drawRow(L"Fuel Use/Hour", vB);

        // --- WHEELS / BRAKES / SUSPENSION ---
        const wchar_t* wPos[4] = { L"FL", L"FR", L"RL", L"RR" };
        for (int i = 0; i < 4; i++) {
            auto& wh = outSim->mWheelData.wheels[i];
            swprintf_s(lB, L"Tire Temp %s", wPos[i]); swprintf_s(vB, L"%.1f °C", wh.tyreTempCore); drawRow(lB, vB);
            swprintf_s(lB, L"Tire Pres %s", wPos[i]); swprintf_s(vB, L"%.1f psi", wh.tyrePressure); drawRow(lB, vB);
            swprintf_s(lB, L"Tire Wear %s", wPos[i]); swprintf_s(vB, L"%.2f %%", wh.tyreWearOverall); drawRow(lB, vB);
            swprintf_s(lB, L"Tire Comp %s", wPos[i]); swprintf_s(vB, L"%d", wh.tyreVisualCompound); drawRow(lB, vB);
            swprintf_s(lB, L"Tire Dirt %s", wPos[i]); swprintf_s(vB, L"%.1f %%", wh.tyreDirtyLevel); drawRow(lB, vB);
            swprintf_s(lB, L"Slip Ang %s", wPos[i]); swprintf_s(vB, L"%.1f°", wh.wheelSlipAngle); drawRow(lB, vB);
            swprintf_s(lB, L"Wheel Spd %s", wPos[i]); swprintf_s(vB, L"%.2f m/s", wh.wheelSpeed); drawRow(lB, vB);
            swprintf_s(lB, L"Tire Grip %s", wPos[i]); swprintf_s(vB, L"%.2f m", wh.tyreGrip); drawRow(lB, vB);
            swprintf_s(lB, L"Surface %s", wPos[i]); swprintf_s(vB, L"%d", wh.wheelSurfaceId); drawRow(lB, vB);

            swprintf_s(lB, L"Brake Temp %s", wPos[i]); swprintf_s(vB, L"%.1f °C", v.brakes[i].brakeDiscTemp); drawRow(lB, vB);
            swprintf_s(lB, L"Brake Wear %s", wPos[i]); swprintf_s(vB, L"%.1f %%", v.brakes[i].brakePadWear); drawRow(lB, vB);

            swprintf_s(lB, L"Susp Defl %s", wPos[i]); swprintf_s(vB, L"%.2f m", v.suspensions[i].suspDeflection); drawRow(lB, vB);
            swprintf_s(lB, L"Susp Vel %s", wPos[i]); swprintf_s(vB, L"%.2f m/s", v.suspensions[i].suspVelocity); drawRow(lB, vB);
        }

        // --- ENVIRONMENT ---
        auto& e = outSim->mSessionData.environment;
        swprintf_s(vB, L"%.1f °C", e.trackTemp); drawRow(L"Track Temp", vB);
        swprintf_s(vB, L"%.1f °C", e.airTemp); drawRow(L"Air Temp", vB);
        swprintf_s(vB, L"%.0f %%", e.humidity); drawRow(L"Humidity", vB);
        swprintf_s(vB, L"%.1f km/h", e.windSpeed); drawRow(L"Wind Speed", vB);
        swprintf_s(vB, L"%.0f°", e.windDirection); drawRow(L"Wind Dir", vB);
        swprintf_s(vB, L"%.1f %%", e.rainLevel); drawRow(L"Rain Density", vB);

        // --- MOTION ---
        auto& m = outSim->mMotionData;
        swprintf_s(vB, L"%.2f", m.carCGLocX); drawRow(L"CarGCLoc X", vB);
        swprintf_s(vB, L"%.2f", m.carCGLocY); drawRow(L"CarGCLoc Y", vB);
        swprintf_s(vB, L"%.2f", m.carCGLocZ); drawRow(L"CarGCLoc Z", vB);
        swprintf_s(vB, L"%.2f°", m.pitch); drawRow(L"Pitch", vB);
        swprintf_s(vB, L"%.2f°", m.roll); drawRow(L"Roll", vB);
        swprintf_s(vB, L"%.2f°", m.yaw); drawRow(L"Yaw", vB);
        swprintf_s(vB, L"%.2f m/s²", m.localAccelX); drawRow(L"Accel X", vB);
        swprintf_s(vB, L"%.2f m/s²", m.localAccelY); drawRow(L"Accel Y", vB);
        swprintf_s(vB, L"%.2f m/s²", m.localAccelZ); drawRow(L"Accel Z", vB);

        if (dumper.m_dumpReady && slot > -1) {
            dumper.dumpPoint(outSim->mPlayers.player[slot].currentLapTime * 1000, v.speed, (int)v.gear, v.rpm, car_pos);
        }

        RECT mapRect = { 10, 410, 652, 820 };
        m_trackRenderer.Draw(memDC, mapRect, car_pos);
    }

    // 3. Blit and cleanup
    BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);
    RestoreDC(memDC, savedDC);
    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}


// thread that invalidates the host window at ~30 FPS, respecting pause
void MonitorUIPlugin::RenderThreadFunc()
{
    bool firstPass = true;
    OutSimData* outSim = m_pBuffersOut->outSimDataOUT;
    int ms = 200;
    while (!m_stopRequested) {
        {
            std::unique_lock<std::mutex> lock(m_pauseMutex);
            m_pauseCv.wait(lock, [this]() { return !m_isPaused || m_stopRequested; });
            if (m_stopRequested) break;
        }
        if (!outSim || !outSim->mPacketHeader.reportAvailable || outSim->mPacketHeader.paused)
            ms = m_pluginSettings.deviceRate * 12;
        else
            ms = m_pluginSettings.deviceRate;

        // Ask host window to repaint
        if (m_hWnd) {
            RECT rc;
            GetClientRect(m_hWnd, &rc);

            // exclude top area where buttons live (adjust if layout changes)
            if (firstPass) {
                firstPass = false;
                rc.top = 0;
            } else
                rc.top = 52;

            InvalidateRect(m_hWnd, &rc, FALSE);
        }

        // Sleep to target ~30 FPS
      //  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        {
            std::unique_lock<std::mutex> lock(m_sleepMutex);
            m_sleepCv.wait_for(lock, std::chrono::milliseconds(ms), [this]() {
                return m_stopRequested.load();
                });
        }

        // If host app stopped running (if context communicates), we can exit early
        if (m_pContext && !m_pContext->isAppRunning) break;
    }
}

LRESULT CALLBACK MonitorUIPlugin::ButtonSubclassWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    MonitorUIPlugin* pThis = reinterpret_cast<MonitorUIPlugin*>(dwRefData);
    if (!pThis) {
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

    // Determine which button this is and get a pointer to its hover flag
    bool* pHoverFlag = nullptr;
    if (hWnd == pThis->m_hDumpButton) {
        pHoverFlag = &pThis->m_isDumpButtonHovered;
    }
    else if (hWnd == pThis->m_hSelectDumpButton) {
        pHoverFlag = &pThis->m_isSelectDumpButtonHovered;
    }
    else {
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

    switch (uMsg)
    {
    case WM_MOUSEMOVE:
        // When the mouse moves over the button, if we are not already
        // tracking, set the hover flag and request a WM_MOUSELEAVE message.
        if (!(*pHoverFlag))
        {
            *pHoverFlag = true;
            InvalidateRect(hWnd, NULL, FALSE); // Force a repaint to show hover state

            TRACKMOUSEEVENT tme;
            tme.cbSize = sizeof(TRACKMOUSEEVENT);
            tme.dwFlags = TME_LEAVE; // We only need to know when the mouse leaves
            tme.hwndTrack = hWnd;
            TrackMouseEvent(&tme);
        }
        break;

    case WM_MOUSELEAVE:
        // The mouse has left the button. Reset the hover flag and force
        // a repaint to return to the normal state.
        *pHoverFlag = false;
        InvalidateRect(hWnd, NULL, FALSE);
        return 0; // Handled
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
// Window subclass to intercept messages
LRESULT CALLBACK MonitorUIPlugin::SubclassWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
    UINT_PTR /*uIdSubclass*/, DWORD_PTR dwRefData)
{
    MonitorUIPlugin* pThis = reinterpret_cast<MonitorUIPlugin*>(dwRefData);
    if (!pThis) return DefSubclassProc(hWnd, uMsg, wParam, lParam);

    switch (uMsg) {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // Double-buffered painting
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        int width = clientRect.right - clientRect.left;
        int height = clientRect.bottom - clientRect.top;

        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hbmBack = CreateCompatibleBitmap(hdc, std::max<>(1, width), std::max<>(1, height));
        HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmBack);

        // call render
        pThis->RenderTelemetry(hdcMem);

        // blit
        BitBlt(hdc, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);

        // cleanup
        SelectObject(hdcMem, hbmOld);
        DeleteObject(hbmBack);
        DeleteDC(hdcMem);

        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_SETCURSOR:
    {
        // Ensure the normal arrow cursor when the mouse is inside this window
        if (LOWORD(lParam) == HTCLIENT)
        {
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return TRUE; // handled
        }
        break;
    }
    case WM_COMMAND:
    {
        const int id = LOWORD(wParam);
        const int code = HIWORD(wParam);
        HWND hCtl = (HWND)lParam;

        if (id == IDOK && code == BN_CLICKED) {
            // Dump button pressed
            pThis->OnDump(hCtl);
        }
        else if (id == IDCANCEL && code == BN_CLICKED) {
            // Select dump
            pThis->OnSelectDump(hCtl);
        }
        return 0;
    }
    case WM_DESTROY:
        // host might destroy; ensure thread stops
        pThis->m_stopRequested = true;
        pThis->m_pauseCv.notify_all();
        return 0; // DefSubclassProc(hWnd, uMsg, wParam, lParam);
    case WM_ERASEBKGND:
        // we'll handle background in WM_PAINT
        return 1;
    case WM_DRAWITEM:
    {
        LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
        if (dis->hwndItem == pThis->m_hDumpButton || dis->hwndItem == pThis->m_hSelectDumpButton)
        {
            Graphics g(dis->hDC);
            // No anti-aliasing needed for sharp rectangles, but we keep it for the text.
            g.SetTextRenderingHint(TextRenderingHintAntiAlias);

            // Get the full client rect of the button
            RectF rc((REAL)dis->rcItem.left, (REAL)dis->rcItem.top,
                (REAL)(dis->rcItem.right - dis->rcItem.left),
                (REAL)(dis->rcItem.bottom - dis->rcItem.top));

            // 1. Determine button state and colors
            bool pressed = (dis->itemState & ODS_SELECTED) != 0;
            bool focused = (dis->itemState & ODS_FOCUS) != 0;
            bool hot = (dis->hwndItem == pThis->m_hDumpButton)
                ? pThis->m_isDumpButtonHovered
                : pThis->m_isSelectDumpButtonHovered;

            Color bgColor = pressed ? Color(204, 232, 255) // Pressed blue
                : (hot ? Color(229, 243, 255)   // Hover blue
                    : Color(250, 250, 250)); // Normal off-white


            // Use a slightly darker blue for the border when focused, otherwise a standard gray.
            Color borderColor = focused ? Color(0, 120, 215) : Color(173, 173, 173);

            SolidBrush bgBrush(bgColor);
            SolidBrush borderBrush(borderColor);
            const int borderWidth = 1;

            // 2. Draw the border by filling the entire rectangle with the border color.
            g.FillRectangle(&borderBrush, rc);

            // 3. Draw the background by filling an inner rectangle.
            //    This leaves a 1px "frame" which acts as our border.
            RectF innerRc(rc.X + borderWidth, rc.Y + borderWidth,
                rc.Width - (borderWidth * 2), rc.Height - (borderWidth * 2));
            g.FillRectangle(&bgBrush, innerRc);



            // 4. Draw the caption (text) centered in the button
            WCHAR text[128];
            GetWindowText(dis->hwndItem, text, 128);
            FontFamily ff(L"Segoe UI");
            Font font(&ff, 14.0f, FontStyleRegular, UnitPixel);
            StringFormat sf;
            sf.SetAlignment(StringAlignmentCenter);
            sf.SetLineAlignment(StringAlignmentCenter);
            SolidBrush txtBrush(Color(30, 30, 30));

            // Draw string in the original full rectangle for perfect centering
            g.DrawString(text, -1, &font, rc, &sf, &txtBrush);

            return TRUE;
        }
        break;
    }
    default:
        break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

