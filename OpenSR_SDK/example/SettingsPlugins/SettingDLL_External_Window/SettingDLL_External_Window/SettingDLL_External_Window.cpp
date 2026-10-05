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

// SettingDLL_External_Window.cpp
#include "SettingDLL_External_Window.h"
#include "StringUtils.h"
#include <windows.h>
#include<iostream>
#include <string>
#include <thread>
#include "resource.h"

#include "tinyxml2.h"
#include <CommCtrl.h>

#pragma comment(lib, "Comctl32.lib")
using namespace tinyxml2;

void SettingsDLLExternalDialog::LoadSettings() {
    // Defaults
    strcpy_s(m_settings.outgauge_ip, "127.0.0.1");
    m_settings.outgauge_port = 4441;
    strcpy_s(m_settings.motionsim_ip, "127.0.0.1");
    m_settings.motionsim_port = 4444;

    tinyxml2::XMLDocument doc;
    FILE* file = _wfsopen(m_xmlPath.c_str(), L"rb", _SH_DENYNO);
    if (!file) {
        std::wcerr << L"Failed to open file: " << m_xmlPath << std::endl;
        return;
    }

    tinyxml2::XMLError error = doc.LoadFile(file);
    fclose(file);
    if (error == XML_SUCCESS) {
        XMLElement* root = doc.FirstChildElement("settings");
        if (root) {
            for (XMLElement* opt = root->FirstChildElement("option"); opt; opt = opt->NextSiblingElement("option")) {
                const char* name = opt->Attribute("name");
                const char* value = opt->GetText();
                if (!name || !value) continue;

                if (strcmp(name, "outgauge ip") == 0)
                    strncpy_s(m_settings.outgauge_ip, value, _TRUNCATE);
                else if (strcmp(name, "outgauge port") == 0)
                    m_settings.outgauge_port = atoi(value);
                else if (strcmp(name, "motionsim ip") == 0)
                    strncpy_s(m_settings.motionsim_ip, value, _TRUNCATE);
                else if (strcmp(name, "motionsim port") == 0)
                    m_settings.motionsim_port = atoi(value);
            }
        }
    }
}


void SettingsDLLExternalDialog::SaveSettings(HWND hwndDlg) {
    GetDlgItemTextA(hwndDlg, IDC_OUTGAUGE_IP, m_settings.outgauge_ip, sizeof(m_settings.outgauge_ip));
    char buf[32];
    std::memset(buf, 0, sizeof(buf));
    GetDlgItemTextA(hwndDlg, IDC_OUTGAUGE_PORT, buf, sizeof(buf));
    m_settings.outgauge_port = atoi(buf);

    GetDlgItemTextA(hwndDlg, IDC_MOTIONSIM_IP, m_settings.motionsim_ip, sizeof(m_settings.motionsim_ip));

    std::memset(buf, 0, sizeof(buf));
    GetDlgItemTextA(hwndDlg, IDC_MOTIONSIM_PORT, buf, sizeof(buf));
    m_settings.motionsim_port = atoi(buf);

    m_settings.updateNeeded = true;

    tinyxml2::XMLDocument doc;
    XMLElement* root = doc.NewElement("settings");
    doc.InsertFirstChild(root);

    auto addOption = [&](const char* name, const char* value) {
        XMLElement* opt = doc.NewElement("option");
        opt->SetAttribute("name", name);
        opt->SetText( value);
        root->InsertEndChild(opt);
    };

    std::memset(buf, 0, sizeof(buf));
    addOption("outgauge ip", m_settings.outgauge_ip);
    sprintf_s(buf, "%d", m_settings.outgauge_port);
    addOption("outgauge port", buf);
    addOption("motionsim ip", m_settings.motionsim_ip);
    sprintf_s(buf, "%d", m_settings.motionsim_port);
    addOption("motionsim port", buf);

    //  Save using FILE* to handle wstring path correctly
    FILE* fp = _wfsopen(m_xmlPath.c_str(), L"wb", _SH_DENYNO);
    if (!fp) {
        std::wcerr << L"Failed to save settings file: " << m_xmlPath << std::endl; 
        return;
    }

    doc.SaveFile(fp);
    fclose(fp);
}

// paint dialog background
void SettingsDLLExternalDialog::OnPaintDlg(HWND hWnd) {
    // We are using standard Win32 controls, which paint themselves.
    // We only need to paint our background.
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    // Fill background with the standard dialog color to match the controls
    FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_BTNFACE + 1));
    EndPaint(hWnd, &ps);
}

// paint child window background
void SettingsDLLExternalDialog::OnPaint(HWND hWnd) {
    // We are using standard Win32 controls, which paint themselves.
    // We only need to paint our background.
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    // Fill background with the standard dialog color to match the controls
    FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_BACKGROUND + 1));
    EndPaint(hWnd, &ps);
}

LRESULT CALLBACK SettingsDLLExternalDialog::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    SettingsDLLExternalDialog* pThis = nullptr;
    if (uMsg == WM_INITDIALOG) {
        pThis = (SettingsDLLExternalDialog*)(lParam);
        SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)pThis);
    }
    else {
        pThis = (SettingsDLLExternalDialog*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
        //if (!pThis && uMsg != WM_GETFONT) // Ignore WM_GETFONT before init
        //    return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        if (pThis) pThis->InitDialogValues(hwndDlg);
        return TRUE;
    case WM_PAINT: {
        if (pThis) pThis->OnPaintDlg(hwndDlg);
        return 0;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            if (pThis) pThis->SaveSettings(hwndDlg);
            // end dialog
            EndDialog(hwndDlg, IDOK);
            return TRUE;
        case IDCANCEL:
            pThis->m_settings.updateNeeded = false;
            EndDialog(hwndDlg, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// init control values with loaded settings
void SettingsDLLExternalDialog::InitDialogValues(HWND hwndDlg) {
    // center dialog
    CenterDialogInParent(hwndDlg);

    LoadSettings();

    // init values
    SetDlgItemTextA(hwndDlg, IDC_OUTGAUGE_IP, m_settings.outgauge_ip);
    char buf[16];
    sprintf_s(buf, "%d", m_settings.outgauge_port);
    SetDlgItemTextA(hwndDlg, IDC_OUTGAUGE_PORT, buf);

    SetDlgItemTextA(hwndDlg, IDC_MOTIONSIM_IP, m_settings.motionsim_ip);
    sprintf_s(buf, "%d", m_settings.motionsim_port);
    SetDlgItemTextA(hwndDlg, IDC_MOTIONSIM_PORT, buf);
}

// center dialog
void SettingsDLLExternalDialog::CenterDialogInParent(HWND hDlg )
{
    if (!m_hWnd)
        return;

    RECT rcParent, rcDlg;
    GetWindowRect(m_hWnd, &rcParent);
    GetWindowRect(hDlg, &rcDlg);

    int x = rcParent.left + (rcParent.right - rcParent.left - (rcDlg.right - rcDlg.left)) / 2;
    int y = rcParent.top + (rcParent.bottom - rcParent.top - (rcDlg.bottom - rcDlg.top)) / 2;

    SetWindowPos(hDlg, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

//  The Subclass Window Procedure
LRESULT CALLBACK SettingsDLLExternalDialog::SubclassWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    SettingsDLLExternalDialog* pThis = (SettingsDLLExternalDialog*)dwRefData;
    if (!pThis) return DefSubclassProc(hWnd, uMsg, wParam, lParam);

    switch (uMsg) {
    case WM_USER + 11: {
        INT_PTR ret = DialogBoxParam(
            pThis->g_hInstance,
            MAKEINTRESOURCE(IDD_DIALOG1),
            NULL,
            DialogProc,
            (LPARAM)pThis
        );
   
        break; 
    }
    case WM_PAINT:
        pThis->OnPaint(hWnd);
        return 0;
    case WM_NCDESTROY:
        RemoveWindowSubclass(hWnd, SettingsDLLExternalDialog::SubclassWndProc, 1);
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

BOOL SettingsDLLExternalDialog::Initialize(HWND hWindow, OpenSRContext* context, void* buffersOut, const wchar_t* pluginFolderPath, const uint64_t sharedBuffer)
{
    if (context) {
        m_hWnd = hWindow;
        m_pContext = context;
        m_xmlPath = StringUtils::createWString(pluginFolderPath, L"\\settings.xml");

        // Subclass the window provided by the host so we can handle its messages.
        SetWindowSubclass(m_hWnd, SettingsDLLExternalDialog::SubclassWndProc, 1, (DWORD_PTR)this);        
        InvalidateRect(m_hWnd, NULL, FALSE);
        UpdateWindow(m_hWnd);                // Force synchronous paint
        
        // Use the DLL's HINSTANCE
        HMODULE hModule = NULL;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)DialogProc, &hModule);
        g_hInstance = hModule;

        // post custom msg to open dialog, give time to paint child window bacground
        PostMessage(m_hWnd, WM_USER + 11, 0, 0); 
    
        return TRUE;
    }
    return FALSE;
}

BOOL SettingsDLLExternalDialog::Shutdown()
{
    // absolutely needed even if doubled with WM_NCDESTROY
    RemoveWindowSubclass(m_hWnd, SettingsDLLExternalDialog::SubclassWndProc, 1);
    return m_settings.updateNeeded;
}
    
