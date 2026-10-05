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

// OpenSRMonitorPlugin.cpp
#include "OpenSRMonitorPlugin.h"
#include <iostream>
#include <string>

#ifdef EXTERNAL_TELEM_WINDOW
#include <vector>
#include <utility>
#include <thread>
#include <chrono>
#include <shlwapi.h>
#include "tinyxml2.h"

#pragma comment(lib, "shlwapi.lib")

using namespace tinyxml2;
#endif


// Mandatory export symbols
extern "C" __declspec(dllexport) osr::IOpenSRPlugin * CreatePlugin() {
    return new OpenSRMonitorPlugin();
}

extern "C" __declspec(dllexport) void DestroyPlugin(osr::IOpenSRPlugin * plugin) {
    if (plugin)
        delete plugin;
}

OpenSRMonitorPlugin::OpenSRMonitorPlugin() : m_pContext(nullptr), m_hWnd(nullptr) {
#ifdef EXTERNAL_TELEM_WINDOW
    // Initialize GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplusToken_, &gdiplusStartupInput, NULL);
#endif
 }

OpenSRMonitorPlugin::~OpenSRMonitorPlugin() {
    Stop();
#ifdef EXTERNAL_TELEM_WINDOW

    // Shutdown GDI+
    Gdiplus::GdiplusShutdown(gdiplusToken_);
#endif
}

bool OpenSRMonitorPlugin::Init(OpenSRContext* context, void* outBuf, const wchar_t* pluginPath) {
    m_pContext = context;
    m_pBuffersOut = static_cast<OpenSRBuffersOUT*>(outBuf);
    m_pluginPath = pluginPath;
    if (context) {

        std::wstring path = StringUtils::createWString(context->osrDocFolder, L"\\", pluginPath);
        m_mapPath = StringUtils::createWString(path, "\\Map");
    
        _wmkdir(m_mapPath.c_str());
    
    }

    return true;
}

bool OpenSRMonitorPlugin::Start() {
    if (!m_pContext)
        return false;
    m_stopRequested = false;
    m_isPluginRunning = true;
#ifdef EXTERNAL_TELEM_WINDOW
    // show / hide external window at startup
    std::wstring spath = StringUtils::createWString(m_pContext->osrDocFolder, L"\\", m_pluginPath, L"\\settings.xml");
    if (LoadPluginSettings(spath, m_pluginSettings))
        m_showExternalW = m_pluginSettings.showExternalMonitorWindow;

    windowThread_ = std::thread(&OpenSRMonitorPlugin::WindowThreadFunc, this);
#endif
    return true;
}

void OpenSRMonitorPlugin::Stop() {
#ifdef EXTERNAL_TELEM_WINDOW    
    m_stopRequested = true;
    m_isPluginRunning = false;

    if (windowThread_.joinable()) {
        windowThread_.join();
    }

    m_isPluginRunning = false;
#endif
}

void OpenSRMonitorPlugin::Shutdown() {
  
}

/*
* callback: event from host app after editing a game configuration or editing plugin settings or cliking 'Check Device' button
* to test the ProfilePathChanged callback add a profile_template.xml from any other out plugin in the MonitorPlugin folder
*/
void OpenSRMonitorPlugin::OnContextChanged(osr::OpenSRContextChange reason) {
    if (m_pContext && reason == osr::OpenSRContextChange::ProfilePathChanged) {
        std::wstring profilePath = m_pContext->currentProfilePath;
        std::wcout << "[MONITOR Plugin] Profile need to be reloaded.\nProfile Path: " << profilePath << std::endl;
        // use GameProfileHelper class to parse configuration
        if (m_profile.LoadProfile(profilePath))
        {
            // success, setup device with m_config values 
            auto& cfg = m_profile.GetConfig();

            if (cfg.shiftLights.allowed)
            {
                // ...
            }
        }
    }
    else if (reason == osr::OpenSRContextChange::CheckDevice) {
        // user ask to check device, not used here
        // CheckDevice();
    }
#ifdef EXTERNAL_TELEM_WINDOW
    // only relevant if external window is available
    else if (reason == osr::OpenSRContextChange::SettingsChanged) {
        std::wcout << "[MONITOR PLUGIN] Settings need to be reloaded.\nSettings Path: ? \n";
        if (m_pContext && !m_pluginPath.empty()) {
            std::wstring spath = StringUtils::createWString(m_pContext->osrDocFolder, L"\\", m_pluginPath, L"\\settings.xml");
            if (LoadPluginSettings(spath, m_pluginSettings)) {
                m_showExternalW = m_pluginSettings.showExternalMonitorWindow;
                if (m_hWnd) {
                    ShowWindow(m_hWnd, (m_showExternalW ? SW_RESTORE : SW_MINIMIZE));
                }
            }
        }
    }
#endif
}

#ifdef EXTERNAL_TELEM_WINDOW
bool OpenSRMonitorPlugin::LoadPluginSettings(const std::wstring& filepath, PluginSettings& s) {
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

        if (strcmp(name, "Show External Monitor Window At Startup") == 0) {
            if ((strcmp(value, "true") == 0))
                s.showExternalMonitorWindow = true;
            else
                s.showExternalMonitorWindow = false;
        }
        else if (strcmp(name, "device update delay (ms)") == 0) {
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

void OpenSRMonitorPlugin::OnSelectDump(HWND ctrl) {
    if (m_pContext) {
        OPENFILENAME ofn;
        TCHAR szFile[260] = { 0 };

        // Initialize OPENFILENAME for opening a file
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = L".lap Files\0*.lap\0All Files\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrInitialDir = m_mapPath.c_str();
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        // Open the file dialog
        if (GetOpenFileName(&ofn) == TRUE) {
            std::wstring fullPath(ofn.lpstrFile);

            // Extract path and filename
            size_t lastSlash = fullPath.find_last_of(L"\\/");
            std::wstring pathOnly = fullPath.substr(0, lastSlash);
            std::wstring filename = L"\\" + fullPath.substr(lastSlash + 1);

            // Ensure the filename ends with .lap
            if (filename.size() < 4 || filename.substr(filename.size() - 4) != L".lap") {
                std::wcout << L"Selected file is not a .lap file." << std::endl;
                return;
            }

            std::wcout << L"Path only: " << pathOnly << std::endl;
            std::wcout << L"Filename: " << filename << std::endl;

            m_trackRenderer.m_fullPath = fullPath;
           // m_trackRenderer.SetAngleView(180);
            m_trackRenderer.init(pathOnly.c_str(), filename.c_str());
        }
    }
}

void OpenSRMonitorPlugin::OnDump(HWND ctrl) {
    if (dumper.m_dumpReady) {
        dumper.Stop();
    }
    else if (m_pContext && !dumper.m_dumpReady) {
        // init dumper
        OPENFILENAME ofn;
        TCHAR szFile[260] = { 0 };

        // Initialize OPENFILENAME
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = L".lap Files\0*.lap\0All Files\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrInitialDir = m_mapPath.c_str();
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

        // Open the save dialog
        if (GetSaveFileName(&ofn) == TRUE) {
            std::wstring fullPath(ofn.lpstrFile);
            // Extract path without filename
            size_t lastSlash = fullPath.find_last_of(L"\\/");
            std::wstring pathOnly = fullPath.substr(0, lastSlash);
            std::wstring filename = L"\\" + fullPath.substr(lastSlash + 1);
            if (filename.size() < 4 || filename.substr(filename.size() - 4) != L".lap") {
                filename += L".lap";
                fullPath = pathOnly + L"\\" + filename;
            }
            // Use selectedFilePath as needed
            std::wcout << L"Selected file path: " << fullPath << std::endl;
           // 
            dumper.Init(pathOnly.c_str(), filename.c_str());

        }
        else {
            return;
        }

     }
    // set toggle name
    if (ctrl) {
        if (dumper.m_dumpReady) {
            // dump lap set name to stop
            SetWindowText(ctrl, L"Stop");
        }
        else {
            SetWindowText(ctrl, L"Dump Lap");
        }
        InvalidateRect(ctrl, NULL, FALSE);
    }
 
}

void OpenSRMonitorPlugin::WindowThreadFunc() {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    // get instance of the DLL
    HMODULE hModule = NULL;
    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, L"Init", &hModule);
    g_hInstance = (HINSTANCE)hModule;
    wc.hInstance = g_hInstance;
 
    // generate an unique lpszClassName
    WCHAR szClassName[256];
    swprintf_s(szClassName, L"OpenSRMonitorDeviceWindow%08x", (UINT)g_hInstance);
    wc.lpszClassName = szClassName;
    // register
    RegisterClass(&wc);

    m_hWnd = CreateWindowEx(0, szClassName, L"OpenSR Monitor Plugin",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 668, 840,
        NULL, NULL, g_hInstance, this);

    if (m_hWnd)
    {
        //  Save Button (Larger and better positioned)
        const int buttonWidth = 80;
        const int buttonHeight = 28;
        m_hDumpButton = CreateWindowExW(0, L"BUTTON", L"Dump Lap", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            25, 5, buttonWidth, buttonHeight, m_hWnd, (HMENU)IDOK, g_hInstance, NULL); // Use ID 1 for dump

        m_hSelectDumpButton = CreateWindowExW(0, L"BUTTON", L"Select Dump Lap", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            250, 5, buttonWidth+45, buttonHeight, m_hWnd, (HMENU)IDCANCEL, g_hInstance, NULL); // Use ID 0 for select

        ShowWindow(m_hWnd, (m_showExternalW ? SW_SHOW : SW_MINIMIZE));

        MSG msg = {};
        m_isPluginRunning = true;
        while (m_isPluginRunning && !m_stopRequested && GetMessage(&msg, NULL, 0, 0)) {
            // Pause Handling
            {
                std::unique_lock<std::mutex> lock(m_pauseMutex);
                // Wait while m_isPaused is true. This efficiently blocks the thread
                // without using CPU cycles, until Resume() is called.
                m_pauseCv.wait(lock, [this] { return !m_isPluginPaused; });
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);

           if (m_pContext && !m_pContext->isAppRunning)
            break;
        }
        // unsure timer is stopped
        KillTimer(m_hWnd, 1);

        // the destroy window is safe here more then in WM_DESTROY event which can be skipped for whatever reason
        DestroyWindow(m_hWnd);

        // unregister
        UnregisterClass(szClassName, GetModuleHandle(NULL));

        m_isPluginRunning = false;
        std::wcout << "[MONITOR PLUGIN] Thread down\n";
     }
}

LRESULT CALLBACK OpenSRMonitorPlugin::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    OpenSRMonitorPlugin* plugin = nullptr; // reinterpret_cast<OpenSRMonitorPlugin*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (uMsg == WM_NCCREATE) {
        plugin = (OpenSRMonitorPlugin*)((CREATESTRUCT*)lParam)->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)plugin);

    }
    else {
        plugin = (OpenSRMonitorPlugin*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    switch (uMsg) {
    case WM_CREATE:
    {
        SetTimer(hwnd, 1, 33, NULL); // ~30 FPS for smooth monitoring
        return 0;
    }
    case WM_COMMAND:
    {
        const int id = LOWORD(wParam);
        const int code = HIWORD(wParam);
        HWND hCtl = (HWND)lParam;

        if (id == IDOK && code == BN_CLICKED) {
            plugin->OnDump(hCtl);
        }
        if (id == IDCANCEL && code == BN_CLICKED) {
            plugin->OnSelectDump(hCtl);
        }
        return 0;
    }
    case WM_LBUTTONDOWN:
        plugin->m_trackRenderer.OnMouseDown(LOWORD(lParam), HIWORD(lParam), false);
        SetCapture(hwnd);
        break;
    case WM_RBUTTONDOWN:
        plugin->m_trackRenderer.OnMouseDown(LOWORD(lParam), HIWORD(lParam), true);
        SetCapture(hwnd);
        break;
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
        plugin->m_trackRenderer.OnMouseUp(LOWORD(lParam), HIWORD(lParam), uMsg == WM_RBUTTONUP);
        ReleaseCapture();
        break;
    case WM_MOUSEMOVE:
        plugin->m_trackRenderer.OnMouseMove(LOWORD(lParam), HIWORD(lParam));
        break;
    case WM_MOUSEWHEEL:
        plugin->m_trackRenderer.OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
        InvalidateRect(hwnd, nullptr, FALSE);
        break;
    case WM_TIMER:
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Double Buffering Start

        // 1. Get the size of the client area.
        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        int width = clientRect.right - clientRect.left;
        int height = clientRect.bottom - clientRect.top;

        // 2. Create an in-memory DC compatible with the screen.
        HDC hdcMem = CreateCompatibleDC(hdc);

        // 3. Create a bitmap to use as the back buffer.
        HBITMAP hbmBack = CreateCompatibleBitmap(hdc, width, height);

        // 4. Select the bitmap into the memory DC. Keep the old one to restore later.
        HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmBack);

        // All drawing now happens on hdcMem

        if (plugin && plugin->m_pluginSettings.showExternalMonitorWindow && IsWindowVisible(hwnd) && !IsIconic(hwnd)) {
            // Pass the MEMORY DC to the rendering function, not the screen DC.
            plugin->RenderTelemetry(hdcMem);
        }

        // Drawing is complete, now copy the back buffer to the screen

        // 5. Blit (copy) the entire back buffer (hdcMem) to the screen (hdc) in one go.
        BitBlt(hdc, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);


        // Cleanup: Important to release GDI resources

        // 6. Restore the original bitmap.
        SelectObject(hdcMem, hbmOld);

        // 7. Delete the back buffer bitmap and the memory DC.
        DeleteObject(hbmBack);
        DeleteDC(hdcMem);

        // Double Buffering End

        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_SYSCOMMAND:
        // The bottom 4 bits of the command ID are used internally by Windows,
        // so we must mask them off using 0xFFF0 to check the command correctly.
        if ((wParam & 0xFFF0) == SC_CLOSE)
        {
            // The user clicked 'X' or pressed Alt+F4
            ShowWindow(hwnd, SW_MINIMIZE);
            return 0; // Consuming this message prevents the window from closing
        }
        // IMPORTANT: You must break here to let DefWindowProc handle 
        // other system commands (like Drag/Move, Minimize, Maximize)
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        KillTimer(hwnd, 1);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

template<typename... Args>
std::wstring wfmt(const wchar_t* fmt, Args... args)
{
    wchar_t buf[256];
    swprintf_s(buf, fmt, args...);   // safe formatting into buffer
    return std::wstring(buf);        // return it as std::wstring
}

void OpenSRMonitorPlugin::RenderTelemetry(HDC hdc)
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
        //auto drawRow = [&](const wchar_t* lbl, const wchar_t* val) {
        //    int col = count / (110 / ((width > 1000) ? 3 : (width > 600 ? 2 : 1))); // Approximate rows
        //    // Simplified positioning logic for speed
        //    int x = margin + (count % ((width > 1000) ? 3 : (width > 600 ? 2 : 1))) * colWidth;
        //    int y = margin + 40 + (count / ((width > 1000) ? 3 : (width > 600 ? 2 : 1))) * lineHeight;

        //    SetTextColor(memDC, RGB(60, 60, 60));
        //    ExtTextOutW(memDC, x, y, 0, NULL, lbl, (UINT)wcslen(lbl), NULL);

        //    SetTextColor(memDC, RGB(0, 0, 0));
        //    ExtTextOutW(memDC, x + labelValueGap, y, 0, NULL, val, (UINT)wcslen(val), NULL);
        //    count++;
        //};

        auto drawRow = [&](const wchar_t* lbl, const wchar_t* val, bool forceNewLine = false) {
            int maxCols = (width > 1000) ? 3 : (width > 600 ? 2 : 1);

            if (forceNewLine && (count % maxCols != 0)) {
                count += maxCols - (count % maxCols);
            }

            int x = margin + (count % maxCols) * colWidth;
            int y = margin + 40 + (count / maxCols) * lineHeight;

            SetTextColor(memDC, RGB(60, 60, 60));
            ExtTextOutW(memDC, x, y, 0, NULL, lbl, (UINT)wcslen(lbl), NULL);

            SetTextColor(memDC, RGB(0, 0, 0));
            ExtTextOutW(memDC, x + labelValueGap, y, 0, NULL, val, (UINT)wcslen(val), NULL);
            count++;
        };

        // PACKET HEADER
        swprintf_s(vB, L"%d", outSim->mPacketHeader.paused); drawRow(L"Paused", vB);
        swprintf_s(vB, L"%d", outSim->mPacketHeader.reportAvailable); drawRow(L"Report Available", vB);
        swprintf_s(vB, L"%d", outSim->mPacketHeader.frameId); drawRow(L"Frame Id", vB);
        swprintf_s(vB, L"%d", outSim->mPacketHeader.playerSlotIndex); drawRow(L"Player Slot", vB);

        // PLAYER DATA
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

        // VEHICLE DATA
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

        // WHEELS / BRAKES / SUSPENSION
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

        // ENVIRONMENT
        auto& e = outSim->mSessionData.environment;
        swprintf_s(vB, L"%.1f °C", e.trackTemp); drawRow(L"Track Temp", vB);
        swprintf_s(vB, L"%.1f °C", e.airTemp); drawRow(L"Air Temp", vB);
        swprintf_s(vB, L"%.0f %%", e.humidity); drawRow(L"Humidity", vB);
        swprintf_s(vB, L"%.1f km/h", e.windSpeed); drawRow(L"Wind Speed", vB);
        swprintf_s(vB, L"%.0f°", e.windDirection); drawRow(L"Wind Dir", vB);
        swprintf_s(vB, L"%.1f %%", e.rainLevel); drawRow(L"Rain Density", vB);

        // MOTION
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
        swprintf_s(vB, L"");
        drawRow(L"Map Help: Mouse Scroll to Zoom | Mouse Left Drag to Pan | Mouse Right Drag to Rotate.", vB, true);

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
#endif // EXTERNAL_TELEM_WINDOW
