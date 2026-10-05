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
#include "BeamNGDriveSettings.h"
#include "StringUtils.h"
#include <windowsx.h>
#include <iostream>
#include <CommCtrl.h>
#include "tinyxml2.h"

#pragma comment(lib, "Comctl32.lib")

//  Exported C Functions

// This function is called by the host application to create an instance of our settings class.
extern "C" __declspec(dllexport) IPluginSettings * CreatePluginSettings() {
    // We simply create a 'new' instance of our implementation class.
    return new BeamNGDriveSettings();
}

// This function is called by the host application to destroy the instance.
extern "C" __declspec(dllexport) void DestroyPluginSettings(IPluginSettings * pSettings) {
    // We cast it back to the concrete type and 'delete' it.
    if (pSettings) {
        delete static_cast<BeamNGDriveSettings*>(pSettings);
    }
}


// Helper for Clipboard ---
std::wstring BeamNGDriveSettings::GetClipboardText() {
    if (!OpenClipboard(nullptr)) return L"";
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (hData == nullptr) {
        CloseClipboard();
        return L"";
    }
    wchar_t* pszText = static_cast<wchar_t*>(GlobalLock(hData));
    std::wstring text = pszText ? pszText : L"";
    GlobalUnlock(hData);
    CloseClipboard();
    return text;
}

void BeamNGDriveSettings::SetClipboardText(const std::wstring& text) {
    if (!OpenClipboard(m_hWnd)) return;
    EmptyClipboard();
    HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, (text.size() + 1) * sizeof(wchar_t));
    if (hg) {
        memcpy(GlobalLock(hg), text.c_str(), (text.size() + 1) * sizeof(wchar_t));
        GlobalUnlock(hg);
        SetClipboardData(CF_UNICODETEXT, hg);
    }
    CloseClipboard();
}

// INPUT FIELD LOGIC

IDWriteTextLayout* BeamNGDriveSettings::InputField::CreateLayout(IDWriteFactory* pFactory, IDWriteTextFormat* pFormat) {
    IDWriteTextLayout* layout = nullptr;
    pFactory->CreateTextLayout(value.c_str(), (UINT32)value.length(), pFormat, (rect.right - rect.left) + 100.0f, rect.bottom - rect.top, &layout);
    return layout;
}

void BeamNGDriveSettings::InputField::Draw(ID2D1RenderTarget* pRT, ID2D1SolidColorBrush* pTextBrush, ID2D1SolidColorBrush* pBgBrush, ID2D1SolidColorBrush* pSelBrush, ID2D1SolidColorBrush* pErrorBrush, bool isFocused) {
    pRT->FillRectangle(rect, pBgBrush);

    ID2D1Brush* borderBrush = pTextBrush;
    float thickness = 1.0f;

    if (isFocused) {
        borderBrush = pSelBrush;
        thickness = 2.0f;
    }

    // If invalid format (detected during typing or validation), show Red
    if (!isValid) {
        borderBrush = pErrorBrush;
        thickness = 2.0f;
    }

    pRT->DrawRectangle(rect, borderBrush, thickness);
}

// Basic IP Format check
bool BeamNGDriveSettings::InputField::IsIPValid(const std::wstring& ip) {
    int a = -1, b = -1, c = -1, d = -1;
    // Try to parse 4 integers separated by dots
    // Note: swscanf_s is safer, but swscanf is standard
    int count = swscanf_s(ip.c_str(), L"%d.%d.%d.%d", &a, &b, &c, &d);

    if (count != 4) return false;
    if (a < 0 || a > 255) return false;
    if (b < 0 || b > 255) return false;
    if (c < 0 || c > 255) return false;
    if (d < 0 || d > 255) return false;
    return true;
}

void BeamNGDriveSettings::InputField::Validate() {
    if (type == FieldType::TYPE_NUMBER) {
        if (value.empty()) {
            value = std::to_wstring(minVal);
        }
        else {
            try {
                int val = std::stoi(value);
                if (val < minVal) val = minVal;
                if (val > maxVal) val = maxVal;
                value = std::to_wstring(val);
            }
            catch (...) {
                value = std::to_wstring(minVal);
            }
        }
        isValid = true;
    }
    else if (type == TYPE_IP) {
        // Just check validity. Do NOT reset text.
        // This keeps the user's input so they can fix a typo.
        isValid = IsIPValid(value);
    }

    // Update cursors
    caretPos = value.length();
    selStart = selEnd = caretPos;
}

void BeamNGDriveSettings::InputField::InsertChar(wchar_t c) {
    // 1. Basic Character Filter
    if (type == TYPE_NUMBER) {
        if (c < '0' || c > '9') return;
    }
    else if (type == TYPE_IP) {
        if ((c < '0' || c > '9') && c != '.') return;
    }

    // 2. Simulate the edit to check validity
    std::wstring temp = value;
    size_t start = std::min(selStart, selEnd);
    size_t end = std::max(selStart, selEnd);

    // Simulate Erase
    if (start < temp.length()) {
        temp.erase(start, end - start);
    }
    // Simulate Insert
    temp.insert(start, 1, c);

    // 3. Logic Check on Result
    if (type == TYPE_IP) {
        if (!CheckPartialIP(temp)) return; // Reject key if it creates an invalid segment > 255
    }

    // 4. Apply Change
    ReplaceSelection(std::wstring(1, c));

    // Reset valid flag visual state while typing (assuming partial is valid enough for now)
    if (type == FieldType::TYPE_IP) isValid = true;
}

void BeamNGDriveSettings::InputField::ReplaceSelection(const std::wstring& text) {
    size_t start = std::min(selStart, selEnd);
    size_t end = std::max(selStart, selEnd);
    if (start > value.length()) start = value.length(); // Safety
    if (end > value.length()) end = value.length();


    value.erase(start, end - start);
    value.insert(start, text);

    caretPos = start + text.length();
    selStart = selEnd = caretPos;
}

bool BeamNGDriveSettings::InputField::CheckPartialIP(const std::wstring& proposed) {
    if (proposed.length() > 15) return false; // Max length 255.255.255.255

    std::wstring currentSegment;
    int dots = 0;

    for (size_t i = 0; i < proposed.length(); i++) {
        wchar_t c = proposed[i];
        if (c == '.') {
            dots++;
            if (dots > 3) return false; // Too many dots
            if (!currentSegment.empty()) {
                if (std::stoi(currentSegment) > 255) return false;
            }
            currentSegment.clear();
        }
        else if (c >= '0' && c <= '9') {
            currentSegment += c;
            // Check immediate overflow (e.g. 256)
            if (currentSegment.length() > 3) return false;
            try {
                if (std::stoi(currentSegment) > 255) return false;
            }
            catch (...) { return false; }
        }
    }
    // Check the last segment after the loop
    if (!currentSegment.empty()) {
        try {
            if (std::stoi(currentSegment) > 255) return false;
        }
        catch (...) { return false; }
    }
    return true;
}

void BeamNGDriveSettings::InputField::HandleKey(WPARAM key, bool shift, bool ctrl) {
    if (key == VK_LEFT) {
        if (caretPos > 0) caretPos--;
        if (!shift) selStart = caretPos;
        selEnd = caretPos;
    }
    else if (key == VK_RIGHT) {
        if (caretPos < value.length()) caretPos++;
        if (!shift) selStart = caretPos;
        selEnd = caretPos;
    }
    else if (key == VK_HOME) {
        caretPos = 0;
        if (!shift) selStart = 0;
        selEnd = 0;
    }
    else if (key == VK_END) {
        caretPos = value.length();
        if (!shift) selStart = caretPos;
        selEnd = caretPos;
    }
    else if (key == VK_DELETE) {
        if (selStart != selEnd) ReplaceSelection(L"");
        else if (caretPos < value.length()) {
            value.erase(caretPos, 1);
        }
    }
    else if (key == VK_BACK) {
        if (selStart != selEnd) ReplaceSelection(L"");
        else if (caretPos > 0) {
            value.erase(caretPos - 1, 1);
            caretPos--;
            selStart = selEnd = caretPos;
        }
    }
    else if (ctrl && key == 'A') {
        selStart = 0;
        selEnd = value.length();
        caretPos = selEnd;
    }
}

void BeamNGDriveSettings::ShowToast(const std::wstring& msg) {
    if (m_showToast) return;
    m_toastMsg = msg;
    m_showToast = true;
    m_toastEndTime = GetTickCount64() + TOAST_DURATION_MS;

    // Set a Windows timer to trigger a repaint exactly when the toast expires
    SetTimer(m_hWnd, TOAST_TIMER_ID, TOAST_DURATION_MS, NULL);

    // Trigger immediate repaint to show it
    InvalidateRect(m_hWnd, NULL, FALSE);
}

void BeamNGDriveSettings::DrawToast() {
    if (!m_showToast) return;

    // Check Timeout
    if (GetTickCount64() >= m_toastEndTime) {
        m_showToast = false;
        KillTimer(m_hWnd, TOAST_TIMER_ID);
        return;
    }
    ID2D1SolidColorBrush* pBrushToastBg = nullptr;   // Black 0.6 opacity
    ID2D1SolidColorBrush* pBrushToastText = nullptr; // 0xcccccc

    // Create Brushes if missing
    m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.6f), &pBrushToastBg);
    m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.8f, 0.8f, 0.8f, 1.0f), &pBrushToastText); // 0xCCCCCC is approx 0.8

    D2D1_SIZE_F size = m_pRenderTarget->GetSize();

    // Define Toast Dimensions (Fixed size or dynamic)
    float toastW = 300.0f;
    float toastH = 80.0f;
    float posX = (size.width - toastW) / 2.0f;
    float posY = (size.height - toastH) / 2.0f;

    D2D1_RECT_F rect = { posX, posY, posX + toastW, posY + toastH };
    D2D1_ROUNDED_RECT rounded = { rect, 10.0f, 10.0f };

    // Draw Background
    if (pBrushToastBg)
        m_pRenderTarget->FillRoundedRectangle(&rounded, pBrushToastBg);

    // Draw Text (Centered)
    IDWriteTextLayout* pLayout = nullptr;
    m_pDWriteFactory->CreateTextLayout(m_toastMsg.c_str(), (UINT32)m_toastMsg.length(), m_pTextFormat, toastW, toastH, &pLayout);

    if (pLayout) {
        pLayout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        pLayout->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        if (pBrushToastText)
            m_pRenderTarget->DrawTextLayout(D2D1::Point2F(posX, posY), pLayout, pBrushToastText);

        SafeRelease(&pLayout);
    }

    SafeRelease(&pBrushToastBg);
    SafeRelease(&pBrushToastText);
}


BeamNGDriveSettings::BeamNGDriveSettings() {
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);  
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&m_pDWriteFactory));

}

BeamNGDriveSettings::~BeamNGDriveSettings() {
    Shutdown();
}

BOOL BeamNGDriveSettings::Initialize(HWND hWindow, OpenSRContext* context, void* buffersOut, const wchar_t* pluginFolderPath, const uint64_t sharedBuffer) {
    m_hWnd = hWindow;
    m_pContext = context;
    m_pluginFolderPath = pluginFolderPath;
    m_touched = FALSE;

  //  if(!m_pD2DFactory) D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);
  // if(!m_pDWriteFactory) DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&m_pDWriteFactory));
    SetWindowSubclass(m_hWnd, BeamNGDriveSettings::SubclassWndProc, 1, (DWORD_PTR)this);

    // Initialize Fields with Types
    m_inputs.clear();
    m_inputs.push_back({ "outgauge ip",   L"Outgauge IP:",   L"", {0,0,0,0}, TYPE_IP , 0, 0 });
    m_inputs.push_back({ "outgauge port", L"Outgauge Port:", L"", {0,0,0,0}, TYPE_NUMBER, 1024, 65535 });
    m_inputs.push_back({ "motionsim ip",  L"MotionSim IP:",  L"", {0,0,0,0}, TYPE_IP , 0, 0 });
    m_inputs.push_back({ "motionsim port",L"MotionSim Port:",L"", {0,0,0,0}, TYPE_NUMBER, 1024, 65535 });

    LoadXMLSettings();
    return TRUE;
}

void BeamNGDriveSettings::LoadXMLSettings() {
    tinyxml2::XMLDocument doc;
    std::wstring spath = StringUtils::createWString(m_pluginFolderPath, L"settings.xml");
    std::string pathObj = StringUtils::toNarrow(spath);

    if (doc.LoadFile(pathObj.c_str()) == tinyxml2::XML_SUCCESS) {
        tinyxml2::XMLElement* root = doc.FirstChildElement("settings");
        if (root) {
            for (tinyxml2::XMLElement* opt = root->FirstChildElement("option"); opt; opt = opt->NextSiblingElement("option")) {
                const char* name = opt->Attribute("name");
                const char* val = opt->GetText();
                if (name && val) {
                    for (auto& field : m_inputs) {
                        if (field.name == name) {
                            field.value = StringUtils::toWide(val);
                            field.Validate(); // Validate loaded data immediately
                        }
                    }

                    if (strcmp(name, "outgauge allowed") == 0) {
                        //std::string v = val;
                        m_outgaugeAllowed = opt->BoolText(true);
                    }
                    else if (strcmp(name, "motionsim allowed") == 0) {
                        //std::string v = val;
                        m_motionSimAllowed = opt->BoolText(true);
                    }
                }
            }
        }
    }
}

void BeamNGDriveSettings::SaveXMLSettings() {
    // Validate All Fields first
    bool allValid = true;
    for (auto& field : m_inputs) {
        field.Validate();
        if (!field.isValid) allValid = false;
    }

    // Force redraw to show Red borders
    InvalidateRect(m_hWnd, NULL, FALSE);

    // Guard Clause
    if (!allValid) {
        ShowToast(L"Cannot Save: Invalid Input detected");
        return; // ABORT
    }

    // Proceed with Saving
    tinyxml2::XMLDocument doc;
    std::wstring spath = StringUtils::createWString(m_pluginFolderPath, L"\\settings.xml");
    std::string pathObj = StringUtils::toNarrow(spath);

    doc.LoadFile(pathObj.c_str());
    tinyxml2::XMLElement* root = doc.FirstChildElement("settings");
    if (!root) {
        doc.Clear();
        root = doc.NewElement("settings");
        root->SetAttribute("type", "unique");
        doc.InsertFirstChild(root);
    }

    for (const auto& field : m_inputs) {
        bool found = false;
        for (tinyxml2::XMLElement* opt = root->FirstChildElement("option"); opt; opt = opt->NextSiblingElement("option")) {
            const char* name = opt->Attribute("name");
            if (name && field.name == name) {
                std::string valStr = StringUtils::toNarrow(field.value);
                opt->SetText(valStr.c_str());
                found = true;
                break;
            }
        }
        if (!found) {
            tinyxml2::XMLElement* newOpt = doc.NewElement("option");
            newOpt->SetAttribute("name", field.name.c_str());
            newOpt->SetAttribute("type", field.type == TYPE_NUMBER ? "number" : "text");
            std::string valStr = StringUtils::toNarrow(field.value);
            newOpt->SetText(valStr.c_str());
            root->InsertEndChild(newOpt);
        }
    }

    // Save Toggles
    auto SaveBool = [&](const char* name, bool value)
    {
        bool found = false;

        for (tinyxml2::XMLElement* opt = root->FirstChildElement("option"); opt; opt = opt->NextSiblingElement("option")) {
            const char* n = opt->Attribute("name");
            if (n && strcmp(n, name) == 0) {
                opt->SetText(value ? "true" : "false");
                found = true;
                break;
            }
        }

        if (!found) {
            tinyxml2::XMLElement* newOpt = doc.NewElement("option");
            newOpt->SetAttribute("name", name);
            newOpt->SetAttribute("type", "bool");
            newOpt->SetText(value ? "true" : "false");
            root->InsertEndChild(newOpt);
        }
    };

    SaveBool("outgauge allowed", m_outgaugeAllowed);
    SaveBool("motionsim allowed", m_motionSimAllowed);

    doc.SaveFile(pathObj.c_str());

    m_touched = TRUE;
    // Success Toast
    ShowToast(L"Settings Saved Successfully");

    HWND hParent = GetParent(m_hWnd);
    if (hParent && IsWindow(hParent))
    {
        PostMessage(hParent, _PLUGIN_SETTINGS_CHANGED, 0, 0);
    }
}

BOOL BeamNGDriveSettings::Shutdown() {
    if (m_hWnd && IsWindow(m_hWnd)) {
        RemoveWindowSubclass(m_hWnd, BeamNGDriveSettings::SubclassWndProc, 1);
    }
    SafeRelease(&m_pTextFormat);
    SafeRelease(&m_pRenderTarget);
    SafeRelease(&m_pDWriteFactory);
    SafeRelease(&m_pD2DFactory);
    return m_touched;
}

void BeamNGDriveSettings::OnLButtonDown(int x, int y) {
    SetFocus(m_hWnd);

    // Validate Previous Field before switching
    if (m_focusedIndex >= 0 && m_focusedIndex < m_inputs.size()) {
        m_inputs[m_focusedIndex].Validate();
    }

    m_focusedIndex = -1;

    for (int i = 0; i < m_inputs.size(); ++i) {
        if (x >= m_inputs[i].rect.left && x <= m_inputs[i].rect.right &&
            y >= m_inputs[i].rect.top && y <= m_inputs[i].rect.bottom) {

            m_focusedIndex = i;
            m_isDragging = true;
            SetCapture(m_hWnd);  // Capture mouse so selection works if you drag outside box

            // Handle Caret Placement via HitTest
            if (m_pDWriteFactory && m_pTextFormat) {
                IDWriteTextLayout* pLayout = m_inputs[i].CreateLayout(m_pDWriteFactory, m_pTextFormat);
                if (pLayout) {
                    BOOL isTrailing, isInside;
                    DWRITE_HIT_TEST_METRICS metrics;
                    // Offset mouse X relative to text box text start (approx +5px padding)
                    float clickX = (float)x - (m_inputs[i].rect.left + 5.0f);
                    float clickY = (float)y - m_inputs[i].rect.top;

                    pLayout->HitTestPoint(clickX, clickY, &isTrailing, &isInside, &metrics);
                    m_inputs[i].caretPos = (size_t)metrics.textPosition + (isTrailing ? 1 : 0);

                    // Reset selection
                    m_inputs[i].selStart = m_inputs[i].selEnd = m_inputs[i].caretPos;

                    SafeRelease(&pLayout);
                }
            }
            break;
        }
    }

    // Toggle Outgauge
    if (x >= m_outgaugeToggleRect.left && x <= m_outgaugeToggleRect.right &&
        y >= m_outgaugeToggleRect.top && y <= m_outgaugeToggleRect.bottom) {

        m_outgaugeAllowed = !m_outgaugeAllowed;
        InvalidateRect(m_hWnd, NULL, FALSE);
        return;
    }

    // Toggle MotionSim
    if (x >= m_motionToggleRect.left && x <= m_motionToggleRect.right &&
        y >= m_motionToggleRect.top && y <= m_motionToggleRect.bottom) {

        m_motionSimAllowed = !m_motionSimAllowed;
        InvalidateRect(m_hWnd, NULL, FALSE);
        return;
    }

    if (x >= m_saveBtnRect.left && x <= m_saveBtnRect.right &&
        y >= m_saveBtnRect.top && y <= m_saveBtnRect.bottom) {
        SaveXMLSettings();
    }

    InvalidateRect(m_hWnd, NULL, FALSE);
}

void BeamNGDriveSettings::OnLButtonUp() {
    if (m_isDragging) {
        m_isDragging = false;
        ReleaseCapture(); // Stop capturing mouse
    }
}

void BeamNGDriveSettings::OnMouseMove(int x, int y) {
    if (m_isDragging && m_focusedIndex >= 0 && m_focusedIndex < m_inputs.size()) {
        InputField& f = m_inputs[m_focusedIndex];

        // Recalculate caret position based on new mouse coordinates
        if (m_pDWriteFactory && m_pTextFormat) {
            IDWriteTextLayout* pLayout = f.CreateLayout(m_pDWriteFactory, m_pTextFormat);
            if (pLayout) {
                BOOL isTrailing, isInside;
                DWRITE_HIT_TEST_METRICS metrics;
                // Use relative coordinates
                float mouseX = (float)x - (f.rect.left + 5.0f);
                float mouseY = (float)y - f.rect.top;

                pLayout->HitTestPoint(mouseX, mouseY, &isTrailing, &isInside, &metrics);

                // Update Caret
                size_t newPos = (size_t)metrics.textPosition + (isTrailing ? 1 : 0);
                f.caretPos = newPos;

                // Update Selection End (Keep selStart anchored where click began)
                f.selEnd = newPos;

                SafeRelease(&pLayout);
                InvalidateRect(m_hWnd, NULL, FALSE);
            }
        }
    }
}

void BeamNGDriveSettings::OnChar(wchar_t c) {
    if (m_focusedIndex >= 0 && m_focusedIndex < m_inputs.size()) {
        if (c >= 32) { // Standard chars
            m_inputs[m_focusedIndex].InsertChar(c);
            InvalidateRect(m_hWnd, NULL, FALSE);
        }
    }
}

void BeamNGDriveSettings::OnKeyDown(WPARAM key) {
    if (m_focusedIndex >= 0 && m_focusedIndex < m_inputs.size()) {
        bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

        // TAB NAVIGATION
        if (key == VK_TAB) {
            if (m_inputs.empty()) return;

            // Validate current before leaving
            if (m_focusedIndex >= 0) m_inputs[m_focusedIndex].Validate();

            if (m_focusedIndex == -1) {
                // If none focused, start at 0
                m_focusedIndex = 0;
            }
            else {
                // Cycle logic
                int dir = shift ? -1 : 1;
                m_focusedIndex += dir;

                // Wrap around
                if (m_focusedIndex < 0) m_focusedIndex = (int)m_inputs.size() - 1;
                else if (m_focusedIndex >= m_inputs.size()) m_focusedIndex = 0;
            }

            // Select all text when tabbing into field
            m_inputs[m_focusedIndex].selStart = 0;
            m_inputs[m_focusedIndex].selEnd = m_inputs[m_focusedIndex].value.length();
            m_inputs[m_focusedIndex].caretPos = m_inputs[m_focusedIndex].selEnd;

            InvalidateRect(m_hWnd, NULL, FALSE);
            return;
        }
        if (m_focusedIndex >= 0 && m_focusedIndex < m_inputs.size()) {
            if (ctrl && key == 'V') {
                // PASTE
                std::wstring clip = GetClipboardText();
                for (wchar_t c : clip) {
                    // Crude filtering loop
                    if (c >= 32) m_inputs[m_focusedIndex].InsertChar(c);
                }
            }
            else if (ctrl && key == 'C') {
                std::wstring sel = m_inputs[m_focusedIndex].GetSelectedText();
                if (!sel.empty()) {
                    SetClipboardText(sel);
                }
            }
            else if (ctrl && key == 'X') {
                // NEW CUT LOGIC ---
                std::wstring sel = m_inputs[m_focusedIndex].GetSelectedText();
                if (!sel.empty()) {
                    SetClipboardText(sel);
                    m_inputs[m_focusedIndex].ReplaceSelection(L""); // Delete after copy
                }
            }
            else {
                m_inputs[m_focusedIndex].HandleKey(key, shift, ctrl);
            }

            InvalidateRect(m_hWnd, NULL, FALSE);
        }
    }
}

LRESULT CALLBACK BeamNGDriveSettings::SubclassWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    BeamNGDriveSettings* pThis = (BeamNGDriveSettings*)dwRefData;

    switch (uMsg) {
    case WM_SIZE: {
        if (pThis && pThis->m_pRenderTarget) {
            UINT width = LOWORD(lParam);
            UINT height = HIWORD(lParam);
            pThis->m_pRenderTarget->Resize(D2D1::SizeU(width, height));
        }
        // Force re-creation of RT if needed in OnPaint
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }
    case WM_PAINT:
        if (pThis) pThis->OnPaint();
        ValidateRect(hWnd, NULL);
        return 0;
    case WM_LBUTTONDOWN:
        if (pThis) pThis->OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_LBUTTONUP:
        if (pThis) pThis->OnLButtonUp();
        return 0;
    case WM_MOUSEMOVE:
        if (pThis) pThis->OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_CHAR:
        if (pThis) pThis->OnChar((wchar_t)wParam);
        return 0;
    case WM_TIMER:
        if (pThis && wParam == pThis->TOAST_TIMER_ID) {
            // Timer expired, force repaint to hide toast
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
        }
        break;
    case WM_KEYDOWN:
        if (pThis) pThis->OnKeyDown(wParam);
        // Important: Return 0 for TAB so focus doesn't drift
         if (wParam == VK_TAB) return 0;
        return 0; // Eat the key so game doesn't react
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
    case WM_NCDESTROY:
        RemoveWindowSubclass(hWnd, BeamNGDriveSettings::SubclassWndProc, 1);
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void BeamNGDriveSettings::DrawToggle(const D2D1_RECT_F& rect, bool enabled, ID2D1SolidColorBrush* pBrushInputBg, ID2D1SolidColorBrush* pBrushButton, ID2D1SolidColorBrush* pBrushError, ID2D1SolidColorBrush* pBrushText)
{
    if (!pBrushInputBg || !pBrushButton || !pBrushError || !pBrushText) return;

    float radius = (rect.bottom - rect.top) / 2.0f;

    D2D1_ROUNDED_RECT track = { rect, radius, radius };

    if (enabled)
    {
        // Filled track (green)
        m_pRenderTarget->FillRoundedRectangle(&track, pBrushButton);
    }
   
    {
        // Outline only
        m_pRenderTarget->DrawRoundedRectangle(&track, pBrushText, 1.5f);
    }

    // Thumb size
    float margin = 2.0f;
    float thumbDiameter = (rect.bottom - rect.top) - margin * 2;

    float thumbX = enabled
        ? rect.right - thumbDiameter - margin
        : rect.left + margin;

    D2D1_ELLIPSE thumb =
    {
        { thumbX + thumbDiameter / 2.0f, rect.top + (rect.bottom - rect.top) / 2.0f },
        thumbDiameter / 2.0f,
        thumbDiameter / 2.0f
    };

    m_pRenderTarget->FillEllipse(&thumb, pBrushInputBg);
}

void BeamNGDriveSettings::OnPaint() {
    if (!m_pRenderTarget) {
        RECT rc; GetClientRect(m_hWnd, &rc);
        D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
        if (m_pD2DFactory)
            m_pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(m_hWnd, size), &m_pRenderTarget);
    }
    if (!m_pRenderTarget || !m_pDWriteFactory) return;

    // Brushes
    ID2D1SolidColorBrush* pBrushText = nullptr;    // Gray label
    ID2D1SolidColorBrush* pBrushInputBg = nullptr; // White bg
    ID2D1SolidColorBrush* pBrushInputTxt = nullptr;// Black text
    ID2D1SolidColorBrush* pBrushButton = nullptr;  // Green
    ID2D1SolidColorBrush* pBrushSelect = nullptr;  // Blue Highlight
    ID2D1SolidColorBrush* pBrushError = nullptr; // Red for invalid

    // Create Brushes
   m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Gainsboro), &pBrushText);
   m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &pBrushInputBg);
   m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &pBrushInputTxt);
   m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::ForestGreen), &pBrushButton);
   m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.2f, 0.6f, 1.0f, 0.5f), &pBrushSelect);
   m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &pBrushError);

   if (!pBrushText
       || !pBrushInputBg
       || !pBrushInputTxt
       || !pBrushButton
       || !pBrushSelect
       || !pBrushError)
       return;

    // Text Format
    if (!m_pTextFormat) {
        m_pDWriteFactory->CreateTextFormat(L"Segoe UI", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 14.0f, L"en-us", &m_pTextFormat);
    }

    m_pRenderTarget->BeginDraw();
    m_pRenderTarget->Clear(D2D1::ColorF(0x202020));

    float startX = 20.0f; float startY = 20.0f;
    float rowHeight = 45.0f; float labelWidth = 120.0f; float inputWidth = 150.0f; float inputHeight = 26.0f;

    for (int i = 0; i < m_inputs.size(); ++i) {
        float y = startY + (i * rowHeight);
        InputField& f = m_inputs[i];

        // Label
        m_pRenderTarget->DrawText(f.label.c_str(), (UINT32)f.label.length(), m_pTextFormat, D2D1::RectF(startX, y, startX + labelWidth, y + inputHeight), pBrushText);

        // Input Rect
        f.rect = D2D1::RectF(startX + labelWidth, y, startX + labelWidth + inputWidth, y + inputHeight);

        // Draw Field BG and Border
        f.Draw(m_pRenderTarget, pBrushText, pBrushInputBg, pBrushSelect, pBrushError, (i == m_focusedIndex));
   
        // Create Layout for advanced rendering (Selection/Caret)
        IDWriteTextLayout* pLayout = nullptr;
        m_pDWriteFactory->CreateTextLayout(f.value.c_str(), (UINT32)f.value.length(), m_pTextFormat, inputWidth, inputHeight, &pLayout);

        if (pLayout) {
            float padX = f.rect.left + 5.0f;
            float padY = f.rect.top;

            // Draw Selection Highlight
            if (i == m_focusedIndex && f.selStart != f.selEnd) {
                DWRITE_TEXT_RANGE rng = { (UINT32)std::min(f.selStart, f.selEnd), (UINT32)std::abs((int)f.selEnd - (int)f.selStart) };
                UINT32 count = 0;
                pLayout->HitTestTextRange(rng.startPosition, rng.length, 0, 0, 0, 0, &count);
                if (count > 0) {
                    std::vector<DWRITE_HIT_TEST_METRICS> hitMetrics(count);
                    pLayout->HitTestTextRange(rng.startPosition, rng.length, 0, 0, hitMetrics.data(), count, &count);
                    for (const auto& m : hitMetrics) {
                        D2D1_RECT_F selRect = { padX + m.left, padY + m.top, padX + m.left + m.width, padY + m.top + m.height };
                        m_pRenderTarget->FillRectangle(selRect, pBrushSelect);
                    }
                }
            }

            // Draw Text
            m_pRenderTarget->DrawTextLayout(D2D1::Point2F(padX, padY), pLayout, pBrushInputTxt);

            // Draw Caret
            if (i == m_focusedIndex) {
                float x, y;
                DWRITE_HIT_TEST_METRICS m;
                BOOL isTrailing, isInside;
                // Clamp caret pos to string length
                UINT32 safeCaret = std::min((UINT32)f.caretPos, (UINT32)f.value.length());
                pLayout->HitTestTextPosition(safeCaret, FALSE, &x, &y, &m);

                D2D1_POINT_2F p1 = { padX + x, padY + y };
                D2D1_POINT_2F p2 = { padX + x, padY + y + m.height };
                m_pRenderTarget->DrawLine(p1, p2, pBrushInputTxt, 1.0f);
            }
            SafeRelease(&pLayout);
        }
    }



    // Toggle Switches
    float toggleY = startY + (m_inputs.size() * rowHeight);
    float toggleWidth = 42.0f;
    float toggleHeight = 22.0f;

    // Outgauge Toggle
    m_pRenderTarget->DrawText(
        L"Outgauge Enabled",
        16,
        m_pTextFormat,
        D2D1::RectF(startX, toggleY, startX + labelWidth + 40, toggleY + toggleHeight),
        pBrushText);

    m_outgaugeToggleRect =
        D2D1::RectF(startX + labelWidth + 50, toggleY,
            startX + labelWidth + 50 + toggleWidth,
            toggleY + toggleHeight);

    DrawToggle(m_outgaugeToggleRect, m_outgaugeAllowed, pBrushInputBg, pBrushButton, pBrushError, pBrushText);

    // MotionSim Toggle
    toggleY += rowHeight;

    m_pRenderTarget->DrawText(
        L"MotionSim Enabled",
        17,
        m_pTextFormat,
        D2D1::RectF(startX, toggleY, startX + labelWidth + 40, toggleY + toggleHeight),
        pBrushText);

    m_motionToggleRect =
        D2D1::RectF(startX + labelWidth + 50, toggleY,
            startX + labelWidth + 50 + toggleWidth,
            toggleY + toggleHeight);

    
    DrawToggle(m_motionToggleRect, m_motionSimAllowed, pBrushInputBg, pBrushButton, pBrushError, pBrushText);

    SafeRelease(&pBrushText);
    SafeRelease(&pBrushInputTxt);
    SafeRelease(&pBrushSelect);
    SafeRelease(&pBrushError);

    // Save Button
    float btnY = startY + ((m_inputs.size()+2) * rowHeight) + 10;
    m_saveBtnRect = D2D1::RectF(startX, btnY, startX + 100, btnY + 30);
    D2D1_ROUNDED_RECT roundedBtn = { m_saveBtnRect, 5.0f, 5.0f }; // 5px radius
    m_pRenderTarget->FillRoundedRectangle(&roundedBtn, pBrushButton);

    SafeRelease(&pBrushButton);    
    
    // Centered Text
    m_pRenderTarget->DrawText(L"SAVE", 4, m_pTextFormat, D2D1::RectF(m_saveBtnRect.left + 30, m_saveBtnRect.top + 5, m_saveBtnRect.right, m_saveBtnRect.bottom), pBrushInputBg);

    SafeRelease(&pBrushInputBg);
    // toast msg
    DrawToast();

    m_pRenderTarget->EndDraw();
}
