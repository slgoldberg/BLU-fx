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

#include "settings-img-window.h"
#include "blu_fx.h"
#include "ImgFontAtlas.h"

#include <climits>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <string>

// Load compressed font arrays and FontAwesome 5 icon definitions from ImgWindow/sample-code/fonts/:
#include "sample-code/fonts/IconsFontAwesome5.h"
#include "sample-code/fonts/fa-solid-900.inc"                      /* FontAwesome icons */
#include "sample-code/fonts/gf-roboto-regular-400.inc"
#include "sample-code/fonts/gf-roboto-bold.inc"
#include "sample-code/fonts/gf-robotomono-regular-400.inc"
#include "sample-code/fonts/gf-roboto-italic-400.inc"
#include "sample-code/fonts/gf-robotomono-regular.inc"            /* 500 medium */

constexpr int SYSTEM_FONT_SIZE = 14;
#define BASELINE_FONT_SIZE    ( SYSTEM_FONT_SIZE )
#define FA_FONT_SIZE(x)       ( (x) - 2 )

// Persistent Markdown configuration (bound to ImGui::SetDefaultMarkdownConfig):
static ImGui::MarkdownConfig s_MarkdownConfig;

// Persistent TimedTooltip configuration (bound to ImGui::TimedTooltip::SetDefaultTooltipConfig):
static ImGui::TimedTooltip::TooltipConfig s_TooltipConfig;

bool InitializeImGui()
{
    if (!ImgWindow::sFontAtlas) {
        ImgWindow::sFontAtlas = std::make_shared<ImgFontAtlas>();
    }

    ImFontConfig mainConfig;

    // -------------- Fonts[0 = IM_NORMAL_FONT]: ------------------------------
    std::strncpy(mainConfig.Name, "Roboto-Regular-400-14px", sizeof(mainConfig.Name) - 1);
    mainConfig.GlyphOffset = { 0.f, 0.f };
    ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF(
        gf_roboto_regular_400_compressed_data,
        gf_roboto_regular_400_compressed_size,
        BASELINE_FONT_SIZE, &mainConfig);

    ImFontConfig mergeConfig;
    mergeConfig.MergeMode = true;

    static ImVector<ImWchar> icon_ranges;
    if (icon_ranges.empty()) {
        ImFontGlyphRangesBuilder builder;
        builder.AddText(ICON_FA_ALIGN_CENTER
                        ICON_FA_ANGLE_DOUBLE_DOWN
                        ICON_FA_ANGLE_DOUBLE_LEFT
                        ICON_FA_ANGLE_DOUBLE_RIGHT
                        ICON_FA_ANGLE_DOUBLE_UP
                        ICON_FA_ANGLE_DOWN
                        ICON_FA_ANGLE_LEFT
                        ICON_FA_ANGLE_RIGHT
                        ICON_FA_ANGLE_UP
                        ICON_FA_ARROW_CIRCLE_DOWN
                        ICON_FA_ARROW_CIRCLE_LEFT
                        ICON_FA_ARROW_CIRCLE_RIGHT
                        ICON_FA_ARROW_CIRCLE_UP
                        ICON_FA_ARROW_DOWN
                        ICON_FA_ARROW_LEFT
                        ICON_FA_ARROW_RIGHT
                        ICON_FA_ARROW_UP
                        ICON_FA_ARROWS_ALT_H
                        ICON_FA_ARROWS_ALT_V
                        ICON_FA_ASTERISK
                        ICON_FA_CAMERA
                        ICON_FA_CARET_DOWN
                        ICON_FA_CARET_LEFT
                        ICON_FA_CARET_RIGHT
                        ICON_FA_CARET_SQUARE_DOWN
                        ICON_FA_CARET_SQUARE_LEFT
                        ICON_FA_CARET_SQUARE_RIGHT
                        ICON_FA_CARET_SQUARE_UP
                        ICON_FA_CARET_UP
                        ICON_FA_CART_ARROW_DOWN
                        ICON_FA_CHECK
                        ICON_FA_CHECK_CIRCLE
                        ICON_FA_CHECK_SQUARE
                        ICON_FA_CHEVRON_CIRCLE_LEFT
                        ICON_FA_CHEVRON_CIRCLE_RIGHT
                        ICON_FA_CHEVRON_DOWN
                        ICON_FA_CHEVRON_LEFT
                        ICON_FA_CHEVRON_RIGHT
                        ICON_FA_CHEVRON_UP
                        ICON_FA_CLIPBOARD_LIST
                        ICON_FA_CLOCK
                        ICON_FA_COG
                        ICON_FA_CROSS
                        ICON_FA_DHARMACHAKRA
                        ICON_FA_DIAGNOSES
                        ICON_FA_EDIT
                        ICON_FA_ELLIPSIS_H
                        ICON_FA_ELLIPSIS_V
                        ICON_FA_EXCLAMATION
                        ICON_FA_EXCLAMATION_CIRCLE
                        ICON_FA_EXCLAMATION_TRIANGLE
                        ICON_FA_EYE
                        ICON_FA_EXTERNAL_LINK_SQUARE_ALT
                        ICON_FA_FAST_FORWARD
                        ICON_FA_FAST_BACKWARD
                        ICON_FA_FILE
                        ICON_FA_FILE_ALT
                        ICON_FA_FROWN
                        ICON_FA_FROWN_OPEN
                        ICON_FA_FOLDER_OPEN
                        ICON_FA_I_CURSOR
                        ICON_FA_ICONS
                        ICON_FA_INFO
                        ICON_FA_INFO_CIRCLE
                        ICON_FA_HAND_POINT_LEFT
                        ICON_FA_HAND_POINT_RIGHT
                        ICON_FA_LEVEL_UP_ALT
                        ICON_FA_LIGHTBULB
                        ICON_FA_LINK
                        ICON_FA_LIST
                        ICON_FA_LIST_ALT
                        ICON_FA_LIST_OL
                        ICON_FA_LIST_UL
                        ICON_FA_LOCATION_ARROW
                        ICON_FA_LOCK
                        ICON_FA_LOCK_OPEN
                        ICON_FA_LONG_ARROW_ALT_DOWN
                        ICON_FA_LONG_ARROW_ALT_LEFT
                        ICON_FA_LONG_ARROW_ALT_RIGHT
                        ICON_FA_LONG_ARROW_ALT_UP
                        ICON_FA_MAP_MARKER_ALT
                        ICON_FA_MINUS
                        ICON_FA_MINUS_SQUARE
                        ICON_FA_PAUSE_CIRCLE
                        ICON_FA_PLANE
                        ICON_FA_PLAY
                        ICON_FA_PLUS
                        ICON_FA_PLUS_SQUARE
                        ICON_FA_QUESTION
                        ICON_FA_QUESTION_CIRCLE
                        ICON_FA_REMOVE_FORMAT
                        ICON_FA_SAVE
                        ICON_FA_SEARCH
                        ICON_FA_SIGN_IN_ALT
                        ICON_FA_SIGN_OUT_ALT
                        ICON_FA_SLIDERS_H
                        ICON_FA_SMILE
                        ICON_FA_SMILE_BEAM
                        ICON_FA_SMILE_WINK
                        ICON_FA_SORT
                        ICON_FA_SQUARE
                        ICON_FA_SPINNER
                        ICON_FA_STEP_BACKWARD
                        ICON_FA_REDO
                        ICON_FA_ROAD
                        ICON_FA_STAR
                        ICON_FA_TACHOMETER_ALT
                        ICON_FA_TASKS
                        ICON_FA_TEXT_WIDTH
                        ICON_FA_THUMBS_UP
                        ICON_FA_TIMES
                        ICON_FA_TIMES_CIRCLE
                        ICON_FA_TOGGLE_OFF
                        ICON_FA_TOGGLE_ON
                        ICON_FA_TOOLS
                        ICON_FA_TRADEMARK
                        ICON_FA_TRASH
                        ICON_FA_TRASH_ALT
                        ICON_FA_USER_CLOCK
                        ICON_FA_UNDO
                        ICON_FA_UNDO_ALT
                        ICON_FA_UPLOAD
                        ICON_FA_USER_EDIT
                        ICON_FA_WINDOW_CLOSE
                        ICON_FA_WINDOW_MAXIMIZE
                        ICON_FA_WINDOW_RESTORE);
        builder.BuildRanges(&icon_ranges);
    }

    ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF(
        fa_solid_900_compressed_data, fa_solid_900_compressed_size,
        FA_FONT_SIZE(BASELINE_FONT_SIZE), &mergeConfig, icon_ranges.Data);

    // -------------- Fonts[1 = IMG_TITLE_FONT]: ------------------------------
    int titleFontSize = BASELINE_FONT_SIZE + 3;
    std::strncpy(mainConfig.Name, "Roboto-Regular-400-17px*", sizeof(mainConfig.Name) - 1);
    ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF(
        gf_roboto_regular_400_compressed_data,
        gf_roboto_regular_400_compressed_size,
        titleFontSize, &mainConfig);

    mergeConfig.GlyphOffset = { 0.f, 0.f };
    ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF(
        fa_solid_900_compressed_data, fa_solid_900_compressed_size,
        FA_FONT_SIZE(titleFontSize), &mergeConfig, icon_ranges.Data);

    // -------------- Fonts[2 = IM_SMALLER_FONT]: -----------------------------
    int smallerFontSize = BASELINE_FONT_SIZE - 2;
    std::strncpy(mainConfig.Name, "Roboto-Regular-400-12px*", sizeof(mainConfig.Name) - 1);
    ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF(
        gf_roboto_regular_400_compressed_data,
        gf_roboto_regular_400_compressed_size,
        smallerFontSize, &mainConfig);

    ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF(
        fa_solid_900_compressed_data, fa_solid_900_compressed_size,
        FA_FONT_SIZE(smallerFontSize), &mergeConfig, icon_ranges.Data);

    // -------------- Fonts[3 = IM_BOLD_FONT]: --------------------------------
    int boldFontSize = BASELINE_FONT_SIZE;
    std::strncpy(mainConfig.Name, "Roboto-Bold-700-14px*", sizeof(mainConfig.Name) - 1);
    mainConfig.GlyphOffset = { 0.f, 0.f };
    ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF(
        gf_roboto_bold_compressed_data,
        gf_roboto_bold_compressed_size,
        boldFontSize, &mainConfig);

    std::strncpy(mergeConfig.Name, mainConfig.Name, sizeof(mergeConfig.Name) - 1);
    ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF(
        fa_solid_900_compressed_data, fa_solid_900_compressed_size,
        FA_FONT_SIZE(boldFontSize), &mergeConfig, icon_ranges.Data);

    // -------------- Fonts[4 = IM_BOLD_LARGER_FONT]: -------------------------
    int boldLargeFontSize = BASELINE_FONT_SIZE + 2;
    std::strncpy(mainConfig.Name, "Roboto-Bold-700-16px*", sizeof(mainConfig.Name) - 1);
    ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF(
        gf_roboto_bold_compressed_data,
        gf_roboto_bold_compressed_size,
        boldLargeFontSize, &mainConfig);

    std::strncpy(mergeConfig.Name, mainConfig.Name, sizeof(mergeConfig.Name) - 1);
    ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF(
        fa_solid_900_compressed_data, fa_solid_900_compressed_size,
        FA_FONT_SIZE(boldLargeFontSize), &mergeConfig, icon_ranges.Data);

    // -------------- Fonts[5 = IM_MONO_NORMAL_FONT]: -------------------------
    int monoFontSize = BASELINE_FONT_SIZE;
    std::strncpy(mainConfig.Name, "RobotoMono-Regular-400-15px", sizeof(mainConfig.Name) - 1);
    mainConfig.GlyphOffset = { 0.f, 0.f };
    ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF(
        gf_robotomono_regular_400_compressed_data,
        gf_robotomono_regular_400_compressed_size,
        monoFontSize, &mainConfig);

    std::strncpy(mergeConfig.Name, mainConfig.Name, sizeof(mergeConfig.Name) - 1);
    ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF(
        fa_solid_900_compressed_data, fa_solid_900_compressed_size,
        FA_FONT_SIZE(monoFontSize), &mergeConfig, icon_ranges.Data);

    // -------------- Fonts[6 = IM_ITALIC_FONT]: ------------------------------
    int italicFontSize = BASELINE_FONT_SIZE;
    std::strncpy(mainConfig.Name, "Roboto-Italic-400-14px", sizeof(mainConfig.Name) - 1);
    mainConfig.GlyphOffset = { 0.f, 0.f };
    ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF(
        gf_roboto_italic_400_compressed_data,
        gf_roboto_italic_400_compressed_size,
        italicFontSize, &mainConfig);

    std::strncpy(mergeConfig.Name, mainConfig.Name, sizeof(mergeConfig.Name) - 1);
    ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF(
        fa_solid_900_compressed_data, fa_solid_900_compressed_size,
        FA_FONT_SIZE(italicFontSize), &mergeConfig, icon_ranges.Data);

    // -------------- Fonts[7 = IM_MONO_MEDIUM_FONT]: -------------------------
    int mediumMonoFontSize = BASELINE_FONT_SIZE + 1;
    std::strncpy(mainConfig.Name, "RobotoMono-500-14px", sizeof(mainConfig.Name) - 1);
    ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF(
        gf_robotomono_regular_compressed_data,
        gf_robotomono_regular_compressed_size,
        mediumMonoFontSize, &mainConfig);

    std::strncpy(mergeConfig.Name, mainConfig.Name, sizeof(mergeConfig.Name) - 1);
    mergeConfig.GlyphOffset = { 0.f, 0.f };
    ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF(
        fa_solid_900_compressed_data, fa_solid_900_compressed_size,
        FA_FONT_SIZE(mediumMonoFontSize), &mergeConfig, icon_ranges.Data);

    // -------------- Configure ImGui::Markdown Semantic Fonts ----------------
    ImFontAtlas* rawAtlas = ImgWindow::sFontAtlas->getAtlas();
    if (rawAtlas && rawAtlas->Fonts.Size >= 7) {
        s_MarkdownConfig.h1Font     = rawAtlas->Fonts[4]; // IM_BOLD_LARGER_FONT
        s_MarkdownConfig.h2Font     = rawAtlas->Fonts[1]; // IMG_TITLE_FONT
        s_MarkdownConfig.h3Font     = rawAtlas->Fonts[3]; // IM_BOLD_FONT
        s_MarkdownConfig.boldFont   = rawAtlas->Fonts[3]; // IM_BOLD_FONT
        s_MarkdownConfig.italicFont = rawAtlas->Fonts[6]; // IM_ITALIC_FONT
        s_MarkdownConfig.monoFont   = rawAtlas->Fonts[5]; // IM_MONO_NORMAL_FONT
        s_MarkdownConfig.bulletSpacing = 4.0f;
        ImGui::SetDefaultMarkdownConfig(&s_MarkdownConfig);
    }

    // Configure globally snappier tooltip appearance (67% of default 65 cycles):
    s_TooltipConfig.hover_delay_cycles = 43;
    ImGui::TimedTooltip::SetDefaultTooltipConfig(&s_TooltipConfig);

    return true;
}

void TeardownImGui()
{
    ImGui::TimedTooltip::SetDefaultTooltipConfig(nullptr);

#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19200)
    if (ImGui::GetCurrentContext() != NULL) {
        ImGui::GetIO().Fonts = NULL;
    }
#endif

    if (ImgWindow::sFontAtlas) {
        ImgWindow::sFontAtlas.reset();
    }

    ImgWindow::sBlankoutUntilCycle = XPLMGetCycleNumber() + 4;
    ImgWindow::Shutdown();
}

// ============================================================================
// SettingsImgWindow Implementation
// ============================================================================

// In-memory saved window position across closes (non-persistent across sim sessions):
static int sLastInSimLeft = -1;
static int sLastInSimTop  = -1;
static bool sHasSavedPosition = false;

static WindowGeometry CalcDefaultSettingsGeometry()
{
    int screenL = 0, screenT = 0, screenR = 0, screenB = 0;
    XPLMGetScreenBoundsGlobal(&screenL, &screenT, &screenR, &screenB);

    int w = SettingsImgWindow::DEFAULT_WIDTH;
    int h = SettingsImgWindow::DEFAULT_HEIGHT;
    if (LEGACY_FEATURES) {
        h += 180; // Add room if XP11 Raleigh Scale, FPS-Limiter & Cinema Verite sections are shown
    }

    if (sHasSavedPosition) {
        return WindowGeometry::Sanitize(
            sLastInSimLeft,
            sLastInSimTop,
            sLastInSimLeft + w,
            sLastInSimTop - h,
            w, h,
            40,
            WindowSanitizeMode::KeepVisibleMargin
        );
    }

    return WindowGeometry::FromTopLeft(
        screenL + SettingsImgWindow::DEFAULT_LEFT_OFFSET,
        screenT - SettingsImgWindow::DEFAULT_TOP_OFFSET,
        w,
        h
    );
}

SettingsImgWindow::SettingsImgWindow()
    : ImgWindow(CalcDefaultSettingsGeometry(),
                xplm_WindowDecorationSelfDecorated,
                xplm_WindowLayerFloatingWindows,
                true) // Enable ImGui mouse cursor handoff!
{
    SetWindowTitle(NAME " v" VERSION " - Settings");
    // Allow programmatic shrink-wrapping down to 480px while preventing user resize handles:
    int minW = SettingsImgWindow::DEFAULT_WIDTH;
    int minH = 480;
    if (LEGACY_FEATURES) {
        minH += 180;
    }
    SetWindowResizingLimits(minW, minH, 760, 1200);

    // Top titlebar drag strip (left-drag on titlebar, plus right-drag anywhere on window):
    SetWindowDragArea(0, 0, INT_MAX, 30);

    // Explicitly disable ImGui's own imgui.ini state file saving!
    // (Note: ImgWindow's base constructor has already set mImGuiContext active.)
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    // Subtle translucent background so users can see the simulator scene behind the window:
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg].w = 0.88f;
    style.FrameRounding = 3.0f;
    style.GrabRounding  = 3.0f;

    XPLMCreateFlightLoop_t flDef = {
        sizeof(flDef),
        xplm_FlightLoop_Phase_BeforeFlightModel,
        FlightLoopCallback,
        this
    };
    mFlightLoopId = XPLMCreateFlightLoop(&flDef);
}

SettingsImgWindow::~SettingsImgWindow()
{
    if (mFlightLoopId) {
        XPLMDestroyFlightLoop(mFlightLoopId);
        mFlightLoopId = nullptr;
    }
}

void SettingsImgWindow::RequestClose()
{
    if (HasUnsavedChanges()) {
        mShowCloseConfirmModal = true;
    } else {
        SetVisible(false);
    }
}

void SettingsImgWindow::ResetToDefaultGeometry()
{
    if (IsInsideSim()) {
        WindowGeometry geom = CalcDefaultSettingsGeometry();
        SetWindowGeometry(geom.left, geom.top, geom.right, geom.bottom);
    }
}

bool SettingsImgWindow::onShow()
{
    mShowCloseConfirmModal = false;
    // Always open at the default top-left position and size (no saved window position for now):
    ResetToDefaultGeometry();
    return true;
}

ImGuiWindowFlags_ SettingsImgWindow::beforeBegin()
{
    // Guarantee IniFilename stays null and allow right-drag moving (by NOT setting ImGuiWindowFlags_NoMove).
    // Disable window scrollbars AND prevent mouse-wheel from scrolling the window under any circumstances,
    // so vertical mouse-wheel interaction passes purely and reliably to sliders!
    ImGui::GetIO().IniFilename = nullptr;
    return (ImGuiWindowFlags_)(ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
                              ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar |
                              ImGuiWindowFlags_NoScrollWithMouse);
}

float SettingsImgWindow::FlightLoopCallback(float, float, int, void* inRefcon)
{
    auto* self = static_cast<SettingsImgWindow*>(inRefcon);
    if (self && self->mPendingMode != (XPLMWindowPositioningMode)-1) {
        self->SetWindowPositioningMode(self->mPendingMode);
        if (self->mPendingMode == xplm_WindowPositionFree) {
            self->ResetToDefaultGeometry();
        }
        self->mPendingMode = (XPLMWindowPositioningMode)-1;
    }
    return 0.0f;
}

void SettingsImgWindow::scheduleModeChange(XPLMWindowPositioningMode newMode)
{
    mPendingMode = newMode;
    if (mFlightLoopId) {
        XPLMScheduleFlightLoop(mFlightLoopId, -1.0f, 1);
    }
}

void SettingsImgWindow::drawCustomTitleBar()
{
    if (IM_BOLD_FONT) ImGui::PushFont(IM_BOLD_FONT);
    ImGui::TextUnformatted(ICON_FA_SLIDERS_H "  " NAME " v" VERSION " — Settings");
    if (IM_BOLD_FONT) ImGui::PopFont();

    ImGui::SameLine();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 start = ImGui::GetCursorScreenPos();
    float availX = ImGui::GetContentRegionAvail().x;
    float gripWidth = availX - 65.0f;

    if (gripWidth > 20.0f) {
        start.y += 3.0f; // Shifted up 2px (from 5.0f) to be vertically centered relative to title & buttons
        for (int i = 0; i < 3; ++i) {
            dl->AddLine(start, ImVec2(start.x + gripWidth, start.y), IM_COL32(160, 160, 160, 140), 1.0f);
            start.y += 4.0f;
        }
    }

    ImGui::SameLine(ImGui::GetWindowWidth() - 60.0f);
    if (IsPoppedOut()) {
        if (ImGui::SmallButtonLink(ICON_FA_WINDOW_MAXIMIZE "##popin")) {
            scheduleModeChange(xplm_WindowPositionFree);
        }
        ImGui::TimedTooltip::TextMD("Return this window back inside the X-Plane simulator window.");
    } else {
        if (ImGui::SmallButtonLink(ICON_FA_WINDOW_RESTORE "##popout")) {
            scheduleModeChange(xplm_WindowPopOut);
        }
        ImGui::TimedTooltip::TextMD("Pop out into a standalone operating system window.");
    }

    ImGui::SameLine();
    if (ImGui::SmallButtonLink(ICON_FA_WINDOW_CLOSE "##close")) {
        RequestClose();
    }
    ImGui::TimedTooltip::TextMD("Close the **" NAME "** Settings window.");

    ImGui::Separator();
}

void SettingsImgWindow::buildInterface()
{
    // Hook ImGui error logging so any recoverable error or assert prints to Log.txt:
    ImGuiContext* ctx = ImGui::GetCurrentContext();
    if (ctx && !ctx->ErrorCallback) {
        ctx->ErrorCallback = [](ImGuiContext* /*c*/, void* /*data*/, const char* msg) {
            char buf[512];
            std::snprintf(buf, sizeof(buf), "[BLU-fx ImGui Error] %s\n", msg);
            XPLMDebugString(buf);
        };
    }

    // Tighten vertical item spacing and frame padding so all controls fit compactly without scrolling:
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 3.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 2.0f));

    drawCustomTitleBar();

    // ------------------------------------------------------------------------
    // Header & Attribution Badge
    // ------------------------------------------------------------------------
    bool ppEnabled = (postProcesssingEnabled != 0);
    static const char* const s_PostProcTooltips[2] = {
        "Post-processing filters are currently **OFF**.\nClick or scroll mouse-wheel up to enable.",
        "Post-processing filters are currently **ON**.\n*(While this Settings window is open, the right half of your screen previews the active filter for live A/B comparison!)*"
    };
    if (ImGui::ToggleButton("Enable Post-Processing", &ppEnabled, ImVec2(0, 0), s_PostProcTooltips)) {
        SetPostProcessingEnabled(ppEnabled);
    }

    float infoWidth = ImGui::CalcTextSize(ICON_FA_INFO_CIRCLE " " NAME " v" VERSION).x + 4.0f;
    ImGui::SameLine(ImGui::GetWindowWidth() - infoWidth - 15.0f);
    ImGui::TextLink(ICON_FA_INFO_CIRCLE " " NAME " v" VERSION);
    ImGui::TimedTooltip::TextMD(
        "**" NAME " v" VERSION " (64-bit)**\n"
        "* **Original Author:** Matteo Hausner\n"
        "* **64-bit Port & Modernization:** Steve Goldberg (`@slgoldberg`)\n"
        "* **UI Engine:** `ImgWindow v2` + `contrib/`\n"
        "\n*(Click to toggle this info card)*"
    );

    ImGui::Separator();

    // ------------------------------------------------------------------------
    // Section 1: Post-Processing Sliders (Mouse-Wheel + 1-Click Reset/Undo)
    // ------------------------------------------------------------------------
    if (IM_BOLD_FONT) ImGui::PushFont(IM_BOLD_FONT);
    ImGui::TextUnformatted("Post-Processing Adjustments");
    if (IM_BOLD_FONT) ImGui::PopFont();
    ImGui::TimedTooltip::TextMD(
        "**Tip:** Hover over any slider track and **spin your mouse wheel** for fine 0.01 step adjustments, "
        "or click the **`[↺]`** button next to any slider to toggle between its default and your custom value!"
    );

    const BLUfxPreset& def = BLUfxPresets[PRESET_DEFAULT];

#if BLUFX_SIDE_BY_SIDE_LAYOUT
    // ========================================================================
    // Option B: Sliders on left (widened), action buttons flush-right (split pairs)
    // ========================================================================
    const float rightColWidth = 125.0f;
    const float leftColWidth  = ImGui::GetContentRegionAvail().x - rightColWidth - 16.0f;

    ImGui::BeginGroup(); // Left column: Post-processing sliders
    const float sliderWidth = leftColWidth - 88.0f;
    ImGui::PushItemWidth(sliderWidth);

    // 1-boxel subtle navy border around transparent slider tracks for visual structure:
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.10f, 0.14f, 0.22f, 0.85f));
    // Boost slider grab (thumb) opacity/vibrancy by ~15% for crisp visibility under Metal/Panel Graphics:
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.28f, 0.60f, 0.96f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.34f, 0.68f, 1.00f, 1.00f));

    const float topSliderY = ImGui::GetCursorPosY();
    ImGui::SliderWithReset("Brightness", &brightness, def.brightness, -0.50f, 0.50f, "%.2f", 0, 0.01f);
    ImGui::TimedTooltip::TextMD("Adjusts overall scene luminance offset (`-0.50` to `+0.50`).");

    ImGui::SliderWithReset("Contrast", &contrast, def.contrast, 0.05f, 2.00f, "%.2f", 0, 0.01f);
    ImGui::TimedTooltip::TextMD("Adjusts dynamic range separation (`0.05` to `2.00`, default `1.00`).");

    ImGui::SliderWithReset("Saturation", &saturation, def.saturation, 0.00f, 2.50f, "%.2f", 0, 0.01f);
    ImGui::TimedTooltip::TextMD("Adjusts color intensity (`0.00` = monochrome, `1.00` = normal, up to `2.50`).");

    ImGui::SliderWithReset("Red Scale", &redScale, def.redScale, -0.75f, 0.75f, "%.2f", 0, 0.01f);
    ImGui::TimedTooltip::TextMD("Non-linear mid-tone curve boost/cut for the **Red** channel (`-0.75` to `+0.75`).");

    ImGui::SliderWithReset("Green Scale", &greenScale, def.greenScale, -0.75f, 0.75f, "%.2f", 0, 0.01f);
    ImGui::TimedTooltip::TextMD("Non-linear mid-tone curve boost/cut for the **Green** channel (`-0.75` to `+0.75`).");

    ImGui::SliderWithReset("Blue Scale", &blueScale, def.blueScale, -0.75f, 0.75f, "%.2f", 0, 0.01f);
    ImGui::TimedTooltip::TextMD("Non-linear mid-tone curve boost/cut for the **Blue** channel (`-0.75` to `+0.75`).");

    ImGui::SliderWithReset("Red Offset", &redOffset, def.redOffset, -0.50f, 0.50f, "%.2f", 0, 0.01f);
    ImGui::TimedTooltip::TextMD("Linear additive shift for the **Red** channel (`-0.50` to `+0.50`).");

    ImGui::SliderWithReset("Green Offset", &greenOffset, def.greenOffset, -0.50f, 0.50f, "%.2f", 0, 0.01f);
    ImGui::TimedTooltip::TextMD("Linear additive shift for the **Green** channel (`-0.50` to `+0.50`).");

    ImGui::SliderWithReset("Blue Offset", &blueOffset, def.blueOffset, -0.50f, 0.50f, "%.2f", 0, 0.01f);
    ImGui::TimedTooltip::TextMD("Linear additive shift for the **Blue** channel (`-0.50` to `+0.50`).");

    const float bottomSliderY = ImGui::GetCursorPosY();
    ImGui::SliderWithReset("Vignette", &vignette, def.vignette, 0.00f, 1.00f, "%.2f", 0, 0.01f);
    ImGui::TimedTooltip::TextMD("Darkens the outer edges of the screen (`0.00` = off, `1.00` = maximum vignette).");

    const float leftColBottomY = ImGui::GetCursorPosY();

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(1);
    ImGui::PopItemWidth();
    ImGui::Dummy(ImVec2(0.0f, 0.0f)); // Consumes cursor position to cleanly close group boundary without assert
    ImGui::EndGroup(); // End Left column

    // Position the right column flush against the right margin:
    ImGui::SameLine(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - rightColWidth);

    ImGui::BeginGroup(); // Right column: Vertical action button stack (Split: Top pair & Bottom pair)
    constexpr float VG = 5.0f;
    const float btnHeight = ImGui::GetFrameHeight();
    const ImVec2 actionBtnSize(rightColWidth, btnHeight);

    const bool isDefaultActive = isActivePreset(&BLUfxPresets[PRESET_DEFAULT]);
    const bool isUserActive    = isActivePreset(&BLUfxPresets[PRESET_USER]);

    // --------------------------------------------------------------------
    // Top Pair: Reset All (aligns with top slider) & Restore (VG below)
    // --------------------------------------------------------------------
    ImGui::SetCursorPosY(topSliderY);
    if (isDefaultActive) ImGui::BeginDisabled();
    if (ImGui::ButtonLink(ICON_FA_UNDO " Reset All", actionBtnSize)) {
        ApplyPreset(PRESET_DEFAULT);
        ImGui::TimedTooltip::ClearActiveTooltip();
    }
    if (isDefaultActive) ImGui::EndDisabled();
    ImGui::TimedTooltip::TextMDProxy(
        ImGui::GetItemID(),
        ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled),
        isDefaultActive ? "All sliders are already at factory defaults."
                        : "Reset all post-processing sliders to factory defaults."
    );

    ImGui::SetCursorPosY(topSliderY + btnHeight + VG);
    bool openRestoreMenu = false;
    const bool hasDiscarded = HasDiscardedPreset();

    if (hasDiscarded) {
        const float chevronWidth = 22.0f;
        const float mainBtnWidth = rightColWidth - chevronWidth - 1.0f;

        if (isUserActive) ImGui::BeginDisabled();
        if (ImGui::ButtonLink(ICON_FA_REDO " Restore", ImVec2(mainBtnWidth, btnHeight))) {
            ApplyPreset(PRESET_USER);
            ImGui::TimedTooltip::ClearActiveTooltip();
        }
        if (isUserActive) ImGui::EndDisabled();

        // Right-click on main button (even if disabled) opens the restore context menu (suppressed during window drag):
        if (!IsWindowMoving() && (ImGui::IsItemClicked(ImGuiMouseButton_Right) ||
            (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && ImGui::IsMouseClicked(ImGuiMouseButton_Right)))) {
            openRestoreMenu = true;
        }

        std::string restoreTooltip = isUserActive
            ? "Sliders already match your startup configuration."
            : "Restore sliders to the initial session startup state (captured when X-Plane loaded).";
        restoreTooltip += "\n\n*(Tip: Click or right-click the amber* <pill warning>" ICON_FA_CARET_DOWN "</pill> *or right-click* <pill black yellow>Restore</pill> *to recover discarded edits)*";

        ImGui::TimedTooltip::TextMDProxy(
            ImGui::GetItemID(),
            ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled),
            restoreTooltip.c_str()
        );

        // Chevron dropdown button with soft amber / gold indicator color:
        ImGui::SameLine(0.0f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.55f, 0.15f, 0.70f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.65f, 0.20f, 0.95f));
        if (ImGui::ButtonLink(ICON_FA_CARET_DOWN "##restore_chevron", ImVec2(chevronWidth, btnHeight)) ||
            (!IsWindowMoving() && ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))) {
            openRestoreMenu = true;
            ImGui::TimedTooltip::ClearActiveTooltip();
        }
        ImGui::PopStyleColor(2);
        ImGui::TimedTooltip::TextMD("Saved discarded edits are available!\nClick or right-click to choose whether to restore Startup or recover Discard.");

    } else {
        if (isUserActive) ImGui::BeginDisabled();
        if (ImGui::ButtonLink(ICON_FA_REDO " Restore", actionBtnSize)) {
            ApplyPreset(PRESET_USER);
            ImGui::TimedTooltip::ClearActiveTooltip();
        }
        if (isUserActive) ImGui::EndDisabled();

        if (!IsWindowMoving() && (ImGui::IsItemClicked(ImGuiMouseButton_Right) ||
            (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && ImGui::IsMouseClicked(ImGuiMouseButton_Right)))) {
            openRestoreMenu = true;
        }

        ImGui::TimedTooltip::TextMDProxy(
            ImGui::GetItemID(),
            ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled),
            isUserActive ? "Sliders already match your startup configuration."
                         : "Restore sliders to the initial session startup state (captured when X-Plane loaded)."
        );
    }

    if (openRestoreMenu) {
        ImGui::OpenPopup("##restore_context_menu");
    }

    if (ImGui::BeginPopup("##restore_context_menu")) {
        if (ImGui::MenuItem(ICON_FA_REDO " Restore Session Startup", nullptr, false, !isUserActive)) {
            ApplyPreset(PRESET_USER);
            ImGui::TimedTooltip::ClearActiveTooltip();
        }
        if (ImGui::MenuItem(ICON_FA_UNDO " Recover Last Discarded Edits", nullptr, false, hasDiscarded)) {
            RestoreDiscardedPreset();
            ImGui::TimedTooltip::ClearActiveTooltip();
        }
        ImGui::EndPopup();
    }

    // --------------------------------------------------------------------
    // Bottom Pair: Load .ini (VG above Save) & Save .ini (aligns with Vignette)
    // --------------------------------------------------------------------
    ImGui::SetCursorPosY(bottomSliderY - btnHeight - VG);
    // Distinct soft steel/slate blue accent for Load .ini:
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.42f, 0.60f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.50f, 0.72f, 1.00f));
    if (ImGui::ButtonLink(ICON_FA_FOLDER_OPEN " Load .ini", actionBtnSize)) {
        LoadSettings();
        ImGui::TimedTooltip::ClearActiveTooltip();
    }
    ImGui::PopStyleColor(2);
    ImGui::TimedTooltip::TextMD("Reload settings from `" CONFIG_PATH_DISPLAY "`, discarding unsaved changes.");

    ImGui::SetCursorPosY(bottomSliderY);
    // Distinct forest/emerald green accent for Save .ini:
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.52f, 0.30f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.62f, 0.36f, 1.00f));
    if (ImGui::ButtonLink(ICON_FA_SAVE " Save .ini", actionBtnSize)) {
        SaveSettings();
        ImGui::TimedTooltip::ClearActiveTooltip();
    }
    ImGui::PopStyleColor(2);
    ImGui::TimedTooltip::TextMD("Reload settings from `" CONFIG_PATH_DISPLAY "`, discarding unsaved changes.");

    ImGui::SetCursorPosY(bottomSliderY);
    // Distinct forest/emerald green accent for Save .ini:
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.52f, 0.30f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.62f, 0.36f, 1.00f));
    if (ImGui::ButtonLink(ICON_FA_SAVE " Save .ini", actionBtnSize)) {
        SaveSettings();
    }
    ImGui::PopStyleColor(2);
    ImGui::TimedTooltip::TextMD("Save current settings immediately to `" CONFIG_PATH_DISPLAY "`.");

    ImGui::Dummy(ImVec2(0.0f, 0.0f));
    ImGui::EndGroup(); // End Right column

    ImGui::SetCursorPosY(std::max(leftColBottomY, ImGui::GetCursorPosY()));

#else
    // ========================================================================
    // Option A: Full-width sliders with horizontal button row underneath
    // ========================================================================
    const float sliderWidth = ImGui::GetContentRegionAvail().x * 0.56f;
    ImGui::PushItemWidth(sliderWidth);

    // 1-boxel subtle navy border around transparent slider tracks for visual structure:
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.10f, 0.14f, 0.22f, 0.85f));
    // Boost slider grab (thumb) opacity/vibrancy by ~15% for crisp visibility under Metal/Panel Graphics:
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.28f, 0.60f, 0.96f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.34f, 0.68f, 1.00f, 1.00f));

    ImGui::SliderWithReset("Brightness", &brightness, def.brightness, -0.50f, 0.50f, "%.2f", 0, 0.01f);
    ImGui::TimedTooltip::TextMD("Adjusts overall scene luminance offset (`-0.50` to `+0.50`).");

    ImGui::SliderWithReset("Contrast", &contrast, def.contrast, 0.05f, 2.00f, "%.2f", 0, 0.01f);
    ImGui::TimedTooltip::TextMD("Adjusts dynamic range separation (`0.05` to `2.00`, default `1.00`).");

    ImGui::SliderWithReset("Saturation", &saturation, def.saturation, 0.00f, 2.50f, "%.2f", 0, 0.01f);
    ImGui::TimedTooltip::TextMD("Adjusts color intensity (`0.00` = monochrome, `1.00` = normal, up to `2.50`).");

    ImGui::SliderWithReset("Red Scale", &redScale, def.redScale, -0.75f, 0.75f, "%.2f", 0, 0.01f);
    ImGui::TimedTooltip::TextMD("Non-linear mid-tone curve boost/cut for the **Red** channel (`-0.75` to `+0.75`).");

    ImGui::SliderWithReset("Green Scale", &greenScale, def.greenScale, -0.75f, 0.75f, "%.2f", 0, 0.01f);
    ImGui::TimedTooltip::TextMD("Non-linear mid-tone curve boost/cut for the **Green** channel (`-0.75` to `+0.75`).");

    ImGui::SliderWithReset("Blue Scale", &blueScale, def.blueScale, -0.75f, 0.75f, "%.2f", 0, 0.01f);
    ImGui::TimedTooltip::TextMD("Non-linear mid-tone curve boost/cut for the **Blue** channel (`-0.75` to `+0.75`).");

    ImGui::SliderWithReset("Red Offset", &redOffset, def.redOffset, -0.50f, 0.50f, "%.2f", 0, 0.01f);
    ImGui::TimedTooltip::TextMD("Linear additive shift for the **Red** channel (`-0.50` to `+0.50`).");

    ImGui::SliderWithReset("Green Offset", &greenOffset, def.greenOffset, -0.50f, 0.50f, "%.2f", 0, 0.01f);
    ImGui::TimedTooltip::TextMD("Linear additive shift for the **Green** channel (`-0.50` to `+0.50`).");

    ImGui::SliderWithReset("Blue Offset", &blueOffset, def.blueOffset, -0.50f, 0.50f, "%.2f", 0, 0.01f);
    ImGui::TimedTooltip::TextMD("Linear additive shift for the **Blue** channel (`-0.50` to `+0.50`).");

    ImGui::SliderWithReset("Vignette", &vignette, def.vignette, 0.00f, 1.00f, "%.2f", 0, 0.01f);
    ImGui::TimedTooltip::TextMD("Darkens the outer edges of the screen (`0.00` = off, `1.00` = maximum vignette).");

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(1);
    ImGui::PopItemWidth();

    // Horizontal action button row:
    bool isDefaultActive = isActivePreset(&BLUfxPresets[PRESET_DEFAULT]);
    bool isUserActive    = isActivePreset(&BLUfxPresets[PRESET_USER]);

    if (isDefaultActive) ImGui::BeginDisabled();
    if (ImGui::ButtonLink(ICON_FA_UNDO " Reset All")) {
        ApplyPreset(PRESET_DEFAULT);
        ImGui::TimedTooltip::ClearActiveTooltip();
    }
    if (isDefaultActive) ImGui::EndDisabled();
    ImGui::TimedTooltip::TextMDProxy(
        ImGui::GetItemID(),
        ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled),
        isDefaultActive ? "All sliders are already at factory defaults."
                        : "Reset all post-processing sliders to factory defaults."
    );

    ImGui::SameLine();
    bool openRestoreMenuOptA = false;
    const bool hasDiscardedOptA = HasDiscardedPreset();

    if (isUserActive) ImGui::BeginDisabled();
    if (ImGui::ButtonLink(ICON_FA_REDO " Restore")) {
        ApplyPreset(PRESET_USER);
        ImGui::TimedTooltip::ClearActiveTooltip();
    }
    if (isUserActive) ImGui::EndDisabled();

    // Right-click on main button (even if disabled) opens the restore context menu (suppressed during window drag):
    if (!IsWindowMoving() && (ImGui::IsItemClicked(ImGuiMouseButton_Right) ||
        (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && ImGui::IsMouseClicked(ImGuiMouseButton_Right)))) {
        openRestoreMenuOptA = true;
    }

    std::string restoreTooltipOptA = isUserActive
        ? "Sliders already match your startup configuration."
        : "Restore sliders to the initial session startup state (captured when X-Plane loaded).";
    if (hasDiscardedOptA) {
        restoreTooltipOptA += "\n\n*(Tip: Click or right-click the amber* <pill warning>" ICON_FA_CARET_DOWN "</pill> *or right-click* <pill black yellow>Restore</pill> *to recover discarded edits)*";
    }
    ImGui::TimedTooltip::TextMDProxy(
        ImGui::GetItemID(),
        ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled),
        restoreTooltipOptA.c_str()
    );

    if (hasDiscardedOptA) {
        ImGui::SameLine(0.0f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.55f, 0.15f, 0.70f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.65f, 0.20f, 0.95f));
        if (ImGui::ButtonLink(ICON_FA_CARET_DOWN "##restore_chevron_optA") ||
            (!IsWindowMoving() && ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))) {
            openRestoreMenuOptA = true;
            ImGui::TimedTooltip::ClearActiveTooltip();
        }
        ImGui::PopStyleColor(2);
        ImGui::TimedTooltip::TextMD("Saved discarded edits are available!\nClick or right-click to choose whether to restore Startup or recover Discard.");
    }

    if (openRestoreMenuOptA) {
        ImGui::OpenPopup("##restore_context_optA");
    }

    if (ImGui::BeginPopup("##restore_context_optA")) {
        if (ImGui::MenuItem(ICON_FA_REDO " Restore Session Startup", nullptr, false, !isUserActive)) {
            ApplyPreset(PRESET_USER);
            ImGui::TimedTooltip::ClearActiveTooltip();
        }
        if (ImGui::MenuItem(ICON_FA_UNDO " Recover Last Discarded Edits", nullptr, false, hasDiscardedOptA)) {
            RestoreDiscardedPreset();
            ImGui::TimedTooltip::ClearActiveTooltip();
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 155.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.42f, 0.60f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.50f, 0.72f, 1.00f));
    if (ImGui::ButtonLink(ICON_FA_FOLDER_OPEN " Load .ini")) {
        LoadSettings();
        ImGui::TimedTooltip::ClearActiveTooltip();
    }
    ImGui::PopStyleColor(2);
    ImGui::TimedTooltip::TextMD("Reload settings from `" CONFIG_PATH_DISPLAY "`, discarding unsaved changes.");

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.52f, 0.30f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.62f, 0.36f, 1.00f));
    if (ImGui::ButtonLink(ICON_FA_SAVE " Save .ini")) {
        SaveSettings();
        ImGui::TimedTooltip::ClearActiveTooltip();
    }
    ImGui::PopStyleColor(2);
    ImGui::TimedTooltip::TextMD("Save current settings immediately to `" CONFIG_PATH_DISPLAY "`.");
#endif

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ------------------------------------------------------------------------
    // Section 2: Presets (Italics Typography)
    // ------------------------------------------------------------------------
    if (IM_BOLD_LARGER_FONT) ImGui::PushFont(IM_BOLD_LARGER_FONT);
    ImGui::TextUnformatted("Presets");
    if (IM_BOLD_LARGER_FONT) ImGui::PopFont();
    ImGui::TimedTooltip::TextMD("Click any preset button below to immediately apply that style.");

    int currentCuratedPresetIdx = -1;
    for (int i = PRESET_POLAROID; i < PRESET_MAX; ++i) {
        if (isActivePreset(&BLUfxPresets[i])) {
            currentCuratedPresetIdx = i;
            break;
        }
    }

    ImGui::BeginGroup();
    // 1-boxel subtle navy border around preset buttons for visual structure:
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.12f, 0.18f, 0.28f, 0.85f));

    if (IM_ITALIC_FONT) ImGui::PushFont(IM_ITALIC_FONT);
    if (ImGui::BeginTable("##PresetsGrid", 2, ImGuiTableFlags_SizingStretchSame)) {
        // Render the 18 curated presets in two columns (matching legacy layout order: 2..10 left, 11..19 right):
        const int halfCount = 9;
        for (int row = 0; row < halfCount; ++row) {
            ImGui::TableNextRow();
            const int indices[2] = { PRESET_POLAROID + row, PRESET_SHINY_CALIFORNIA + row };

            for (int col = 0; col < 2; ++col) {
                ImGui::TableSetColumnIndex(col);
                int pIdx = indices[col];
                if (pIdx >= PRESET_MAX) continue;

                bool active = (currentCuratedPresetIdx == pIdx);
                if (active) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.52f, 0.88f, 0.95f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.26f, 0.59f, 0.98f, 1.00f));
                }

                char btnLabel[64];
                if (active) {
                    std::snprintf(btnLabel, sizeof(btnLabel), ICON_FA_CHECK " %s##preset_%d", BLUfxPresetNames[pIdx], pIdx);
                } else {
                    std::snprintf(btnLabel, sizeof(btnLabel), "%s##preset_%d", BLUfxPresetNames[pIdx], pIdx);
                }

                if (ImGui::ButtonLink(btnLabel, ImVec2(-FLT_MIN, 0.0f))) {
                    ImGui::TimedTooltip::ClearActiveTooltip();
                    if (active) {
                        // Toggled off! Restore pre-preset scratchpad or startup baseline:
                        if (HasPrePresetScratchpad()) {
                            RestorePrePresetScratchpad();
                        } else {
                            ApplyPreset(PRESET_USER);
                        }
                    } else {
                        // Entering presets: if coming from a non-curated state, remember it:
                        if (currentCuratedPresetIdx == -1) {
                            SavePrePresetScratchpad();
                        }
                        ApplyPreset(pIdx);
                    }
                }

                if (active) {
                    ImGui::TimedTooltip::TextMD(
                        "Active preset: **%s**\n%s\n\n*(Click again to toggle off and revert to previous settings)*",
                        BLUfxPresetNames[pIdx], BLUfxPresetDescriptions[pIdx]);
                } else {
                    ImGui::TimedTooltip::TextMD(
                        "**%s**\n%s\n\n*(Click to apply this preset)*",
                        BLUfxPresetNames[pIdx], BLUfxPresetDescriptions[pIdx]);
                }

                if (active) {
                    ImGui::PopStyleColor(2);
                }
            }
        }
        ImGui::EndTable();
    }
    if (IM_ITALIC_FONT) ImGui::PopFont();
    ImGui::PopStyleColor(1);
    ImGui::PopStyleVar(1);
    ImGui::EndGroup();

    // ------------------------------------------------------------------------
    // Legacy XP11 Sections (Raleigh Scale, FPS-Limiter, Cinema Verite)
    // ------------------------------------------------------------------------
    if (LEGACY_FEATURES) {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.10f, 0.14f, 0.22f, 0.85f));

        ImGui::Separator();
        if (IM_BOLD_FONT) ImGui::PushFont(IM_BOLD_FONT);
        ImGui::TextUnformatted("Raleigh Scale (X-Plane 11)");
        if (IM_BOLD_FONT) ImGui::PopFont();
        ImGui::PushItemWidth(sliderWidth);
        if (ImGui::SliderWithReset("Raleigh Scale", &raleighScale, DEFAULT_RALEIGH_SCALE, 1.0f, 100.0f, "%.0f", 0, 1.0f)) {
            UpdateRaleighScale(0);
        }
        ImGui::TimedTooltip::TextMD("Adjusts atmospheric Rayleigh scattering (`sim/private/controls/atmo/atmo_scale_raleigh`).");
        ImGui::PopItemWidth();

        ImGui::Separator();
        if (IM_BOLD_FONT) ImGui::PushFont(IM_BOLD_FONT);
        ImGui::TextUnformatted("FPS-Limiter");
        if (IM_BOLD_FONT) ImGui::PopFont();

        bool fpsEnabled = (fpsLimiterEnabled != 0);
        if (ImGui::ToggleButton("Enable FPS-Limiter", &fpsEnabled)) {
            SetFpsLimiterEnabled(fpsEnabled);
        }
        ImGui::TimedTooltip::TextMD("Limits X-Plane's maximum frame rate to smooth out frame-time spikes.");

        ImGui::PushItemWidth(sliderWidth);
        ImGui::SliderWithReset("Max FPS", &maxFps, DEFAULT_MAX_FRAME_RATE, 20.0f, 200.0f, "%.0f fps", 0, 1.0f);
        ImGui::TimedTooltip::TextMD("Target maximum frame rate when FPS-Limiter is enabled (`20` to `200` FPS).");
        ImGui::PopItemWidth();

        ImGui::Separator();
        if (IM_BOLD_FONT) ImGui::PushFont(IM_BOLD_FONT);
        ImGui::TextUnformatted("Auto Disable / Enable Cinema Verite");
        if (IM_BOLD_FONT) ImGui::PopFont();

        bool cvEnabled = (controlCinemaVeriteEnabled != 0);
        if (ImGui::ToggleButton("Control Cinema Verite", &cvEnabled)) {
            SetControlCinemaVeriteEnabled(cvEnabled);
        }
        ImGui::TimedTooltip::TextMD("Automatically pauses Cinema Verite camera shake in the 3D cockpit while you move the mouse.");

        ImGui::PushItemWidth(sliderWidth);
        ImGui::SliderWithReset("On input disable for", &disableCinemaVeriteTime, DEFAULT_DISABLE_CINEMA_VERITE_TIME, 1.0f, 30.0f, "%.0f sec", 0, 1.0f);
        ImGui::TimedTooltip::TextMD("Duration (in seconds) to keep Cinema Verite paused after mouse interaction stops (`1` to `30` sec).");
        ImGui::PopItemWidth();

        ImGui::PopStyleColor(1);
        ImGui::PopStyleVar(1);
    }

    ImGui::PopStyleVar(2); // Pop ItemSpacing and FramePadding

    // ------------------------------------------------------------------------
    // Dynamic Height Shrink-Wrapping (No-Eyeballing Auto-Fit)
    // ------------------------------------------------------------------------
    int left, top, right, bottom;
    GetWindowGeometry(left, top, right, bottom);
    int currentH = top - bottom;
    int targetH = (int)(ImGui::GetCursorPosY() + ImGui::GetStyle().WindowPadding.y);

    // Defensive bounds guard: height must be realistic (min 450px, max 1200px)
    constexpr int minValidH = 450;
    constexpr int maxValidH = 1200;
    if (targetH >= minValidH && targetH <= maxValidH && std::abs(currentH - targetH) > 1 && !IsPoppedOut()) {
        auto geom = WindowGeometry::FromTopLeft(left, top, right - left, targetH);
        SetWindowGeometry(geom.left, geom.top, geom.right, geom.bottom);
    }

    // Continuously remember window position in-memory while inside the simulator:
    if (!IsPoppedOut() && (right - left) > 0 && (top - bottom) > 0) {
        sLastInSimLeft = left;
        sLastInSimTop  = top;
        sHasSavedPosition = true;
    }

    // ------------------------------------------------------------------------
    // Unsaved Changes Confirmation Modal Dialog
    // ------------------------------------------------------------------------
    if (mShowCloseConfirmModal) {
        ImGui::OpenPopup("Unsaved Changes##close_confirm");
    }

    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Unsaved Changes##close_confirm", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (IM_BOLD_FONT) ImGui::PushFont(IM_BOLD_FONT);
        ImGui::TextUnformatted(ICON_FA_EXCLAMATION_TRIANGLE "  Save Changes to Disk?");
        if (IM_BOLD_FONT) ImGui::PopFont();

        ImGui::Spacing();
        ImGui::TextUnformatted("You have adjusted settings that have not been saved to:");
        ImGui::TextColored(ImVec4(0.40f, 0.80f, 1.00f, 1.00f), "  " CONFIG_PATH_DISPLAY);
        ImGui::TextUnformatted("What would you like to do before closing?");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Distinct forest green accent for Save & Close:
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.52f, 0.30f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.62f, 0.36f, 1.00f));
        if (ImGui::ButtonLink(ICON_FA_SAVE " Save & Close")) {
            SaveSettings();
            mShowCloseConfirmModal = false;
            ImGui::CloseCurrentPopup();
            SetVisible(false);
        }
        ImGui::PopStyleColor(2);

        ImGui::SameLine();
        if (ImGui::ButtonLink(ICON_FA_TRASH_ALT " Discard & Close")) {
            CaptureDiscardedPreset();
            LoadSettings();
            mShowCloseConfirmModal = false;
            ImGui::CloseCurrentPopup();
            SetVisible(false);
        }

        ImGui::SameLine();
        if (ImGui::ButtonLink(ICON_FA_UNDO " Cancel")) {
            mShowCloseConfirmModal = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}
