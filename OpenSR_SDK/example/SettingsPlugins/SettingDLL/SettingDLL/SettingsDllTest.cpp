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
// SettingsDLLTest.cpp
#include "SettingsDLLTest.h"
#include "StringUtils.h"
#include <windows.h>
#include <windowsx.h> // For GET_WM_COMMAND_ID
#include<iostream>
#include <string>
#include "resource.h"
#include "tinyxml2.h"
#include <CommCtrl.h>

#pragma comment(lib, "Comctl32.lib")

using namespace tinyxml2;


//  Exported C Functions

// This function is called by the host application to create an instance of our settings class.
extern "C" __declspec(dllexport) IPluginSettings * CreatePluginSettings() {
    // We simply create a 'new' instance of our implementation class.
    return new SettingsDLLTest();
}

// This function is called by the host application to destroy the instance.
extern "C" __declspec(dllexport) void DestroyPluginSettings(IPluginSettings * pSettings) {
    // We cast it back to the concrete type and 'delete' it.
    if (pSettings) {
        delete static_cast<SettingsDLLTest*>(pSettings);
    }
}

void SettingsDLLTest::LoadSettings() {
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


void SettingsDLLTest::SaveSettings() {
    tinyxml2::XMLDocument doc;
    XMLElement* root = doc.NewElement("settings");
    doc.InsertFirstChild(root);

    auto addOption = [&](const char* name, const char* value) {
        XMLElement* opt = doc.NewElement("option");
        opt->SetAttribute("name", name);
        opt->SetText( value);
        root->InsertEndChild(opt);
    };

    char buf[32];
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

// BUILDING UI
// The New Subclass Window Procedure
LRESULT CALLBACK SettingsDLLTest::SubclassWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    SettingsDLLTest* pThis = (SettingsDLLTest*)dwRefData;
    if (!pThis) return DefSubclassProc(hWnd, uMsg, wParam, lParam);

    switch (uMsg) {
    //case WM_GETDLGCODE: {
    //    //    // Keep any existing flags the base class might want,
    //    //    // then add the “want arrows” flag.
    //       UINT uFlags = DLGC_WANTCHARS | DLGC_WANTARROWS;
    //      return uFlags;
    //    }
    case WM_PAINT:
        pThis->OnPaint();
        return 0; // We handle painting completely.
    case WM_COMMAND:
        pThis->OnCommand(wParam, lParam);
        return 0;
    case WM_NCDESTROY:
        RemoveWindowSubclass(hWnd, SettingsDLLTest::SubclassWndProc, 1);
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void SettingsDLLTest::CreateControls() {
    // We must have a handle to our own DLL instance to create controls.
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(m_hWnd, GWLP_HINSTANCE);

    // Layout Metrics
    // Define a simple grid and standard sizes for a clean, consistent layout.
    const int margin = 15;
    const int labelWidth = 50;
    const int editWidth = 100;
    const int portWidth = 50;
    const int controlHeight = 22; // Taller controls are easier to use
    const int rowSpacing = 28;    // Space between rows
    const int groupWidth = 280;
    const int groupHeight = 85;

    const int col1_x = margin;
    const int col2_x = margin + groupWidth + margin;
   
    //  Set a modern UI font for all child controls
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    LOGFONT lf;
    GetObject(hFont, sizeof(LOGFONT), &lf);
    // Use a slightly larger, more readable font size
    lf.lfHeight = 18;
    // For a modern look, "Segoe UI" is the standard.
    wcscpy_s(lf.lfFaceName, L"Segoe UI");
    HFONT hNewFont = CreateFontIndirect(&lf);

    //  OutGauge Group
    HWND hGroupOutGauge = CreateWindowExW(0, L"BUTTON", L"OutGauge", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        col1_x, margin, groupWidth, groupHeight,
        m_hWnd, NULL, hInstance, NULL);

    // OutGauge Labels
    HWND hLabelOutIp = CreateWindowExW(0, L"STATIC", L"IP:", WS_CHILD | WS_VISIBLE | SS_RIGHT,
        col1_x + 10, margin + 25, labelWidth, controlHeight,
        m_hWnd, NULL, hInstance, NULL);
    HWND hLabelOutPort = CreateWindowExW(0, L"STATIC", L"Port:", WS_CHILD | WS_VISIBLE | SS_RIGHT,
        col1_x + 10, margin + 25 + rowSpacing, labelWidth, controlHeight,
        m_hWnd, NULL, hInstance, NULL);

    // OutGauge Edit Boxes
    m_hOutGaugeIp = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        col1_x + 10 + labelWidth + 5, margin + 25, editWidth, controlHeight,
        m_hWnd, (HMENU)IDC_OUTGAUGE_IP, hInstance, NULL);
    m_hOutGaugePort = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | ES_NUMBER,
        col1_x + 10 + labelWidth + 5, margin + 25 + rowSpacing, portWidth, controlHeight,
        m_hWnd, (HMENU)IDC_OUTGAUGE_PORT, hInstance, NULL);

    //  MotionSim Group
    HWND hGroupMotionSim = CreateWindowExW(0, L"BUTTON", L"MotionSim", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        col2_x, margin, groupWidth, groupHeight,
        m_hWnd, NULL, hInstance, NULL);

    // MotionSim Labels
    HWND hLabelMotionIp = CreateWindowExW(0, L"STATIC", L"IP:", WS_CHILD | WS_VISIBLE | SS_RIGHT,
        col2_x + 10, margin + 25, labelWidth, controlHeight,
        m_hWnd, NULL, hInstance, NULL);
    HWND hLabelMotionPort = CreateWindowExW(0, L"STATIC", L"Port:", WS_CHILD | WS_VISIBLE | SS_RIGHT,
        col2_x + 10, margin + 25 + rowSpacing, labelWidth, controlHeight,
        m_hWnd, NULL, hInstance, NULL);

    // MotionSim Edit Boxes
    m_hMotionSimIp = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        col2_x + 10 + labelWidth + 5, margin + 25, editWidth, controlHeight,
        m_hWnd, (HMENU)IDC_MOTIONSIM_IP, hInstance, NULL);
    m_hMotionSimPort = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | ES_NUMBER,
        col2_x + 10 + labelWidth + 5, margin + 25 + rowSpacing, portWidth, controlHeight,
        m_hWnd, (HMENU)IDC_MOTIONSIM_PORT, hInstance, NULL);

    // ------------------
    // Others

    hEditSingle = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"default text",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        110, 110, 200, 24, m_hWnd, (HMENU)IDC_EDIT_SINGLE, nullptr, nullptr);

    hEditMulti = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"multi-line",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
        110, 140, 200, 60, m_hWnd, (HMENU)IDC_EDIT_MULTI, nullptr, nullptr);

    hList = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", nullptr,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | LBS_NOTIFY,
        110, 210, 200, 80, m_hWnd, (HMENU)IDC_LISTBOX, nullptr, nullptr);
    SendMessageW(hList, LB_ADDSTRING, 0, (LPARAM)L"Apple");
    SendMessageW(hList, LB_ADDSTRING, 0, (LPARAM)L"Banana");
    SendMessageW(hList, LB_ADDSTRING, 0, (LPARAM)L"Cherry");

    hCombo = CreateWindowExW(0, L"COMBOBOX", nullptr,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
        320, 110, 150, 200, m_hWnd, (HMENU)IDC_COMBO, nullptr, nullptr);
    SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"Option A");
    SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"Option B");
    SendMessageW(hCombo, CB_SETCURSEL, 0, 0);

    hCheck = CreateWindowW(L"BUTTON", L"Enable feature",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        320, 150, 150, 24, m_hWnd, (HMENU)IDC_CHECK, nullptr, nullptr);

    hRadio1 = CreateWindowW(L"BUTTON", L"Choice 1",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON | WS_GROUP,
        320, 180, 150, 24, m_hWnd, (HMENU)IDC_RADIO1, nullptr, nullptr);
    hRadio2 = CreateWindowW(L"BUTTON", L"Choice 2",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON,
        320, 210, 150, 24, m_hWnd, (HMENU)IDC_RADIO2, nullptr, nullptr);
    SendMessageW(hRadio1, BM_SETCHECK, BST_CHECKED, 0);

    //  Save Button
    const int buttonWidth = 80;
    const int buttonHeight = 28;
    m_hSaveButton = CreateWindowExW(0, L"BUTTON", L"Save", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        252, 285, buttonWidth, buttonHeight, m_hWnd, (HMENU)IDOK, hInstance, NULL); // Use ID 1 for save

    //  Apply Font to all created controls
    SendMessage(hGroupOutGauge, WM_SETFONT, (WPARAM)hNewFont, TRUE);
    SendMessage(hLabelOutIp, WM_SETFONT, (WPARAM)hNewFont, TRUE);
    SendMessage(hLabelOutPort, WM_SETFONT, (WPARAM)hNewFont, TRUE);
    SendMessage(m_hOutGaugeIp, WM_SETFONT, (WPARAM)hNewFont, TRUE);
    SendMessage(m_hOutGaugePort, WM_SETFONT, (WPARAM)hNewFont, TRUE);
    SendMessage(hGroupMotionSim, WM_SETFONT, (WPARAM)hNewFont, TRUE);
    SendMessage(hLabelMotionIp, WM_SETFONT, (WPARAM)hNewFont, TRUE);
    SendMessage(hLabelMotionPort, WM_SETFONT, (WPARAM)hNewFont, TRUE);
    SendMessage(m_hMotionSimIp, WM_SETFONT, (WPARAM)hNewFont, TRUE);
    SendMessage(m_hMotionSimPort, WM_SETFONT, (WPARAM)hNewFont, TRUE);
    SendMessage(m_hSaveButton, WM_SETFONT, (WPARAM)hNewFont, TRUE);
}

void SettingsDLLTest::PopulateFromSettings() {
    //  Populate the controls with loaded data
    SetWindowTextA(m_hOutGaugeIp, m_settings.outgauge_ip);
    SetWindowTextA(m_hOutGaugePort, std::to_string(m_settings.outgauge_port).c_str());
    SetWindowTextA(m_hMotionSimIp, m_settings.motionsim_ip);
    SetWindowTextA(m_hMotionSimPort, std::to_string(m_settings.motionsim_port).c_str());
}

static std::wstring GetTextW(HWND hCtrl) {
    int len = GetWindowTextLengthW(hCtrl);
    size_t ln = len + 1;
    std::wstring s;
    s.resize(ln);
    GetWindowTextW(hCtrl, &s[0], ln);
    s.resize(len);
    return s;
}

void SettingsDLLTest::OnSaveControls(HWND hWnd) {
    std::wstring result;

    // Edit controls
    result += L"Single line edit: " + GetTextW(hEditSingle) + L"\n";
    result += L"Multi line edit: " + GetTextW(hEditMulti) + L"\n";

    // Listbox (can be multi-select)
    int sel = (int)SendMessageW(hList, LB_GETCURSEL, 0, 0);
    if (sel != LB_ERR) {
        wchar_t buf[128];
        SendMessageW(hList, LB_GETTEXT, sel, (LPARAM)buf);
        result += L"Listbox: " + std::wstring(buf) + L"\n";
    }

    // Combobox
    int selc = (int)SendMessageW(hCombo, CB_GETCURSEL, 0, 0);
    if (selc != CB_ERR) {
        wchar_t buf[128];
        memset(buf, 0, sizeof(buf));
        SendMessageW(hCombo, CB_GETLBTEXT, selc, (LPARAM)buf);
        result += L"Combobox: " + std::wstring(buf) + L"\n";
    }

    // Checkbox
    LRESULT chk = SendMessageW(hCheck, BM_GETCHECK, 0, 0);
    result += L"Checkbox: ";
    result += (chk == BST_CHECKED) ? L"Checked\n" : L"Unchecked\n";

    // Radio buttons
    if (SendMessageW(hRadio1, BM_GETCHECK, 0, 0) == BST_CHECKED)
        result += L"Radio: Option 1\n";
    else if (SendMessageW(hRadio2, BM_GETCHECK, 0, 0) == BST_CHECKED)
        result += L"Radio: Option 2\n";

    MessageBoxW(hWnd, result.c_str(), L"Collected Values", MB_OK);
}

void SettingsDLLTest::OnSave() {
    // Ensure current edit commits IME/composition text (optional but safe)
    HWND hFocus = GetFocus();
    if (hFocus && (hFocus == m_hOutGaugeIp || hFocus == m_hOutGaugePort ||
        hFocus == m_hMotionSimIp || hFocus == m_hMotionSimPort)) {
        // Move focus away to force EN_KILLFOCUS
        SetFocus(m_hSaveButton);
    }

    // Read live values from the *actual* controls we created
    auto ip1W = GetTextW(m_hOutGaugeIp);
    auto port1W = GetTextW(m_hOutGaugePort);
    auto ip2W = GetTextW(m_hMotionSimIp);
    auto port2W = GetTextW(m_hMotionSimPort);

    // Convert & store
    StringUtils::assignAndBuild(m_settings.outgauge_ip, ip1W);
    m_settings.outgauge_port = _wtoi(port1W.c_str());

    StringUtils::assignAndBuild(m_settings.motionsim_ip, ip2W);
    m_settings.motionsim_port = _wtoi(port2W.c_str());

    OnSaveControls(m_hWnd);

    SaveSettings();
    m_settings.updateNeeded = true;
    MessageBox(m_hWnd, L"Settings Saved!", L"Success", MB_OK);
}


// This handles button clicks.
void SettingsDLLTest::OnCommand(WPARAM wParam, LPARAM lParam) {
    const int id = LOWORD(wParam);
    const int code = HIWORD(wParam);
    HWND hCtl = (HWND)lParam;

    if (id == IDOK && code == BN_CLICKED) {
        OnSave();
    }
    else if (code == EN_CHANGE) {
        m_settings.updateNeeded = true;
    }
}

void SettingsDLLTest::OnPaint() {
    // We are using standard Win32 controls, which paint themselves.
    // We only need to paint our background.
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(m_hWnd, &ps);
    // Fill background with the standard dialog color to match the controls
    FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_BTNFACE + 1));
    EndPaint(m_hWnd, &ps);
}

BOOL SettingsDLLTest::Initialize(HWND hWindow, OpenSRContext* context, void* buffersOut, const wchar_t* pluginFolderPath, const uint64_t sharedBuffer)
{
    m_hWnd = hWindow;
    m_pContext = context;
    m_xmlPath = StringUtils::createWString(pluginFolderPath, L"\\settings.xml");
    
    // Load the data from XML into our m_settings struct
    LoadSettings();

    // Subclass the window provided by the host so we can handle its messages.
    SetWindowSubclass(m_hWnd, SettingsDLLTest::SubclassWndProc, 1, (DWORD_PTR)this);

    // Create child controls directly, once.
    CreateControls();
    PopulateFromSettings();

    return TRUE;
}

BOOL SettingsDLLTest::Shutdown()
{
    if(m_hWnd)
        RemoveWindowSubclass(m_hWnd, SettingsDLLTest::SubclassWndProc, 1);
    return m_settings.updateNeeded;
}
    
