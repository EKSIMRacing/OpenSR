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
#pragma once
#include "IPluginSettings.h"
#include <d2d1.h>
#include <dwrite.h>
#include <string>
#include <vector>
#include <algorithm> // For min/max

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

template<class T> void SafeRelease(T** ppT) {
    if (*ppT) {
        (*ppT)->Release();
        *ppT = nullptr;
    }
}

class BeamNGDriveSettings : public IPluginSettings {
public:
    BeamNGDriveSettings();
    virtual ~BeamNGDriveSettings();

    BOOL Initialize(HWND hWindow, OpenSRContext* context, void* buffersOut, const wchar_t* pluginFolderPath, const uint64_t sharedBuffer) override;
    BOOL Shutdown() override;
    BOOL WantsDialogMessages() const override { return FALSE; }

    // Input Event Handlers
    void OnLButtonDown(int x, int y);
    void OnChar(wchar_t c);
    void OnKeyDown(WPARAM key); // Added for Arrows/Del/Paste

private:
    // Input Type Enum
    enum FieldType { TYPE_TEXT, TYPE_IP, TYPE_NUMBER };

    // input field struct
    struct InputField {
        std::string name;       // XML Attribute
        std::wstring label;     // Display Label
        std::wstring value;     // Content
        D2D1_RECT_F rect;       // Hitbox
        FieldType type;         // Filter logic

        // filter
        int minVal = 0;
        int maxVal = 0;
        bool isValid = true;

        // Edit State
        size_t caretPos = 0;
        size_t selStart = 0;
        size_t selEnd = 0;
        float scrollOffset = 0.0f;

        void Draw(ID2D1RenderTarget* pRT, ID2D1SolidColorBrush* pTextBrush, ID2D1SolidColorBrush* pBgBrush, ID2D1SolidColorBrush* pSelBrush, ID2D1SolidColorBrush* pErrorBrush, bool isFocused);
       // void HandleClick(int mouseX, IDWriteTextFormat* pFormat, IDWriteFactory* pFactory);
        void InsertChar(wchar_t c);
        void HandleKey(WPARAM key, bool shift, bool ctrl);

        void Validate(); // Called on focus loss
        bool IsIPValid(const std::wstring& ip);

        //std::wstring GetSelection() const;
        void ReplaceSelection(const std::wstring& text);

        // Internal helper to get text layout for measurement
        IDWriteTextLayout* CreateLayout(IDWriteFactory* pFactory, IDWriteTextFormat* pFormat);

        std::wstring GetSelectedText() const {
            if (selStart == selEnd) return L"";
            size_t s = std::min(selStart, selEnd);
            size_t e = std::max(selStart, selEnd);
            return value.substr(s, e - s);
        }

        bool CheckPartialIP(const std::wstring& proposed);
    };

    std::vector<InputField> m_inputs;
    D2D1_RECT_F m_saveBtnRect = {};
    int m_focusedIndex = -1;

    // xml
    void LoadXMLSettings();
    void SaveXMLSettings();

    BOOL m_touched = false;

    // helper
    std::wstring GetClipboardText();

    // Window Logic
    static LRESULT CALLBACK SubclassWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    void OnPaint();

    HWND m_hWnd = nullptr;
    OpenSRContext* m_pContext = nullptr;
    std::wstring m_pluginFolderPath;

    // Resources
    ID2D1Factory* m_pD2DFactory = nullptr;
    IDWriteFactory* m_pDWriteFactory = nullptr;
    ID2D1HwndRenderTarget* m_pRenderTarget = nullptr;


    // Shared Text Format
    IDWriteTextFormat* m_pTextFormat = nullptr;

    bool m_isDragging = false; // Track mouse drag state

    void SetClipboardText(const std::wstring& text);
    void OnMouseMove(int x, int y);
    void OnLButtonUp();

    // TOAST STATE ---
    bool m_showToast = false;
    std::wstring m_toastMsg;
    ULONGLONG m_toastEndTime = 0;
    const UINT TOAST_DURATION_MS = 2000; // 2 seconds
    const UINT_PTR TOAST_TIMER_ID = 999;


    // Toggle switches
    bool m_outgaugeAllowed = true;
    bool m_motionSimAllowed = true;

    D2D1_RECT_F m_outgaugeToggleRect = {};
    D2D1_RECT_F m_motionToggleRect = {};

    void DrawToggle(const D2D1_RECT_F& rect, bool enabled, ID2D1SolidColorBrush* pBrushInputBg, ID2D1SolidColorBrush* pBrushButton, ID2D1SolidColorBrush* pBrushError, ID2D1SolidColorBrush* pBrushText);
    
    // Toast Methods
    void ShowToast(const std::wstring& msg);
    void DrawToast();
};
