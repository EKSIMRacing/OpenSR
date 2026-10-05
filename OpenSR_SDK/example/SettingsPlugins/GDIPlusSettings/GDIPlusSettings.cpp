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

#include "GDIPlusSettings.h"
#include <windowsx.h>
#include <CommCtrl.h>
#include <iostream>

#pragma comment(lib, "Comctl32.lib")

using namespace Gdiplus;

GDIPlusSettings::GDIPlusSettings() {
    // Start GDI+
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, nullptr);
}

GDIPlusSettings::~GDIPlusSettings() {
    // Shutdown handled in Shutdown()
}

BOOL GDIPlusSettings::Initialize(HWND hWindow, OpenSRContext* context, void* buffersOut, const wchar_t* pluginFolderPath, const uint64_t sharedBuffer) {
    m_hWnd = hWindow;
    m_pContext = context;
    if (pluginFolderPath) m_pluginFolderPath = pluginFolderPath;

    std::cout << "GDIPlusSettings: Initialize called with HWND 0x" << std::hex << (uintptr_t)m_hWnd << std::dec << std::endl;

    // Ensure common controls (for edit control)
    INITCOMMONCONTROLSEX icex = {};
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    // Subclass the host window so we can intercept paints/etc.
    SetWindowSubclass(m_hWnd, GDIPlusSettings::SubclassWndProc, 1, (DWORD_PTR)this);

    // Create child controls at **fixed absolute positions** inside the 1024x800 canvas.
    // These coordinates are logical positions in the plugin canvas ? the host will clip/scroll the viewport.
    // NOTE: parent is m_hWnd and coordinates are relative to that parent (0,0 = canvas origin).
    m_hLabel = CreateWindowExW(
        0,
        L"STATIC",
        L"Name:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        50, 50, 150, 20,          // FIXED absolute position inside 1024x800
        m_hWnd,
        nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );
    if (!m_hLabel)
        return FALSE;

    m_hEdit = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL | WS_TABSTOP,
        120, 48, 300, 24,         // FIXED absolute position inside 1024x800 (aligned with label)
        m_hWnd,
        (HMENU)1001,
        GetModuleHandle(nullptr),
        nullptr
    );
    if (!m_hEdit)
        return FALSE;

    m_hEdit2 = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL | WS_TABSTOP,
        120, 78, 300, 24,         // FIXED absolute position inside 1024x800 (aligned with label)
        m_hWnd,
        (HMENU)1002,
        GetModuleHandle(nullptr),
        nullptr
    );
    if (!m_hEdit2)
        return FALSE;

    // Optionally set default text
    SetWindowTextW(m_hEdit, L"Default value...");

    // Return desired panel size (match your original choice).
    return TRUE;
}

BOOL GDIPlusSettings::Shutdown() {
    std::cout << "GDIPlusSettings: Shutdown called." << std::endl;

    // Remove subclass if still present
    if (m_hWnd && IsWindow(m_hWnd)) {
        RemoveWindowSubclass(m_hWnd, GDIPlusSettings::SubclassWndProc, 1);
    }

    // Destroy child windows
    if (m_hLabel && IsWindow(m_hLabel)) DestroyWindow(m_hLabel);
    if (m_hEdit && IsWindow(m_hEdit)) DestroyWindow(m_hEdit);
    if (m_hEdit2 && IsWindow(m_hEdit2)) DestroyWindow(m_hEdit2);

    m_hLabel = nullptr;
    m_hEdit = nullptr;
    m_hEdit2 = nullptr;

    // Shutdown GDI+
    if (m_gdiplusToken) {
        GdiplusShutdown(m_gdiplusToken);
        m_gdiplusToken = 0;
    }

    return FALSE;
}

LRESULT CALLBACK GDIPlusSettings::SubclassWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
    UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    GDIPlusSettings* pThis = (GDIPlusSettings*)dwRefData;

    switch (uMsg) {
    case WM_SIZE: {
        if (pThis) {
            UINT width = LOWORD(lParam);
            UINT height = HIWORD(lParam);
            return 0;
        }
        break;
    }
    case WM_PAINT: {
        if (pThis) {
            pThis->OnPaint();
            return 0;
        }
        break;
    }
    case WM_COMMAND: {
        if (pThis) {
            WORD id = LOWORD(wParam);
            WORD code = HIWORD(wParam);
            if (id == 1001 && code == EN_CHANGE) {
                // Edit text changed
                wchar_t buf[512] = {};
                GetWindowTextW(pThis->m_hEdit, buf, (int)_countof(buf));
                std::wcout << L"GDIPlusSettings: Edit changed -> " << buf << std::endl;
            }
            if (id == 1002 && code == EN_CHANGE) {
                // Edit text changed
                wchar_t buf[512] = {};
                GetWindowTextW(pThis->m_hEdit2, buf, (int)_countof(buf));
                std::wcout << L"GDIPlusSettings: Edit2 changed -> " << buf << std::endl;
            }
        }
        break;
    }
    case WM_NCDESTROY: {
        // Remove subclass when window is destroyed
        RemoveWindowSubclass(hWnd, GDIPlusSettings::SubclassWndProc, 1);
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void GDIPlusSettings::OnPaint() {
    if (!m_hWnd) return;

#ifdef _DEBUG

    if (m_hWnd) {
        RECT windowRect;
        GetWindowRect(m_hWnd, &windowRect); // Gets SCREEN coordinates
        std::cout << "[Plugin DEBUG] OnPaint: My screen position is L:" << windowRect.left
            << " T:" << windowRect.top
            << " R:" << windowRect.right
            << " B:" << windowRect.bottom << std::endl;
    }
    //  END DIAGNOSTIC
#endif
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(m_hWnd, &ps);
    if (!hdc) return;

    RECT windowRect;
    GetWindowRect(m_hWnd, &windowRect); 

    // Wrap HDC with GDI+ Graphics and draw using fixed coordinates inside 1024x800 canvas.
    Graphics graphics(hdc);

    // High-quality hints
    graphics.SetSmoothingMode(SmoothingModeHighQuality);
    graphics.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

    // IMPORTANT: draw with respect to the fixed canvas origin (0,0) ? do not translate by client offset.
    // The host/parent is responsible for clipping / scrolling the visible portion.
    RectF canvasRect(0.0f, 0.0f, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top);

    // Background (canvas)
    SolidBrush bgBrush(Color(255, 07, 79, 79)); // dark slate-ish
    graphics.FillRectangle(&bgBrush, canvasRect);

    // Example rectangle at fixed position (absolute coordinates)
    RectF boxRect(20.0f, 150.0f, 120.0f, 100.0f); // x=20, y=150 inside the 1024x800
    SolidBrush boxBrush(Color(255, 100, 149, 237)); // cornflower-like
    graphics.FillRectangle(&boxBrush, boxRect);

    // Another visual element placed absolutely
    Pen outlinePen(Color(255, 200, 200, 200), 2.0f);
    graphics.DrawRectangle(&outlinePen, RectF(450.0f, 120.0f, 300.0f, 180.0f));

    // Text drawn at fixed coordinates
    FontFamily ff(L"Segoe UI");
    Font font(&ff, 14.0f, FontStyleRegular, UnitPixel);
    StringFormat sf;
    sf.SetAlignment(StringAlignmentNear);
    sf.SetLineAlignment(StringAlignmentNear);

    SolidBrush textBrush(Color(255, 230, 230, 230));
    RectF textRect(10.0f, 10.0f, 600.0f, 80.0f);
    std::wstring message = L"GDI+ Settings Plugin (absolute 1024x800 canvas)\nElements are placed in fixed coordinates.";
    graphics.DrawString(message.c_str(), (INT)message.length(), &font, textRect, &sf, &textBrush);

    // Draw label near the edit control (optional duplicate, as the STATIC child exists)
    // Use same absolute coordinates as the STATIC + EDIT created in Initialize()
    RectF labelRect(50.0f, 50.0f, 60.0f, 20.0f);
    std::wstring labelText = L"Name:";
    graphics.DrawString(labelText.c_str(), (INT)labelText.length(), &font, labelRect, &sf, &textBrush);

    EndPaint(m_hWnd, &ps);
}

//  Exported C Functions

// This function is called by the host application to create an instance of our settings class.
extern "C" __declspec(dllexport) IPluginSettings * CreatePluginSettings() {
    // We simply create a 'new' instance of our implementation class.
    return new GDIPlusSettings();
}

// This function is called by the host application to destroy the instance.
extern "C" __declspec(dllexport) void DestroyPluginSettings(IPluginSettings * pSettings) {
    // We cast it back to the concrete type and 'delete' it.
    if (pSettings) {
        delete static_cast<GDIPlusSettings*>(pSettings);
    }
}
