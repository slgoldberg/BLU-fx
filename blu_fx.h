/* Copyright (C) 2024-2026  Steve Goldberg
 *
 * Original version:
 * Copyright (C) 2018  Matteo Hausner
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
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef BLU_FX_H
#define BLU_FX_H

#include <string>

// Define plugin name & version macros
#define NAME "BLU-fx"
#define NAME_BLANK "      "           /* to align multi-line log entries :-) */
#define NAME_LOWERCASE "blu_fx"

#define VERSION "2.1.0b1"
#define VERSION_BLANK "     "         /* to align multi-line log entries :-) */

// Define qualified name that includes version (e.g., "BLU-fx v1.3.0")
#define NAME_VERSION NAME " v" VERSION
#define NAME_VERSION_BLANK NAME "  " VERSION_BLANK "  "

// Define legacy config file path (for first-run migration)
#if IBM
#define CONFIG_PATH ".\\Resources\\plugins\\" NAME_LOWERCASE "\\" NAME_LOWERCASE ".ini"
#else
#define CONFIG_PATH "./Resources/plugins/" NAME_LOWERCASE "/" NAME_LOWERCASE ".ini"
#endif

// User-facing display path without leading "./" or ".\\" prefix
#define CONFIG_PATH_DISPLAY "Resources/plugins/" NAME_LOWERCASE "/" NAME_LOWERCASE ".ini"

// Define user profiles directory path
#if IBM
#define PROFILES_PATH ".\\Resources\\plugins\\" NAME_LOWERCASE "\\profiles"
#else
#define PROFILES_PATH "./Resources/plugins/" NAME_LOWERCASE "/profiles"
#endif

#define PROFILES_PATH_DISPLAY "Resources/plugins/" NAME_LOWERCASE "/profiles"
// Backward compatibility aliases:
#define PRESETS_PATH PROFILES_PATH
#define PRESETS_PATH_DISPLAY PROFILES_PATH_DISPLAY

// Define user preferences file path
#if IBM
#define PREFS_PATH ".\\Output\\preferences\\" NAME_LOWERCASE ".prf"
#else
#define PREFS_PATH "./Output/preferences/" NAME_LOWERCASE ".prf"
#endif

#define PREFS_PATH_DISPLAY "Output/preferences/" NAME_LOWERCASE ".prf"

#define DEFAULT_POST_PROCESSING_ENABLED 1
#define DEFAULT_FPS_LIMITER_ENABLED 0
#define DEFAULT_CONTROL_CINEMA_VERITE_ENABLED 0 /* was 1 by default in 32-bit version */
#define DEFAULT_RALEIGH_SCALE 13.0f
#define DEFAULT_MAX_FRAME_RATE 30.0f
#define DEFAULT_DISABLE_CINEMA_VERITE_TIME 5.0f
#define DEFAULT_GAMMA 1.0f
#define DEFAULT_NIGHT_FILTER_STRENGTH 0.45f

enum BLUfxPresets_t
{
    PRESET_USER = 0,            // current scratchpad, saved/restored to .ini file
    PRESET_DEFAULT,
    PRESET_POLAROID,
    PRESET_FOGGED_UP,
    PRESET_HIGH_DYNAMIC_RANGE,
    PRESET_EDITORS_CHOICE,
    PRESET_SLIGHTLY_ENHANCED,
    PRESET_EXTRA_GLOOMY,
    PRESET_RED_ISH,
    PRESET_GREEN_ISH,
    PRESET_BLUE_ISH,
    PRESET_SHINY_CALIFORNIA,
    PRESET_DUSTY_DRY,
    PRESET_GRAY_WINTER,
    PRESET_FANCY_IMAGINATION,
    PRESET_SIXTIES,
    PRESET_COLD_WINTER,
    PRESET_VINTAGE_FILM,
    PRESET_COLORLESS,
    PRESET_MONOCHROME,
    PRESET_MAX
};

struct BLUfxPreset_t
{
    // basic
    float brightness;
    float contrast;
    float saturation;
    // scale
    float redScale;
    float greenScale;
    float blueScale;
    // offset
    float redOffset;
    float greenOffset;
    float blueOffset;
    // misc
    float vignette;
    float raleighScale;
    float maxFps;
    float disableCinemaVeriteTime;
    // v2.1 extended tone, color & optics (neutral defaults keep v1.x/v2.0 presets identical)
    float shadows = 0.0f;
    float highlights = 0.0f;
    float gammaVal = DEFAULT_GAMMA;
    float temperature = 0.0f;
    float tint = 0.0f;
    float vibrance = 0.0f;
    float sharpen = 0.0f;
    float filmGrain = 0.0f;
    int filmicRolloff = 0;
    int nightFilterEnabled = 0;
    float nightFilterStrength = DEFAULT_NIGHT_FILTER_STRENGTH;
};
typedef BLUfxPreset_t BLUfxPreset;

// Global state & presets defined in blu_fx.cpp:
extern BLUfxPreset BLUfxPresets[PRESET_MAX];
extern const char* const BLUfxPresetNames[PRESET_MAX];
extern const char* const BLUfxPresetDescriptions[PRESET_MAX];

extern int xplmVersionNum;
#define IS_XP12         (xplmVersionNum >= 120000)
#define IS_XP12_1_4     (xplmVersionNum >= 120104)
#define LEGACY_FEATURES (!IS_XP12)

extern int postProcesssingEnabled;
extern int fpsLimiterEnabled;
extern int controlCinemaVeriteEnabled;

extern float maxFps;
extern float disableCinemaVeriteTime;

extern float brightness;
extern float contrast;
extern float saturation;
extern float redScale;
extern float greenScale;
extern float blueScale;
extern float redOffset;
extern float greenOffset;
extern float blueOffset;
extern float vignette;
extern float raleighScale;

// v2.1 extended adjustments & effects:
extern float shadows;
extern float highlights;
extern float gammaVal;
extern float temperature;
extern float tint;
extern float vibrance;
extern float sharpen;
extern float filmGrain;
extern int filmicRolloff;
extern int nightFilterEnabled;
extern float nightFilterStrength;

// Session-only per-section bypass flags (for live A/B auditing in accordion UI):
extern bool sBypassTone;
extern bool sBypassRgb;
extern bool sBypassOptics;

// Core plugin actions exposed to SettingsImgWindow:
bool isActivePreset(const BLUfxPreset_t* preset);
bool HasUnsavedChanges(void);
bool HasDiscardedPreset(void);
void CaptureDiscardedPreset(void);
void RestoreDiscardedPreset(void);
void SavePrePresetScratchpad(void);
void RestorePrePresetScratchpad(void);
void ClearPrePresetScratchpad(void);
bool HasPrePresetScratchpad(void);
void ApplyPreset(int presetIdx);
void UpdatePostProcessingMenuState(void);
void UpdateMenuCheckmarks(void);
void SetPostProcessingEnabled(bool enabled);
void SetFpsLimiterEnabled(bool enabled);
void SetControlCinemaVeriteEnabled(bool enabled);
void UpdateRaleighScale(int reset);
void UpdateDiskSnapshot(void);
void SaveSettings(void);
void LoadSettings(void);
bool SaveSettingsToFile(const std::string& path);
bool LoadSettingsFromFile(const std::string& path);
void EnsureProfilesDirectoryExists(void);
void EnsurePresetsDirectoryExists(void); // alias

// Global command references
extern XPLMCommandRef g_ToggleUserGuideCmd;

// Live Preview Modes:
enum BluFxPreviewMode {
    PREVIEW_MODE_SPLIT = 0,
    PREVIEW_MODE_FULL  = 1
};

enum BluFxSplitOrientation {
    SPLIT_ORIENT_VERTICAL   = 0,
    SPLIT_ORIENT_HORIZONTAL = 1
};

extern int sPreviewMode;
extern float sPreviewSplitPosition;
extern int sPreviewSplitOrientation;
extern bool sPreviewSplitFlipped;
extern bool sMomentaryFullView;
extern bool sShowSplitDivider;
void SetMomentaryFullView(bool active);

// Application Preferences (Output/preferences/blu_fx.prf):
extern std::string sActiveProfileName;
extern std::string sStartupActiveProfile;
extern std::string sSessionInitialProfile;
extern std::string sUiMode;
extern float sUiOpacity;
extern int sConfirmOnClose;
extern int sMirrorLegacyIni;
void SyncLegacyIniMirror(void);
extern int sWindowLeft;
extern int sWindowTop;

// User Guide (DocViewer) persistent preferences (in-simulator boxels, zoom, theme & opacity):
extern int sDocViewerLeft;
extern int sDocViewerTop;
extern int sDocViewerWidth;
extern int sDocViewerHeight;
extern float sDocViewerZoom;
extern int sDocViewerTheme;
extern float sDocViewerOpacity;
extern int sDocViewerTransparent;

std::string GetPrefsFilePath(void);
bool LoadPreferences(void);
bool SavePreferences(void);
void MigrateLegacyConfigIfNeeded(void);

#endif // BLU_FX_H
