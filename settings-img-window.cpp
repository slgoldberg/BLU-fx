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
#include "doc-viewer-img-window.h"
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
                        ICON_FA_BOOK
                        ICON_FA_BOOK_OPEN
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
    ImgWindow::GetMainMonitorBoundsGlobal(&screenL, &screenT, &screenR, &screenB);

    int w = SettingsImgWindow::DEFAULT_WIDTH;
    int h = SettingsImgWindow::DEFAULT_HEIGHT;
    if (LEGACY_FEATURES) {
        h += 200; // Add room if XP11 Raleigh Scale, FPS-Limiter & Cinema Verite sections are shown
    }

    if (sHasSavedPosition || (sWindowTop > 0 && sWindowLeft != -1)) {
        int left = sHasSavedPosition ? sLastInSimLeft : sWindowLeft;
        int top  = sHasSavedPosition ? sLastInSimTop  : sWindowTop;
        return WindowGeometry::Sanitize(
            left,
            top,
            left + w,
            top - h,
            w, h,
            40,
            WindowSanitizeMode::KeepVisibleMargin,
            xplm_WindowDecorationSelfDecorated
        );
    }

    return WindowGeometry::FromMainMonitorTopLeft(
        SettingsImgWindow::DEFAULT_LEFT_OFFSET,
        SettingsImgWindow::DEFAULT_TOP_OFFSET,
        w,
        h,
        xplm_WindowDecorationSelfDecorated
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
        minH += 200;
    }
    SetWindowResizingLimits(minW, minH, 760, 1200);

    // Top titlebar drag strip (left-drag on titlebar, plus right-drag anywhere on window):
    SetWindowDragArea(0, 0, INT_MAX, 30);

    // Explicitly disable ImGui's own imgui.ini state file saving!
    // (Note: ImgWindow's base constructor has already set mImGuiContext active.)
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    // Apply modern ImGee Charcoal Slate cockpit theme:
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::ApplyDefaultTheme(&style);
    style.Colors[ImGuiCol_WindowBg].w = sUiOpacity;
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
    UpdateMenuCheckmarks();
}

void SettingsImgWindow::RequestClose()
{
    if (sConfirmOnClose && HasUnsavedChanges()) {
        mShowCloseConfirmModal = true;
    } else {
        SetVisible(false);
        SavePreferences();
        UpdateMenuCheckmarks();
    }
}

void SettingsImgWindow::ResetToDefaultGeometry()
{
    if (IsInsideSim()) {
        WindowGeometry geom = CalcDefaultSettingsGeometry();
        SetWindowGeometry(geom.left, geom.top, geom.right, geom.bottom);
    }
}

static void SyncUIStateWithActiveProfile();

bool SettingsImgWindow::onShow()
{
    mShowCloseConfirmModal = false;
    SyncUIStateWithActiveProfile();
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

static const char* const s_AboutTooltipMD =
    "**" NAME " v" VERSION "**\n\n"
    "* **Original Author:** Matteo Hausner (v1.0)\n"
    "* **Modernization & Features:** Steve Goldberg (`@slgoldberg` on X-Plane.org)\n"
    "* **UI Framework:** `ImgWindow v2` + `ImGee UX`\n"
    "* **Compatibility:** X-Plane 12 & 11 (macOS universal, Windows, Linux)\n\n"
    "*(Click button to toggle or click away to close)*";

void SettingsImgWindow::drawCustomTitleBar()
{
    // Tighten the title bar top margin so the title text, grip lines, and buttons sit centered in the title bar:
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3.0f);

    if (IM_BOLD_FONT) ImGui::PushFont(IM_BOLD_FONT);
    ImGui::TextUnformatted(ICON_FA_SLIDERS_H "  " NAME " v" VERSION " -- Settings");
    if (IM_BOLD_FONT) ImGui::PopFont();

    ImGui::SameLine();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 start = ImGui::GetCursorScreenPos();
    float availX = ImGui::GetContentRegionAvail().x;
    bool showTitleAbout = (sPreviewMode == PREVIEW_MODE_SPLIT);
    float gripWidth = availX - (showTitleAbout ? 115.0f : 90.0f);

    if (gripWidth > 20.0f) {
        // 3 grip lines moved up 1 boxel (startY = centerY - 4.0f) with 1 extra boxel spacing (4.0f):
        const float centerY = start.y + 7.25f;
        const float lineSpacing = 4.0f;
        const float startY = centerY - lineSpacing; // 3 lines: start.y + 3.25f, start.y + 7.25f, start.y + 11.25f

        for (int i = 0; i < 3; ++i) {
            float y = startY + (float)i * lineSpacing;
            dl->AddLine(ImVec2(start.x, y), ImVec2(start.x + gripWidth, y), IM_COL32(160, 165, 175, 105), 1.0f);
        }
    }

    if (showTitleAbout) {
        ImGui::SameLine(ImGui::GetWindowWidth() - 110.0f);
        if (ImGui::SmallButtonLink(ICON_FA_QUESTION_CIRCLE "##doc_help")) {
            if (g_ToggleUserGuideCmd) {
                XPLMCommandOnce(g_ToggleUserGuideCmd);
            }
        }
        ImGui::TimedTooltip::TextMD("**BLU-fx User Guide:**\nOpen the interactive manual, quick reference, and shortcuts guide.");
        ImGui::SameLine();
        ImGui::SmallButtonLink(ICON_FA_INFO_CIRCLE "##about");
        ImGui::TimedTooltip::TextMD(s_AboutTooltipMD);
        ImGui::SameLine();
    } else {
        ImGui::SameLine(ImGui::GetWindowWidth() - 85.0f);
        if (ImGui::SmallButtonLink(ICON_FA_QUESTION_CIRCLE "##doc_help")) {
            if (g_ToggleUserGuideCmd) {
                XPLMCommandOnce(g_ToggleUserGuideCmd);
            }
        }
        ImGui::TimedTooltip::TextMD("**BLU-fx User Guide:**\nOpen the interactive manual, quick reference, and shortcuts guide.");
        ImGui::SameLine();
    }
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

// User profiles and file dialog state:
static std::string s_SelectedPresetFilename;
static int s_ActiveCuratedPresetIdx = -1;
static bool s_SaveModalOpen = false;
static std::string s_SavedFilePath;
static std::string s_DefaultSaveName;
static bool s_OpenModalOpen = false;
static std::string s_OpenFilePath;
static bool s_CloseAfterSaveAs = false;
static bool s_SetAsStartupAfterSave = false;
static std::string s_PrePresetFilename;
static int s_PrePresetActiveCuratedIdx = -1;

static void SyncUIStateWithActiveProfile()
{
    if (sActiveProfileName.rfind("preset:", 0) == 0) {
        s_SelectedPresetFilename.clear();
        std::string pName = sActiveProfileName.substr(7);
        s_ActiveCuratedPresetIdx = -1;
        for (int i = PRESET_POLAROID; i < PRESET_MAX; ++i) {
            if (pName == BLUfxPresetNames[i]) {
                s_ActiveCuratedPresetIdx = i;
                break;
            }
        }
    } else {
        s_SelectedPresetFilename = sActiveProfileName;
        s_ActiveCuratedPresetIdx = -1;
    }
    ClearPrePresetScratchpad();
}

static void RestoreSessionStartup()
{
    ApplyPreset(PRESET_USER);
    sActiveProfileName = sSessionInitialProfile;
    SyncUIStateWithActiveProfile();
    UpdateDiskSnapshot();
}

static std::string GetSmartPresetDefaultFilename(int activeCuratedIdx, const std::string& currentPresetFile)
{
    std::string baseName;
    if (activeCuratedIdx >= PRESET_POLAROID && activeCuratedIdx < PRESET_MAX) {
        baseName = BLUfxPresetNames[activeCuratedIdx];
        const BLUfxPreset& cur = BLUfxPresets[activeCuratedIdx];
        bool tweaked = (std::abs(brightness - cur.brightness) > 0.001f ||
                        std::abs(contrast - cur.contrast) > 0.001f ||
                        std::abs(saturation - cur.saturation) > 0.001f ||
                        std::abs(redScale - cur.redScale) > 0.001f ||
                        std::abs(greenScale - cur.greenScale) > 0.001f ||
                        std::abs(blueScale - cur.blueScale) > 0.001f ||
                        std::abs(redOffset - cur.redOffset) > 0.001f ||
                        std::abs(greenOffset - cur.greenOffset) > 0.001f ||
                        std::abs(blueOffset - cur.blueOffset) > 0.001f ||
                        std::abs(vignette - cur.vignette) > 0.001f);
        if (tweaked) {
            baseName += "-custom";
        }
    } else if (!currentPresetFile.empty()) {
        baseName = currentPresetFile;
        size_t slash = baseName.find_last_of("/\\");
        if (slash != std::string::npos) baseName = baseName.substr(slash + 1);
        if (baseName.size() > 4 && baseName.substr(baseName.size() - 4) == ".ini") {
            baseName = baseName.substr(0, baseName.size() - 4);
        }
    } else {
        baseName = "CustomPreset";
    }

    EnsureProfilesDirectoryExists();
    std::string candidate = baseName + ".ini";
    std::string fullPath = std::string(PROFILES_PATH) + "/" + candidate;

    if (!ImGui::Detail::FileExists(fullPath)) {
        return candidate;
    }

    for (int i = 2; i <= 99; ++i) {
        std::string cand_i = baseName + "-" + std::to_string(i) + ".ini";
        std::string full_i = std::string(PROFILES_PATH) + "/" + cand_i;
        if (!ImGui::Detail::FileExists(full_i)) {
            return cand_i;
        }
    }
    return candidate;
}

void SettingsImgWindow::buildInterface()
{
    // Auto-select active startup profile on first display:
    static bool s_InitializedActiveProfile = false;
    if (!s_InitializedActiveProfile && !sActiveProfileName.empty()) {
        s_InitializedActiveProfile = true;
        SyncUIStateWithActiveProfile();
    }

    // Keep window background opacity aligned with preferences:
    ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w = sUiOpacity;

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
    // Header: Controls & Live Preview Bar
    // ------------------------------------------------------------------------
    bool ppEnabled = (postProcesssingEnabled != 0);
    static const char* const s_PostProcTooltips[2] = {
        "Post-processing filters are currently **OFF**.\nClick or scroll mouse-wheel up to enable.",
        "Post-processing filters are currently **ON**."
    };

    const float previewRightWidth = 145.0f;
    const float rightColX = ImGui::GetWindowWidth() - previewRightWidth - ImGui::GetStyle().WindowPadding.x;

    // Row 1 Left: Enable toggle
    bool isTemporarilyOverridden = sMomentaryFullView && !postProcesssingEnabled;
    bool ppDisplayState = (postProcesssingEnabled != 0) || isTemporarilyOverridden;
    if (isTemporarilyOverridden) ImGui::BeginDisabled();
    if (ImGui::ToggleButton("Enable Post-Processing", &ppDisplayState, ImVec2(0, 0), s_PostProcTooltips)) {
        SetPostProcessingEnabled(ppDisplayState);
    }
    if (isTemporarilyOverridden) ImGui::EndDisabled();

    // Row 1 Right: When in Full preview mode, "Hold for Full View" is irrelevant,
    // so display the 1-line info attribution link. In Split mode, show the momentary button.
    if (sPreviewMode == PREVIEW_MODE_FULL) {
        float infoWidth = ImGui::CalcTextSize(ICON_FA_INFO_CIRCLE " " NAME " v" VERSION).x + ImGui::GetStyle().FramePadding.x * 2.0f;
        ImGui::SameLine(ImGui::GetWindowWidth() - infoWidth - ImGui::GetStyle().WindowPadding.x);

        ImGui::SmallButtonLink(ICON_FA_INFO_CIRCLE " " NAME " v" VERSION "##about");
        ImGui::TimedTooltip::TextMD(s_AboutTooltipMD);
    } else {
        // Split mode: Momentary "Hold for Full View" button
        ImGui::SameLine(rightColX);

        static XPLMCommandRef s_HoldCmdRef = NULL;
        if (!s_HoldCmdRef) {
            s_HoldCmdRef = XPLMFindCommand("blu_fx/post_processing/momentary");
        }

        bool isButtonActive = sMomentaryFullView;
        if (isButtonActive) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.55f, 0.15f, 1.00f)); // warm amber lit up while active
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.92f, 0.62f, 0.22f, 1.00f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.28f, 0.38f, 0.85f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.26f, 0.36f, 0.48f, 1.00f));
        }
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.85f, 0.55f, 0.15f, 1.00f));

        ImGui::ButtonLink(ICON_FA_EYE " Hold for Full View", ImVec2(previewRightWidth, 0.0f));

        bool isMouseDown = ImGui::IsItemActive();
        static bool s_PrevMouseDown = false;
        if (isMouseDown != s_PrevMouseDown) {
            s_PrevMouseDown = isMouseDown;
            if (s_HoldCmdRef) {
                if (isMouseDown) {
                    XPLMCommandBegin(s_HoldCmdRef);
                } else {
                    XPLMCommandEnd(s_HoldCmdRef);
                }
            } else {
                SetMomentaryFullView(isMouseDown);
            }
        }

        ImGui::PopStyleColor(3);

        // Fast-fading tooltip config for this action button (disappears quickly so it doesn't obstruct the view):
        static ImGui::TimedTooltip::TooltipConfig s_HoldFullTooltipCfg;
        static bool s_HoldFullTooltipCfgInit = false;
        if (!s_HoldFullTooltipCfgInit) {
            s_HoldFullTooltipCfg = ImGui::TimedTooltip::GetCurrentTooltipConfig();
            s_HoldFullTooltipCfg.max_display_cycles = 150;      // ~2.5s at 60fps (snappy auto-hide)
            s_HoldFullTooltipCfg.size_skew_multiplier = 0.0f;   // don't inflate timeout based on text length
            s_HoldFullTooltipCfg.enable_wiggle_latch = false;   // don't restart timer on small mouse jitter
            s_HoldFullTooltipCfgInit = true;
        }

        ImGui::TimedTooltip::TextMD(
            &s_HoldFullTooltipCfg,
            "**Momentary Full View:**\n"
            "Press and hold to view 100%% post-processing across the entire screen.\n\n"
            "You can also add this to a joystick button or keyboard shortcut using:   "
            "<code dark>blu_fx/post_processing/momentary</code>"
        );
    }

    // Row 2: Live Preview Mode & Split Curtain Wipe
    ImGui::Spacing();

    // Row 2 Left: Split Wipe slider OR Fullscreen active text
    if (!ppEnabled) ImGui::BeginDisabled();
    if (sPreviewMode == PREVIEW_MODE_SPLIT) {
        const float gap = ImGui::GetStyle().ItemSpacing.x;
        const float iconW = std::max(ImGui::CalcTextSize(ICON_FA_EYE).x, ImGui::CalcTextSize(ICON_FA_EYE_SLASH).x);
        const float eyeBtnWidth = iconW + ImGui::GetStyle().FramePadding.x * 2.0f + 6.0f; // ~28px, comfortably fits FA eye-slash
        const float eyeX = rightColX - gap - eyeBtnWidth;
        const float sliderTotalWidth = (eyeX - gap) - ImGui::GetCursorPosX();

        ImGui::PushItemWidth(std::max(sliderTotalWidth, 80.0f));
        float splitPercent = sPreviewSplitPosition * 100.0f;
        if (ImGui::SliderWithReset("##wipe_pos", &splitPercent, 50.0f, 5.0f, 95.0f, "Wipe: %.0f%%", 0, 1.0f)) {
            sPreviewSplitPosition = splitPercent / 100.0f;
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            SavePreferences();
        }
        ImGui::PopItemWidth();
        ImGui::TimedTooltip::TextMD(
            "**Curtain Wipe Position:**\n"
            "Adjust where the split screen boundary sits across your monitor (`5%%` to `95%%`).\n"
            "*(Raw native scenery on the left, active color grade on the right)*"
        );

        ImGui::SameLine(eyeX);
        if (ImGui::ButtonLink(sShowSplitDivider ? ICON_FA_EYE "##div" : ICON_FA_EYE_SLASH "##div", ImVec2(eyeBtnWidth, 0.0f))) {
            sShowSplitDivider = !sShowSplitDivider;
            SavePreferences();
        }
        ImGui::TimedTooltip::TextMD(sShowSplitDivider
            ? "Split hairline divider is **ON**.\nClick to hide for direct edge-to-edge pixel comparison."
            : "Split hairline divider is **OFF**.\nClick to show the vertical split boundary.");
    } else {
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled(ICON_FA_DESKTOP " Full-screen live grading active");
        ImGui::TimedTooltip::TextMD(
            "**Full-Screen Preview Active:**\n"
            "Color adjustments are applied across 100%% of your display while the window is open.\n"
            "Switch to **Split** to compare against raw scenery side by side."
        );
    }
    if (!ppEnabled) ImGui::EndDisabled();

    // Row 2 Right: [ Full | Split ] segmented buttons (Always enabled so users can configure preview mode)
    ImGui::SameLine(rightColX);
    const float halfBtnW = (previewRightWidth - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

    // Full button:
    bool isFull = (sPreviewMode == PREVIEW_MODE_FULL);
    if (isFull) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.45f, 0.70f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.52f, 0.80f, 1.00f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.22f, 0.28f, 0.70f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.30f, 0.38f, 0.85f));
    }
    if (ImGui::ButtonLink(ICON_FA_EXPAND " Full", ImVec2(halfBtnW, 0.0f))) {
        sPreviewMode = PREVIEW_MODE_FULL;
        SavePreferences();
    }
    ImGui::PopStyleColor(2);
    ImGui::TimedTooltip::TextMD("Apply color adjustments across 100%% of the screen while tuning.");

    ImGui::SameLine();

    // Split button:
    bool isSplit = (sPreviewMode == PREVIEW_MODE_SPLIT);
    if (isSplit) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.45f, 0.70f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.52f, 0.80f, 1.00f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.22f, 0.28f, 0.70f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.30f, 0.38f, 0.85f));
    }
    if (ImGui::ButtonLink(ICON_FA_COLUMNS " Split", ImVec2(halfBtnW, 0.0f))) {
        sPreviewMode = PREVIEW_MODE_SPLIT;
        SavePreferences();
    }
    ImGui::PopStyleColor(2);
    ImGui::TimedTooltip::TextMD("Split screen: raw scenery on left, active grade on right.");

    ImGui::Separator();

    // ------------------------------------------------------------------------
    // Section 1: Post-Processing Sliders (Mouse-Wheel + 1-Click Reset/Undo)
    // ------------------------------------------------------------------------
    if (IM_BOLD_FONT) ImGui::PushFont(IM_BOLD_FONT);
    ImGui::TextUnformatted("Post-Processing Adjustments");
    if (IM_BOLD_FONT) ImGui::PopFont();
    ImGui::TimedTooltip::TextMD(
        "**Tip:** Hover over any slider track and **spin your mouse wheel** for fine 0.01 step adjustments, "
        "or click the **Reset (↺)** button next to any slider to toggle between its default and your custom value!"
    );

    const BLUfxPreset& def = BLUfxPresets[PRESET_DEFAULT];

    // Check if sliders match any factory preset exactly (only if not in a user profile):
    int exactCuratedPresetIdx = -1;
    for (int i = PRESET_POLAROID; i < PRESET_MAX; ++i) {
        if (isActivePreset(&BLUfxPresets[i])) {
            exactCuratedPresetIdx = i;
            break;
        }
    }
    if (s_SelectedPresetFilename.empty()) {
        if (exactCuratedPresetIdx != -1 && s_ActiveCuratedPresetIdx != exactCuratedPresetIdx) {
            s_ActiveCuratedPresetIdx = exactCuratedPresetIdx;
        }
    }

    bool isCleanCuratedPreset = false;
    std::string currentPresetRef;
    if (s_ActiveCuratedPresetIdx >= PRESET_POLAROID && s_ActiveCuratedPresetIdx < PRESET_MAX &&
        isActivePreset(&BLUfxPresets[s_ActiveCuratedPresetIdx])) {
        isCleanCuratedPreset = true;
        currentPresetRef = "preset:" + std::string(BLUfxPresetNames[s_ActiveCuratedPresetIdx]);
    }

#if BLUFX_SIDE_BY_SIDE_LAYOUT
    // ========================================================================
    // Option B: Sliders on left (widened), action buttons flush-right (split pairs)
    // ========================================================================
    const float rightColWidth = 125.0f;
    const float leftColWidth  = ImGui::GetContentRegionAvail().x - rightColWidth - 16.0f;

    ImGui::BeginGroup(); // Left column: Post-processing adjustments
    const float topSliderY = ImGui::GetCursorPosY();

#if BLUFX_EXPERIMENTAL_KNOB_UI
    // ------------------------------------------------------------------------
    // Experimental Rotary Dials: Grouped into Triads (Tone, RGB Scale, RGB Offset)
    // ------------------------------------------------------------------------
    constexpr float kKnobRadius = 11.0f; // Compact diameter 22px
    constexpr ImGuiKnobFlags kDialFlags = ImGuiKnobFlags_ShowTicks;

    auto DrawDialCell = [&](const char* label, float* p_val, float def_val, float v_min, float v_max, const char* tooltip, const ImVec4* tint_col = nullptr) {
        ImGui::TextUnformatted(label);
        if (tint_col) {
            ImGui::PushStyleColor(ImGuiCol_SliderGrab, *tint_col);
            ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(tint_col->x * 1.15f, tint_col->y * 1.15f, tint_col->z * 1.15f, 1.0f));
        }
        std::string knob_id = std::string("##dial_") + label;
        ImGui::KnobWithReset(knob_id.c_str(), p_val, def_val, v_min, v_max, "%.2f", kKnobRadius, 0.01f, nullptr, kDialFlags);
        if (tint_col) {
            ImGui::PopStyleColor(2);
        }
        if (tooltip && tooltip[0]) {
            ImGui::TimedTooltip::TextMD(tooltip);
        }
    };

    const ImVec4 redTint(0.92f, 0.32f, 0.32f, 1.0f);
    const ImVec4 greenTint(0.32f, 0.85f, 0.40f, 1.0f);
    const ImVec4 blueTint(0.35f, 0.65f, 0.98f, 1.0f);

    // Triad 1: Tone & Dynamics (Brightness, Contrast, Saturation)
    if (ImGui::BeginTable("##triad_tone", 3, ImGuiTableFlags_SizingStretchSame, ImVec2(leftColWidth, 0.0f))) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        DrawDialCell("Brightness", &brightness, def.brightness, -0.50f, 0.50f, "Adjusts overall scene luminance offset (`-0.50` to `+0.50`).");

        ImGui::TableSetColumnIndex(1);
        DrawDialCell("Contrast", &contrast, def.contrast, 0.05f, 2.00f, "Adjusts dynamic range separation (`0.05` to `2.00`, default `1.00`).");

        ImGui::TableSetColumnIndex(2);
        DrawDialCell("Saturation", &saturation, def.saturation, 0.00f, 2.50f, "Adjusts color intensity (`0.00` = mono, `1.00` = normal, up to `2.50`).");
        ImGui::EndTable();
    }

    ImGui::Spacing();

    // Triad 2: RGB Scale (Mid-tones)
    if (ImGui::BeginTable("##triad_rgb_scale", 3, ImGuiTableFlags_SizingStretchSame, ImVec2(leftColWidth, 0.0f))) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        DrawDialCell("Red Scale", &redScale, def.redScale, -0.75f, 0.75f, "Non-linear mid-tone curve boost/cut for **Red** (`-0.75` to `+0.75`).", &redTint);

        ImGui::TableSetColumnIndex(1);
        DrawDialCell("Green Scale", &greenScale, def.greenScale, -0.75f, 0.75f, "Non-linear mid-tone curve boost/cut for **Green** (`-0.75` to `+0.75`).", &greenTint);

        ImGui::TableSetColumnIndex(2);
        DrawDialCell("Blue Scale", &blueScale, def.blueScale, -0.75f, 0.75f, "Non-linear mid-tone curve boost/cut for **Blue** (`-0.75` to `+0.75`).", &blueTint);
        ImGui::EndTable();
    }

    ImGui::Spacing();

    // Triad 3: RGB Offset (Shadows / Additive)
    if (ImGui::BeginTable("##triad_rgb_offset", 3, ImGuiTableFlags_SizingStretchSame, ImVec2(leftColWidth, 0.0f))) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        DrawDialCell("Red Offset", &redOffset, def.redOffset, -0.50f, 0.50f, "Linear additive shift for **Red** (`-0.50` to `+0.50`).", &redTint);

        ImGui::TableSetColumnIndex(1);
        DrawDialCell("Green Offset", &greenOffset, def.greenOffset, -0.50f, 0.50f, "Linear additive shift for **Green** (`-0.50` to `+0.50`).", &greenTint);

        ImGui::TableSetColumnIndex(2);
        DrawDialCell("Blue Offset", &blueOffset, def.blueOffset, -0.50f, 0.50f, "Linear additive shift for **Blue** (`-0.50` to `+0.50`).", &blueTint);
        ImGui::EndTable();
    }

    ImGui::Spacing();

    // Row 4: Optics (Vignette)
    const float bottomSliderY = ImGui::GetCursorPosY();
    DrawDialCell("Vignette", &vignette, def.vignette, 0.00f, 1.00f, "Darkens the outer edges of the screen (`0.00` = off, `1.00` = max).");

#else
    // ------------------------------------------------------------------------
    // Classic Post-Processing Sliders (SliderWithReset)
    // ------------------------------------------------------------------------
    const float sliderWidth = leftColWidth - 88.0f;
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

    const float bottomSliderY = ImGui::GetCursorPosY();
    ImGui::SliderWithReset("Vignette", &vignette, def.vignette, 0.00f, 1.00f, "%.2f", 0, 0.01f);
    ImGui::TimedTooltip::TextMD("Darkens the outer edges of the screen (`0.00` = off, `1.00` = maximum vignette).");

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(1);
    ImGui::PopItemWidth();
#endif
    const float leftColBottomY = ImGui::GetCursorPosY();
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
        s_SelectedPresetFilename.clear();
        s_ActiveCuratedPresetIdx = -1;
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
            RestoreSessionStartup();
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
            RestoreSessionStartup();
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
            RestoreSessionStartup();
            ImGui::TimedTooltip::ClearActiveTooltip();
        }
        if (ImGui::MenuItem(ICON_FA_UNDO " Recover Last Discarded Edits", nullptr, false, hasDiscarded)) {
            RestoreDiscardedPreset();
            s_SelectedPresetFilename.clear();
            s_ActiveCuratedPresetIdx = -1;
            ImGui::TimedTooltip::ClearActiveTooltip();
        }
        ImGui::EndPopup();
    }

    // --------------------------------------------------------------------
    // Middle Section: User Profiles (Profile Library in profiles/)
    // --------------------------------------------------------------------
    ImGui::SetCursorPosY(topSliderY + (btnHeight + VG) * 2 + 6.0f);
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextDisabled("USER PROFILES");
    if (HasUnsavedChanges()) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.95f, 0.70f, 0.20f, 1.00f), "*");
        ImGui::TimedTooltip::TextMD("You have unsaved slider adjustments in memory.");
    }

    ImGui::SetNextItemWidth(actionBtnSize.x);
    if (ImGui::FilePickerCombo("##user_presets", &s_SelectedPresetFilename, PROFILES_PATH, ".ini")) {
        if (!s_SelectedPresetFilename.empty()) {
            std::string fullPath = std::string(PROFILES_PATH) + "/" + s_SelectedPresetFilename;
            LoadSettingsFromFile(fullPath);
            sActiveProfileName = s_SelectedPresetFilename;
            s_ActiveCuratedPresetIdx = -1;
            ClearPrePresetScratchpad();
            UpdateDiskSnapshot();
            ImGui::TimedTooltip::ClearActiveTooltip();
        }
    }
    ImGui::TimedTooltip::TextMD(s_SelectedPresetFilename.empty()
        ? "Select a saved user profile from `" PROFILES_PATH_DISPLAY "`."
        : ("Active profile: `" + s_SelectedPresetFilename + "`\nClick dropdown to switch profiles.").c_str());

    // 1. Save (saves to currently active profile in profiles/):
    bool hasActiveProfile = !s_SelectedPresetFilename.empty();
    if (!hasActiveProfile) ImGui::BeginDisabled();
    if (ImGui::ButtonLink(ICON_FA_SAVE " Save", actionBtnSize)) {
        std::string fullPath = std::string(PROFILES_PATH) + "/" + s_SelectedPresetFilename;
        SaveSettingsToFile(fullPath);
        sActiveProfileName = s_SelectedPresetFilename;
        s_ActiveCuratedPresetIdx = -1;
        UpdateDiskSnapshot();
        ImGui::TimedTooltip::ClearActiveTooltip();
    }
    if (!hasActiveProfile) ImGui::EndDisabled();
    ImGui::TimedTooltip::TextMDProxy(
        ImGui::GetItemID(),
        ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled),
        hasActiveProfile ? ("Save adjustments into `" + s_SelectedPresetFilename + "`.").c_str()
                         : "No user profile is currently selected. Use 'Save As...' to create one."
    );

    // 2. Save As... (saves current adjustments as a new named profile in profiles/):
    if (ImGui::ButtonLink(ICON_FA_PLUS " Save As...", actionBtnSize)) {
        s_DefaultSaveName = GetSmartPresetDefaultFilename(s_ActiveCuratedPresetIdx, s_SelectedPresetFilename);
        s_SaveModalOpen = true;
        s_CloseAfterSaveAs = false;
        ImGui::TimedTooltip::ClearActiveTooltip();
    }
    ImGui::TimedTooltip::TextMD("Save current adjustments as a new named profile in `" PROFILES_PATH_DISPLAY "` (does not alter your active preferred profile).");

    // --------------------------------------------------------------------
    // Bottom Section: Startup Configuration (X-Plane Boot Default via .prf)
    // --------------------------------------------------------------------
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextDisabled("STARTUP DEFAULT");

    std::string startupBadge;
    if (sStartupActiveProfile.rfind("preset:", 0) == 0) {
        startupBadge = sStartupActiveProfile.substr(7) + " (preset)";
    } else if (!sStartupActiveProfile.empty()) {
        startupBadge = sStartupActiveProfile;
    } else {
        startupBadge = "(None)";
    }

    bool isAlreadyStartup = false;
    std::string startupTooltip;

    if (isCleanCuratedPreset) {
        isAlreadyStartup = (sStartupActiveProfile == currentPresetRef);
        if (isAlreadyStartup) {
            startupTooltip = "<badge cyan>" + std::string(BLUfxPresetNames[s_ActiveCuratedPresetIdx]) + "</badge> is already your configured boot default.";
        } else {
            startupTooltip = "Set <badge cyan>" + std::string(BLUfxPresetNames[s_ActiveCuratedPresetIdx]) + "</badge> as your startup preset on X-Plane boot.";
        }
    } else if (!s_SelectedPresetFilename.empty()) {
        isAlreadyStartup = (s_SelectedPresetFilename == sStartupActiveProfile && !HasUnsavedChanges());
        if (isAlreadyStartup) {
            startupTooltip = "<badge emerald>" + s_SelectedPresetFilename + "</badge> is already your configured boot default.";
        } else if (HasUnsavedChanges()) {
            startupTooltip = "Save adjustments into <badge emerald>" + s_SelectedPresetFilename + "</badge> and set as your boot default.";
        } else {
            startupTooltip = "Set <badge emerald>" + s_SelectedPresetFilename + "</badge> as your permanent default when X-Plane boots.";
        }
    } else {
        // Dirty / customized look from scratchpad or tweaked preset:
        isAlreadyStartup = false;
        if (s_ActiveCuratedPresetIdx >= PRESET_POLAROID && s_ActiveCuratedPresetIdx < PRESET_MAX) {
            startupTooltip = "Save adjustments derived from <badge cyan>" + std::string(BLUfxPresetNames[s_ActiveCuratedPresetIdx]) + "</badge> as a new profile and set as your permanent boot default.";
        } else {
            startupTooltip = "Save current customized look as a new profile and set as your permanent boot default.";
        }
    }

    if (isAlreadyStartup) {
        ImGui::TextColored(ImVec4(0.35f, 0.80f, 0.45f, 1.00f), ICON_FA_CHECK " %s", startupBadge.c_str());
    } else {
        ImGui::TextColored(ImVec4(0.70f, 0.75f, 0.82f, 1.00f), ICON_FA_STAR " %s", startupBadge.c_str());
    }
    ImGui::TimedTooltip::TextMD(
        "Configured default profile loaded when X-Plane boots."
    );

    const float naturalLoadY = ImGui::GetCursorPosY() + VG;
    const float alignedLoadY = bottomSliderY - btnHeight - VG;
    ImGui::SetCursorPosY(std::max(naturalLoadY, alignedLoadY));

    // Distinct forest/emerald green accent for Set as Startup:
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.52f, 0.30f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.62f, 0.36f, 1.00f));
    if (isAlreadyStartup) ImGui::BeginDisabled();
    if (ImGui::ButtonLink(ICON_FA_STAR " Set as Startup", actionBtnSize)) {
        if (isCleanCuratedPreset) {
            // Clean factory preset: store lightweight preset reference without redundant disk files
            sStartupActiveProfile = currentPresetRef;
            sActiveProfileName = currentPresetRef;
            SavePreferences();
            if (sMirrorLegacyIni) SyncLegacyIniMirror();
            UpdateDiskSnapshot();
        } else if (!s_SelectedPresetFilename.empty()) {
            // Existing user profile: save to disk and update preferences
            std::string fullPath = std::string(PROFILES_PATH) + "/" + s_SelectedPresetFilename;
            SaveSettingsToFile(fullPath);
            sActiveProfileName = s_SelectedPresetFilename;
            sStartupActiveProfile = s_SelectedPresetFilename;
            s_ActiveCuratedPresetIdx = -1;
            SavePreferences();
            UpdateDiskSnapshot();
        } else {
            // Dirty / customized look without a profile file: prompt Save As... to name the slot
            s_DefaultSaveName = GetSmartPresetDefaultFilename(s_ActiveCuratedPresetIdx, s_SelectedPresetFilename);
            s_SaveModalOpen = true;
            s_SetAsStartupAfterSave = true;
            s_CloseAfterSaveAs = false;
        }
        ImGui::TimedTooltip::ClearActiveTooltip();
    }
    if (isAlreadyStartup) ImGui::EndDisabled();
    ImGui::PopStyleColor(2);
    ImGui::TimedTooltip::TextMDProxy(
        ImGui::GetItemID(),
        ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled),
        startupTooltip.c_str()
    );

    const float naturalSaveY = ImGui::GetCursorPosY() + VG;
    const float alignedSaveY = bottomSliderY;
    ImGui::SetCursorPosY(std::max(naturalSaveY, alignedSaveY));

    // Distinct soft steel/slate blue accent for Reload Startup:
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.42f, 0.60f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.50f, 0.72f, 1.00f));
    if (ImGui::ButtonLink(ICON_FA_SYNC " Reload Startup", actionBtnSize)) {
        LoadSettings();
        SyncUIStateWithActiveProfile();
        UpdateDiskSnapshot();
        ImGui::TimedTooltip::ClearActiveTooltip();
    }
    ImGui::PopStyleColor(2);
    if (!IsWindowMoving() && (ImGui::IsItemClicked(ImGuiMouseButton_Right) ||
        (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)))) {
        s_OpenModalOpen = true;
    }

    std::string reloadTooltip;
    if (sStartupActiveProfile.rfind("preset:", 0) == 0) {
        reloadTooltip = "Reload your configured startup preset (<badge cyan>" + sStartupActiveProfile.substr(7) + "</badge>), discarding session tweaks.\n\n*(Tip: Right-click to browse and open any `.ini` file)*";
    } else if (!sStartupActiveProfile.empty()) {
        reloadTooltip = "Reload your configured startup profile (<badge emerald>" + sStartupActiveProfile + "</badge>), discarding session tweaks.\n\n*(Tip: Right-click to browse and open any `.ini` file)*";
    } else {
        reloadTooltip = "Reload your default neutral startup settings, discarding session tweaks.\n\n*(Tip: Right-click to browse and open any `.ini` file)*";
    }
    ImGui::TimedTooltip::TextMD(reloadTooltip.c_str());

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
        s_SelectedPresetFilename.clear();
        s_ActiveCuratedPresetIdx = -1;
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
        RestoreSessionStartup();
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
            RestoreSessionStartup();
            ImGui::TimedTooltip::ClearActiveTooltip();
        }
        if (ImGui::MenuItem(ICON_FA_UNDO " Recover Last Discarded Edits", nullptr, false, hasDiscardedOptA)) {
            RestoreDiscardedPreset();
            s_SelectedPresetFilename.clear();
            s_ActiveCuratedPresetIdx = -1;
            ImGui::TimedTooltip::ClearActiveTooltip();
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 155.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.42f, 0.60f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.50f, 0.72f, 1.00f));
    if (ImGui::ButtonLink(ICON_FA_FOLDER_OPEN " Load .ini")) {
        LoadSettings();
        SyncUIStateWithActiveProfile();
        UpdateDiskSnapshot();
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

                bool isBasePreset = (s_ActiveCuratedPresetIdx == pIdx);
                bool isExactMatch = isBasePreset && isActivePreset(&BLUfxPresets[pIdx]);

                if (isExactMatch) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.52f, 0.88f, 0.95f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.26f, 0.59f, 0.98f, 1.00f));
                } else if (isBasePreset) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.38f, 0.52f, 0.90f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.46f, 0.62f, 1.00f));
                }

                char btnLabel[64];
                if (isExactMatch) {
                    std::snprintf(btnLabel, sizeof(btnLabel), ICON_FA_CHECK " %s##preset_%d", BLUfxPresetNames[pIdx], pIdx);
                } else if (isBasePreset) {
                    std::snprintf(btnLabel, sizeof(btnLabel), ICON_FA_PEN " %s*##preset_%d", BLUfxPresetNames[pIdx], pIdx);
                } else {
                    std::snprintf(btnLabel, sizeof(btnLabel), "%s##preset_%d", BLUfxPresetNames[pIdx], pIdx);
                }

                if (ImGui::ButtonLink(btnLabel, ImVec2(-FLT_MIN, 0.0f))) {
                    ImGui::TimedTooltip::ClearActiveTooltip();
                    if (isBasePreset) {
                        // Toggled off! Restore pre-preset scratchpad or startup baseline:
                        if (HasPrePresetScratchpad()) {
                            RestorePrePresetScratchpad();
                            s_SelectedPresetFilename = s_PrePresetFilename;
                            s_ActiveCuratedPresetIdx = s_PrePresetActiveCuratedIdx;
                            if (!s_PrePresetFilename.empty()) {
                                sActiveProfileName = s_PrePresetFilename;
                            }
                            UpdateDiskSnapshot();
                        } else {
                            RestoreSessionStartup();
                        }
                    } else {
                        // Entering presets: if coming from a non-curated state, remember it:
                        if (s_ActiveCuratedPresetIdx == -1) {
                            SavePrePresetScratchpad();
                            s_PrePresetFilename = s_SelectedPresetFilename;
                            s_PrePresetActiveCuratedIdx = -1;
                        }
                        s_SelectedPresetFilename.clear();
                        s_ActiveCuratedPresetIdx = pIdx;
                        ApplyPreset(pIdx);
                    }
                }

                if (isExactMatch) {
                    ImGui::TimedTooltip::TextMD(
                        "Active preset: **%s**\n%s\n\n*(Click again to toggle off and revert to previous settings)*",
                        BLUfxPresetNames[pIdx], BLUfxPresetDescriptions[pIdx]);
                } else if (isBasePreset) {
                    ImGui::TimedTooltip::TextMD(
                        "Base preset: **%s (tweaked)**\n%s\n\n*(Click to toggle off and revert to previous settings)*",
                        BLUfxPresetNames[pIdx], BLUfxPresetDescriptions[pIdx]);
                } else {
                    ImGui::TimedTooltip::TextMD(
                        "**%s**\n%s\n\n*(Click to apply this preset)*",
                        BLUfxPresetNames[pIdx], BLUfxPresetDescriptions[pIdx]);
                }

                if (isExactMatch || isBasePreset) {
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
    // Compatibility & Downgrade Safety Option (Compact Muted Footer)
    // ------------------------------------------------------------------------
    ImGui::Separator();
    if (IM_SMALLER_FONT) ImGui::PushFont(IM_SMALLER_FONT);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.58f, 0.64f, 0.72f, 0.75f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.40f, 0.48f, 0.58f, 0.68f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.19f, 0.25f, 0.72f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.22f, 0.28f, 0.36f, 0.80f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

    bool disableMirror = (sMirrorLegacyIni == 0);
    if (ImGui::Checkbox("I will never return to v1; remove backwards-compatibility hacks.", &disableMirror)) {
        sMirrorLegacyIni = disableMirror ? 0 : 1;
        SavePreferences();
        if (sMirrorLegacyIni) {
            SyncLegacyIniMirror();
        } else {
            std::remove(CONFIG_PATH);
        }
    }

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);

    // TimedTooltip invoked for checkbox:
    ImGui::TimedTooltip::TextMD(disableMirror
        ? "**Legacy Downgrade Compatibility: DISABLED**\n"
          "BLU-fx will no longer update `blu_fx.ini` in the plugin root.\n\n"
          "*(Unchecking this box will re-enable automatic v1.x downgrade protection.)*"
        : "**Legacy Downgrade Compatibility: ACTIVE**\n"
          "BLU-fx automatically mirrors active color grading into `blu_fx.ini`.\n\n"
          "If you roll back to v1.x (via SkunkCrafts Updater or manual archive), "
          "v1.x will seamlessly pick up your latest color grading adjustments.\n\n"
          "*(Check this box only if you will never return to v1.x.)*"
    );

    float cbEndX = ImGui::GetItemRectMax().x;

    // Right-aligned User Guide Window toggle button:
    bool docOpen = DocViewerImgWindow::IsOpen();
    const char* docLabel = docOpen ? (ICON_FA_BOOK_OPEN " Close User Guide") : (ICON_FA_BOOK " User Guide Window");
    float docBtnW = ImGui::CalcTextSize(docLabel).x + ImGui::GetStyle().FramePadding.x * 2.0f;
    float rightX = ImGui::GetWindowWidth() - docBtnW - ImGui::GetStyle().WindowPadding.x;

    if (rightX > cbEndX + 12.0f) {
        ImGui::SameLine(rightX);
    } else {
        ImGui::NewLine();
        ImGui::SameLine(std::max(0.0f, rightX));
    }

    ImVec4 linkCol = docOpen ? ImVec4(0.55f, 0.82f, 1.00f, 0.95f) : ImVec4(0.78f, 0.85f, 0.95f, 0.90f);
    ImGui::PushStyleColor(ImGuiCol_Text, linkCol);
    if (ImGui::SmallButtonLink(docLabel)) {
        DocViewerImgWindow::ToggleInstance();
    }
    ImGui::PopStyleColor();

    ImGui::TimedTooltip::TextMD(docOpen
        ? "**Close User Guide Window:**\n\n"
          "The illustrated User Guide is currently open in X-Plane. Click here to close it."
        : "**BLU-fx User Guide Window:**\n\n"
          "Opens the interactive, illustrated user manual right here inside X-Plane as a window -- "
          "no need to leave the simulator or open an external PDF viewer!"
    );

    if (IM_SMALLER_FONT) ImGui::PopFont();

    // ------------------------------------------------------------------------
    // Legacy XP11 Sections (Raleigh Scale, FPS-Limiter, Cinema Verite)
    // ------------------------------------------------------------------------
    if (LEGACY_FEATURES) {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.10f, 0.14f, 0.22f, 0.85f));
        const float legacySliderWidth = ImGui::GetContentRegionAvail().x * 0.60f;

        ImGui::Separator();
        if (IM_BOLD_FONT) ImGui::PushFont(IM_BOLD_FONT);
        ImGui::TextUnformatted("Raleigh Scale (X-Plane 11)");
        if (IM_BOLD_FONT) ImGui::PopFont();
        ImGui::PushItemWidth(legacySliderWidth);
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
            SavePreferences();
        }
        ImGui::TimedTooltip::TextMD("Limits X-Plane's maximum frame rate to smooth out frame-time spikes.");

        ImGui::PushItemWidth(legacySliderWidth);
        if (ImGui::SliderWithReset("Max FPS", &maxFps, DEFAULT_MAX_FRAME_RATE, 20.0f, 200.0f, "%.0f fps", 0, 1.0f)) {
            SavePreferences();
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            SavePreferences();
        }
        ImGui::TimedTooltip::TextMD("Target maximum frame rate when FPS-Limiter is enabled (`20` to `200` FPS).");
        ImGui::PopItemWidth();

        ImGui::Separator();
        if (IM_BOLD_FONT) ImGui::PushFont(IM_BOLD_FONT);
        ImGui::TextUnformatted("Auto Disable / Enable Cinema Verite");
        if (IM_BOLD_FONT) ImGui::PopFont();

        bool cvEnabled = (controlCinemaVeriteEnabled != 0);
        if (ImGui::ToggleButton("Control Cinema Verite", &cvEnabled)) {
            SetControlCinemaVeriteEnabled(cvEnabled);
            SavePreferences();
        }
        ImGui::TimedTooltip::TextMD("Automatically pauses Cinema Verite camera shake in the 3D cockpit while you move the mouse.");

        ImGui::PushItemWidth(legacySliderWidth);
        if (ImGui::SliderWithReset("On input disable for", &disableCinemaVeriteTime, DEFAULT_DISABLE_CINEMA_VERITE_TIME, 1.0f, 30.0f, "%.0f sec", 0, 1.0f)) {
            SavePreferences();
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            SavePreferences();
        }
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
    int extraBottomPad = LEGACY_FEATURES ? (int)(ImGui::GetTextLineHeightWithSpacing() + 4.0f) : 2;
    int targetH = (int)(ImGui::GetCursorPosY()) + extraBottomPad;

    // Defensive bounds guard: height must be realistic (min 350px, max 1200px)
    constexpr int minValidH = 350;
    constexpr int maxValidH = 1200;
    if (targetH >= minValidH && targetH <= maxValidH && std::abs(currentH - targetH) > 1 && !IsPoppedOut()) {
        auto geom = WindowGeometry::FromTopLeft(left, top, right - left, targetH);
        SetWindowGeometry(geom.left, geom.top, geom.right, geom.bottom);
    }

    // Continuously remember window position in-memory and update preferences:
    if (!IsPoppedOut() && (right - left) > 0 && (top - bottom) > 0) {
        if (sLastInSimLeft != left || sLastInSimTop != top) {
            sLastInSimLeft = left;
            sLastInSimTop  = top;
            sHasSavedPosition = true;
            sWindowLeft = left;
            sWindowTop  = top;
        }
    }

    // ------------------------------------------------------------------------
    // Unsaved Changes Confirmation Modal Dialog
    // ------------------------------------------------------------------------
    if (mShowCloseConfirmModal) {
        ImGui::OpenPopup("Unsaved Changes##close_confirm");
    }

    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(410.0f, 0.0f), ImVec2(520.0f, 600.0f));

    if (ImGui::BeginPopupModal("Unsaved Changes##close_confirm", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (IM_BOLD_FONT) ImGui::PushFont(IM_BOLD_FONT);
        ImGui::TextUnformatted(ICON_FA_EXCLAMATION_TRIANGLE "  Save Adjustments?");
        if (IM_BOLD_FONT) ImGui::PopFont();

        ImGui::Spacing();
        bool hasActiveProfile = !s_SelectedPresetFilename.empty();
        if (hasActiveProfile) {
            ImGui::TextUnformatted("You have unsaved adjustments to profile:");
            ImGui::TextColored(ImVec4(0.40f, 0.80f, 1.00f, 1.00f), "  %s", s_SelectedPresetFilename.c_str());
        } else if (isCleanCuratedPreset) {
            ImGui::TextUnformatted("You are auditioning factory preset:");
            ImGui::TextColored(ImVec4(0.40f, 0.80f, 1.00f, 1.00f), "  %s", BLUfxPresetNames[s_ActiveCuratedPresetIdx]);
        } else if (s_ActiveCuratedPresetIdx >= PRESET_POLAROID && s_ActiveCuratedPresetIdx < PRESET_MAX) {
            ImGui::TextUnformatted("You have unsaved adjustments derived from preset:");
            ImGui::TextColored(ImVec4(0.40f, 0.80f, 1.00f, 1.00f), "  %s", BLUfxPresetNames[s_ActiveCuratedPresetIdx]);
        } else {
            ImGui::TextUnformatted("You have unsaved adjustments to your session scratchpad.");
        }
        ImGui::TextUnformatted("What would you like to do before closing?");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        const float halfBtnW = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

        // Row 1: Save Actions (50/50 split)
        // Left: Save & Close (saves to active profile, closes)
        if (hasActiveProfile) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.52f, 0.30f, 0.85f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.62f, 0.36f, 1.00f));
            std::string saveLabel = ICON_FA_SAVE " Save & Close";
            if (ImGui::ButtonLink(saveLabel.c_str(), ImVec2(halfBtnW, 0.0f))) {
                std::string fullPath = std::string(PROFILES_PATH) + "/" + s_SelectedPresetFilename;
                SaveSettingsToFile(fullPath);
                sActiveProfileName = s_SelectedPresetFilename;
                UpdateDiskSnapshot();
                mShowCloseConfirmModal = false;
                ImGui::CloseCurrentPopup();
                SetVisible(false);
                UpdateMenuCheckmarks();
            }
            ImGui::PopStyleColor(2);
            ImGui::TimedTooltip::TextMD(("Save current adjustments into <badge emerald>" + s_SelectedPresetFilename + "</badge> and close.").c_str());

            ImGui::SameLine();
        } else if (isCleanCuratedPreset) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.52f, 0.30f, 0.85f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.62f, 0.36f, 1.00f));
            std::string startupLabel = ICON_FA_STAR " Set Startup & Close";
            if (ImGui::ButtonLink(startupLabel.c_str(), ImVec2(halfBtnW, 0.0f))) {
                sStartupActiveProfile = currentPresetRef;
                sActiveProfileName = currentPresetRef;
                SavePreferences();
                if (sMirrorLegacyIni) SyncLegacyIniMirror();
                UpdateDiskSnapshot();
                mShowCloseConfirmModal = false;
                ImGui::CloseCurrentPopup();
                SetVisible(false);
                UpdateMenuCheckmarks();
            }
            ImGui::PopStyleColor(2);
            ImGui::TimedTooltip::TextMD(("Set <badge cyan>" + std::string(BLUfxPresetNames[s_ActiveCuratedPresetIdx]) + "</badge> as your permanent startup preset and close.").c_str());

            ImGui::SameLine();
        }

        // Right (or full-width if neither active profile nor clean preset): Save As... (saves to new file without altering startup default)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.42f, 0.60f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.50f, 0.72f, 1.00f));
        const bool hasLeftBtn = hasActiveProfile || isCleanCuratedPreset;
        const ImVec2 saveAsBtnSz = hasLeftBtn ? ImVec2(halfBtnW, 0.0f) : ImVec2(-FLT_MIN, 0.0f);
        if (ImGui::ButtonLink(ICON_FA_PLUS " Save As...", saveAsBtnSz)) {
            s_DefaultSaveName = GetSmartPresetDefaultFilename(s_ActiveCuratedPresetIdx, s_SelectedPresetFilename);
            s_SaveModalOpen = true;
            s_CloseAfterSaveAs = true;
            mShowCloseConfirmModal = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(2);
        ImGui::TimedTooltip::TextMD("Save adjustments to a new named profile without changing your startup default, then close.");

        ImGui::Spacing();

        // Row 2: Non-saving Exits (50/50 split)
        // Left: Keep Tweaks & Close (leaves live adjustments active in memory, touches no files on disk, closes window)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.35f, 0.50f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.42f, 0.60f, 1.00f));
        if (ImGui::ButtonLink(ICON_FA_EYE " Keep Tweaks & Close", ImVec2(halfBtnW, 0.0f))) {
            mShowCloseConfirmModal = false;
            ImGui::CloseCurrentPopup();
            SetVisible(false);
        }
        ImGui::PopStyleColor(2);
        ImGui::TimedTooltip::TextMD("Keep your adjustments active on screen in memory without saving to any file on disk.");

        ImGui::SameLine();

        // Right: Revert & Close (reverts to disk baseline, closes window)
        if (ImGui::ButtonLink(ICON_FA_TRASH_ALT " Revert & Close", ImVec2(halfBtnW, 0.0f))) {
            CaptureDiscardedPreset();
            LoadSettings();
            SyncUIStateWithActiveProfile();
            UpdateDiskSnapshot();
            mShowCloseConfirmModal = false;
            ImGui::CloseCurrentPopup();
            SetVisible(false);
        }
        ImGui::TimedTooltip::TextMD("Revert adjustments to the saved disk baseline and close.");

        ImGui::Spacing();

        // Row 3: Stay in Window
        if (ImGui::ButtonLink(ICON_FA_UNDO " Cancel", ImVec2(-FLT_MIN, 0.0f))) {
            mShowCloseConfirmModal = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::TimedTooltip::TextMD("Stay in the settings window without closing.");

        ImGui::EndPopup();
    }

    // ------------------------------------------------------------------------
    // In-Cockpit File Dialog Modals (Save Preset As... & Open Preset File)
    // ------------------------------------------------------------------------
    if (s_SaveModalOpen) {
        if (ImGui::SaveFileDialog("Save Color Profile", &s_SaveModalOpen, &s_SavedFilePath, PROFILES_PATH, s_DefaultSaveName.c_str(), ".ini")) {
            SaveSettingsToFile(s_SavedFilePath);
            size_t slash = s_SavedFilePath.find_last_of("/\\");
            std::string filename = (slash != std::string::npos) ? s_SavedFilePath.substr(slash + 1) : s_SavedFilePath;

            s_SelectedPresetFilename = filename;
            sActiveProfileName = filename;
            s_ActiveCuratedPresetIdx = -1;

            if (s_SetAsStartupAfterSave) {
                s_SetAsStartupAfterSave = false;
                sStartupActiveProfile = filename;
                SavePreferences();
            }
            UpdateDiskSnapshot();

            if (s_CloseAfterSaveAs) {
                s_CloseAfterSaveAs = false;
                SetVisible(false);
            }
        }
        if (!s_SaveModalOpen) {
            s_CloseAfterSaveAs = false;
            s_SetAsStartupAfterSave = false;
        }
    }

    if (s_OpenModalOpen) {
        if (ImGui::OpenFileDialog("Open Preset File", &s_OpenModalOpen, &s_OpenFilePath, PROFILES_PATH, ".ini")) {
            LoadSettingsFromFile(s_OpenFilePath);
            size_t slash = s_OpenFilePath.find_last_of("/\\");
            s_SelectedPresetFilename = (slash != std::string::npos) ? s_OpenFilePath.substr(slash + 1) : s_OpenFilePath;
            sActiveProfileName = s_SelectedPresetFilename;
            s_ActiveCuratedPresetIdx = -1;
            ClearPrePresetScratchpad();
            UpdateDiskSnapshot();
        }
    }
}
