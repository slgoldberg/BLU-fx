/* Copyright (C) 2024-2026  Steve Goldberg
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef SETTINGS_IMG_WINDOW_H
#define SETTINGS_IMG_WINDOW_H

#include "ImgWindow.h"
#include "contrib.h"

// Font slot macros matching sample-code/InitializeImGui.cpp:
#define IM_FONT_SLOT(i)       ((ImGui::GetIO().Fonts && (i) < ImGui::GetIO().Fonts->Fonts.Size) ? ImGui::GetIO().Fonts->Fonts[i] : nullptr)
#define IM_NORMAL_FONT        IM_FONT_SLOT(0)
#define IMG_TITLE_FONT        IM_FONT_SLOT(1)
#define IM_SMALLER_FONT       IM_FONT_SLOT(2)
#define IM_BOLD_FONT          IM_FONT_SLOT(3)
#define IM_BOLD_LARGER_FONT   IM_FONT_SLOT(4)
#define IM_MONO_NORMAL_FONT   IM_FONT_SLOT(5)
#define IM_ITALIC_FONT        IM_FONT_SLOT(6)
#define IM_MONO_MEDIUM_FONT   IM_FONT_SLOT(7)

// Layout option for the Settings window:
// 0 = Option A (Sliders full width with horizontal action button row underneath)
// 1 = Option B (Sliders on the left, vertical action button stack on the right)
#ifndef BLUFX_SIDE_BY_SIDE_LAYOUT
#define BLUFX_SIDE_BY_SIDE_LAYOUT 1
#endif

// Experimental: Rotary Knob/Dial UI for Post-Processing Adjustments
// 1 = Rotary Dials grouped in visual triads (Tone, RGB Midtones, RGB Shadows, Optics)
// 0 = Classic SliderWithReset layout (Default for v2.0 release)
#ifndef BLUFX_EXPERIMENTAL_KNOB_UI
#define BLUFX_EXPERIMENTAL_KNOB_UI 0
#endif

// Global ImGui font atlas initialization & teardown:
bool InitializeImGui();
void TeardownImGui();

class SettingsImgWindow : public ImgWindow {
public:
    static constexpr int DEFAULT_LEFT_OFFSET = 10;
    static constexpr int DEFAULT_TOP_OFFSET  = 35;
#if BLUFX_SIDE_BY_SIDE_LAYOUT
#if BLUFX_EXPERIMENTAL_KNOB_UI
    static constexpr int DEFAULT_WIDTH       = 520;
    static constexpr int DEFAULT_HEIGHT      = 555;
#else
    static constexpr int DEFAULT_WIDTH       = 465;
    static constexpr int DEFAULT_HEIGHT      = 555;
#endif
#else
    static constexpr int DEFAULT_WIDTH       = 430;
    static constexpr int DEFAULT_HEIGHT      = 555;
#endif

    SettingsImgWindow();
    ~SettingsImgWindow() override;

    // Resets the window to its default top-left position and dimensions
    // whenever reopened (since we intentionally do not persist window position yet):
    void ResetToDefaultGeometry();

    // Requests closing the window. If there are unsaved adjustments vs disk,
    // opens the confirmation modal dialog; otherwise closes immediately:
    void RequestClose();
    bool IsCloseConfirmOpen() const { return mShowCloseConfirmModal; }

protected:
    bool onShow() override;
    ImGuiWindowFlags_ beforeBegin() override;
    void buildInterface() override;

private:
    XPLMFlightLoopID mFlightLoopId = nullptr;
    XPLMWindowPositioningMode mPendingMode = (XPLMWindowPositioningMode)-1;
    bool mShowCloseConfirmModal = false;

    static float FlightLoopCallback(float inElapsedSinceLastCall,
                                    float inElapsedTimeSinceLastFlightLoop,
                                    int inCounter,
                                    void* inRefcon);

    void scheduleModeChange(XPLMWindowPositioningMode newMode);
    void drawCustomTitleBar();
};

#endif // SETTINGS_IMG_WINDOW_H
