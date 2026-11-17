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

#if IBM
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include "GLee.h"
#define __glext_h_ 1
#define __gl_glext_h_ 1
#pragma comment(lib, "winmm.lib")
#elif APL
#include <OpenGL/gl.h>
#elif LIN
#include <GL/gl.h>
#endif

#ifndef XPLM_DEPRECATED
#define XPLM_DEPRECATED 1   /* for legacy PostProcessingCallback draw hook */
#endif
#include "XPLMDisplay.h"

#include "blu_fx.h"
#include "settings-img-window.h"
#include "doc-viewer-img-window.h"

#include "XPLMDataAccess.h"
#include "XPLMDefs.h"

#include "XPLMGraphics.h"
#include "XPLMMenus.h"
#include "XPLMPlugin.h"
#include "XPLMProcessing.h"
#include "XPLMUtilities.h"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <memory>
#include <string>

#if IBM
#include <direct.h>
#define BLUFX_MKDIR(d) _mkdir(d)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define BLUFX_MKDIR(d) mkdir(d, 0755)
#endif

#if !IBM
#include <string.h>
#include <unistd.h>
#endif

const char* const BLUfxPresetNames[PRESET_MAX] =
{
    "User (.ini)",
    "Default",
    "Polaroid",
    "Fogged Up",
    "High Dynamic Range",
    "Editor's Choice",
    "Slightly Enhanced",
    "Extra Gloomy",
    "Red-ish",
    "Green-ish",
    "Blue-ish",
    "Shiny California",
    "Dusty Dry",
    "Gray Winter",
    "Fancy Imagination",
    "Sixties",
    "Cold Winter",
    "Vintage Film",
    "Colorless",
    "Monochrome"
};

const char* const BLUfxPresetDescriptions[PRESET_MAX] =
{
    "User scratchpad loaded from and saved to blu_fx.ini.",
    "Unmodified standard X-Plane scenery colors and neutral contrast.",
    "Warm retro instant-photo grading with subtle corner vignette.",
    "Soft atmospheric haze with reduced contrast for misty weather.",
    "Punchy dynamic range with deepened shadows and vibrant saturation.",
    "Balanced, rich color enhancement curated for everyday flying.",
    "Subtle natural boost to clarity and dynamic range.",
    "Moody overcast grading with darkened exposure and elevated contrast.",
    "Warm sunset atmosphere boosting red mid-tone curves.",
    "Lush verdant tint boosting green terrain and foliage mid-tones.",
    "Crisp high-altitude cool tint boosting blue mid-tones.",
    "Warm golden-hour radiance with brightened exposure and vivid colors.",
    "Warm desaturated palette suited for arid desert landscapes.",
    "Muted overcast tones with chilled desaturation for winter skies.",
    "Stylized cinematic grade with dramatic tonal balance.",
    "Nostalgic analog film grading with warm sixties tones.",
    "Icy blue shadows and high-contrast highlights for freezing weather.",
    "Classic celluloid emulsion with gentle vignette and softened contrast.",
    "Near-complete desaturation preserving subtle hints of color.",
    "High-contrast black-and-white fine-art photography grade."
};

BLUfxPreset BLUfxPresets [PRESET_MAX] =
{
    // PRESET_USER (scratchpad; looks like default, but read from .ini)
    {
        // starts out identical to default, but overwritten by LoadSettings()
        0.0f, // brightness
        1.0f, // contrast
        1.0f, // saturation
        0.0f, // red scale
        0.0f, // green scale
        0.0f, // blue scale
        0.0f, // red offset
        0.0f, // green offset
        0.0f, // blue offset
        0.0f, // vignette
        DEFAULT_RALEIGH_SCALE, // raleigh scale
        DEFAULT_MAX_FRAME_RATE, // default frame rate
        DEFAULT_DISABLE_CINEMA_VERITE_TIME, // default CV disable time
    },
    // PRESET_DEFAULT
    {
        0.0f, // brightness
        1.0f, // contrast
        1.0f, // saturation
        0.0f, // red scale
        0.0f, // green scale
        0.0f, // blue scale
        0.0f, // red offset
        0.0f, // green offset
        0.0f, // blue offset
        0.0f, // vignette
        DEFAULT_RALEIGH_SCALE, // raleigh scale
        DEFAULT_MAX_FRAME_RATE, // default frame rate
        DEFAULT_DISABLE_CINEMA_VERITE_TIME, // default CV disable time
    },
    // PRESET_POLAROID
    {
        0.05f, // brightness
        1.1f, // contrast
        1.4f, // saturation
        0.0f, // red scale
        0.0f, // green scale
        -0.2f, // blue scale
        0.0f, // red offset
        0.0f, // green offset
        0.0f, // blue offset
        0.6f, // vignette
        DEFAULT_RALEIGH_SCALE, // raleigh scale
        DEFAULT_MAX_FRAME_RATE, // default frame rate
        DEFAULT_DISABLE_CINEMA_VERITE_TIME, // default CV disable time
    },
    // PRESET_FOGGED_UP
    {
        0.05f, // brightness
        1.2f, // contrast
        0.7f, // saturation
        0.15f, // red scale
        0.15f, // green scale
        0.15f, // blue scale
        0.0f, // red offset
        0.0f, // green offset
        0.0f, // blue offset
        0.3f, // vignette
        DEFAULT_RALEIGH_SCALE, // raleigh scale
        DEFAULT_MAX_FRAME_RATE, // default frame rate
        DEFAULT_DISABLE_CINEMA_VERITE_TIME, // default CV disable time
    },
    // PRESET_HIGH_DYNAMIC_RANGE
    {
        0.0f, // brightness
        1.15f, // contrast
        0.9f, // saturation
        0.0f, // red scale
        0.0f, // green scale
        0.0f, // blue scale
        0.0f, // red offset
        0.0f, // green offset
        0.0f, // blue offset
        0.6f, // vignette
        DEFAULT_RALEIGH_SCALE, // raleigh scale
        DEFAULT_MAX_FRAME_RATE, // default frame rate
        DEFAULT_DISABLE_CINEMA_VERITE_TIME, // default CV disable time
    },
    // PRESET_EDITORS_CHOICE
    {
        0.05f, // brightness
        1.1f, // contrast
        1.3f, // saturation
        0.0f, // red scale
        0.0f, // green scale
        0.0f, // blue scale
        0.0f, // red offset
        0.0f, // green offset
        0.0f, // blue offset
        0.3f, // vignette
        DEFAULT_RALEIGH_SCALE, // raleigh scale
        DEFAULT_MAX_FRAME_RATE, // default frame rate
        DEFAULT_DISABLE_CINEMA_VERITE_TIME, // default CV disable time
    },
    // PRESET_SLIGHTLY_ENHANCED
    {
        0.05f, // brightness
        1.1f, // contrast
        1.1f, // saturation
        0.0f, // red scale
        0.0f, // green scale
        0.0f, // blue scale
        0.0f, // red offset
        0.0f, // green offset
        0.0f, // blue offset
        0.0f, // vignette
        DEFAULT_RALEIGH_SCALE, // raleigh scale
        DEFAULT_MAX_FRAME_RATE, // default frame rate
        DEFAULT_DISABLE_CINEMA_VERITE_TIME, // default CV disable time
    },
    // PRESET_EXTRA_GLOOMY
    {
        -0.15f, // brightness
        1.3f, // contrast
        1.0f, // saturation
        0.0f, // red scale
        0.0f, // green scale
        0.0f, // blue scale
        0.0f, // red offset
        0.0f, // green offset
        0.0f, // blue offset
        0.0f, // vignette
        DEFAULT_RALEIGH_SCALE, // raleigh scale
        DEFAULT_MAX_FRAME_RATE, // default frame rate
        DEFAULT_DISABLE_CINEMA_VERITE_TIME, // default CV disable time
    },
    // PRESET_RED_ISH
    {
        0.0f, // brightness
        1.0f, // contrast
        1.0f, // saturation
        0.1f, // red scale
        0.0f, // green scale
        0.0f, // blue scale
        0.0f, // red offset
        0.0f, // green offset
        0.0f, // blue offset
        0.0f, // vignette
        DEFAULT_RALEIGH_SCALE, // raleigh scale
        DEFAULT_MAX_FRAME_RATE, // default frame rate
        DEFAULT_DISABLE_CINEMA_VERITE_TIME, // default CV disable time
    },
    // PRESET_GREEN_ISH
    {
        0.0f, // brightness
        1.0f, // contrast
        1.0f, // saturation
        0.0f, // red scale
        0.1f, // green scale
        0.0f, // blue scale
        0.0f, // red offset
        0.0f, // green offset
        0.0f, // blue offset
        0.0f, // vignette
        DEFAULT_RALEIGH_SCALE, // raleigh scale
        DEFAULT_MAX_FRAME_RATE, // default frame rate
        DEFAULT_DISABLE_CINEMA_VERITE_TIME, // default CV disable time
    },
    // PRESET_BLUE_ISH
    {
        0.0f, // brightness
        1.0f, // contrast
        1.0f, // saturation
        0.0f, // red scale
        0.0f, // green scale
        0.1f, // blue scale
        0.0f, // red offset
        0.0f, // green offset
        0.0f, // blue offset
        0.0f, // vignette
        DEFAULT_RALEIGH_SCALE, // raleigh scale
        DEFAULT_MAX_FRAME_RATE, // default frame rate
        DEFAULT_DISABLE_CINEMA_VERITE_TIME, // default CV disable time
    },
    // PRESET_SHINY_CALIFORNIA
    {
        0.1f, // brightness
        1.5f, // contrast
        1.3f, // saturation
        0.0f, // red scale
        0.0f, // green scale
        0.0f, // blue scale
        0.0f, // red offset
        0.0f, // green offset
        -0.1f, // blue offset
        0.0f, // vignette
        DEFAULT_RALEIGH_SCALE, // raleigh scale
        DEFAULT_MAX_FRAME_RATE, // default frame rate
        DEFAULT_DISABLE_CINEMA_VERITE_TIME, // default CV disable time
    },
    // PRESET_DUSTY_DRY
    {
        0.0f, // brightness
        1.3f, // contrast
        1.3f, // saturation
        0.2f, // red scale
        0.0f, // green scale
        0.0f, // blue scale
        0.0f, // red offset
        0.0f, // green offset
        0.0f, // blue offset
        0.6f, // vignette
        DEFAULT_RALEIGH_SCALE, // raleigh scale
        DEFAULT_MAX_FRAME_RATE, // default frame rate
        DEFAULT_DISABLE_CINEMA_VERITE_TIME, // default CV disable time
    },
    // PRESET_GRAY_WINTER
    {
        0.07f, // brightness
        1.15f, // contrast
        1.3f, // saturation
        0.0f, // red scale
        0.0f, // green scale
        0.0f, // blue scale
        0.0f, // red offset
        0.05f, // green offset
        0.0f, // blue offset
        0.6f, // vignette
        DEFAULT_RALEIGH_SCALE, // raleigh scale
        DEFAULT_MAX_FRAME_RATE, // default frame rate
        DEFAULT_DISABLE_CINEMA_VERITE_TIME, // default CV disable time
    },
    // PRESET_FANCY_IMAGINATION
    {
        0.0f, // brightness
        1.6f, // contrast
        1.5f, // saturation
        0.0f, // red scale
        0.0f, // green scale
        -0.1f, // blue scale
        0.0f, // red offset
        0.05f, // green offset
        0.0f, // blue offset
        0.6f, // vignette
        DEFAULT_RALEIGH_SCALE, // raleigh scale
        DEFAULT_MAX_FRAME_RATE, // default frame rate
        DEFAULT_DISABLE_CINEMA_VERITE_TIME, // default CV disable time
    },
    // PRESET_SIXTIES
    {
        0.0f, // brightness
        1.6f, // contrast
        1.5f, // saturation
        0.2f, // red scale
        0.0f, // green scale
        -0.1f, // blue scale
        0.0f, // red offset
        0.05f, // green offset
        0.0f, // blue offset
        0.65f, // vignette
        DEFAULT_RALEIGH_SCALE, // raleigh scale
        DEFAULT_MAX_FRAME_RATE, // default frame rate
        DEFAULT_DISABLE_CINEMA_VERITE_TIME, // default CV disable time
    },
    // PRESET_COLD_WINTER
    {
        0.0f, // brightness
        1.55f, // contrast
        0.0f, // saturation
        0.0f, // red scale
        0.05f, // green scale
        0.2f, // blue scale
        0.0f, // red offset
        0.05f, // green offset
        0.0f, // blue offset
        0.25f, // vignette
        DEFAULT_RALEIGH_SCALE, // raleigh scale
        DEFAULT_MAX_FRAME_RATE, // default frame rate
        DEFAULT_DISABLE_CINEMA_VERITE_TIME, // default CV disable time
    },
    // PRESET_VINTAGE_FILM
    {
        0.0f, // brightness
        1.05f, // contrast
        0.0f, // saturation
        0.0f, // red scale
        0.0f, // green scale
        0.07f, // blue scale
        0.07f, // red offset
        0.03f, // green offset
        0.0f, // blue offset
        0.0f, // vignette
        DEFAULT_RALEIGH_SCALE, // raleigh scale
        DEFAULT_MAX_FRAME_RATE, // default frame rate
        DEFAULT_DISABLE_CINEMA_VERITE_TIME, // default CV disable time
    },
    // PRESET_COLORLESS
    {
        -0.03f, // brightness
        1.3f, // contrast
        0.0f, // saturation
        0.0f, // red scale
        0.0f, // green scale
        0.0f, // blue scale
        0.0f, // red offset
        0.03f, // green offset
        0.0f, // blue offset
        0.65f, // vignette
        DEFAULT_RALEIGH_SCALE, // raleigh scale
        DEFAULT_MAX_FRAME_RATE, // default frame rate
        DEFAULT_DISABLE_CINEMA_VERITE_TIME, // default CV disable time
    },
    // PRESET_MONOCHROME
    {
        -0.13f, // brightness
        1.2f, // contrast
        0.0f, // saturation
        0.0f, // red scale
        0.0f, // green scale
        0.0f, // blue scale
        0.0f, // red offset
        0.03f, // green offset
        0.0f, // blue offset
        0.7f, // vignette
        DEFAULT_RALEIGH_SCALE, // raleigh scale
        DEFAULT_MAX_FRAME_RATE, // default frame rate
        DEFAULT_DISABLE_CINEMA_VERITE_TIME, // default CV disable time
    }
};

// fragment-shader code
#define FRAGMENT_SHADER "#version 120\n"\
                        "const vec3 lumCoeff = vec3(0.2125, 0.7154, 0.0721);"\
                        "uniform float brightness;"\
                        "uniform float contrast;"\
                        "uniform float saturation;"\
                        "uniform float redScale;"\
                        "uniform float greenScale;"\
                        "uniform float blueScale;"\
                        "uniform float redOffset;"\
                        "uniform float greenOffset;"\
                        "uniform float blueOffset;"\
                        "uniform vec2 resolution;"\
                        "uniform float vignette;"\
                        "uniform sampler2D scene;"\
                        "void main()"\
                        "{"\
                            "vec3 color = texture2D(scene, gl_TexCoord[0].st).rgb;"\
                            "color *= contrast;"\
                            "color += vec3(brightness, brightness, brightness);"\
                            "vec3 intensity = vec3(dot(color, lumCoeff));"\
                            "color = mix(intensity, color, saturation);"\
                            "vec3 newColor = (color.rgb - 0.5) * 2.0;"\
                            "newColor.r = 2.0 / 3.0 * (1.0 - (newColor.r * newColor.r));"\
                            "newColor.g = 2.0 / 3.0 * (1.0 - (newColor.g * newColor.g));"\
                            "newColor.b = 2.0 / 3.0 * (1.0 - (newColor.b * newColor.b));"\
                            "newColor.r = clamp(color.r + redScale * newColor.r + redOffset, 0.0, 1.0);"\
                            "newColor.g = clamp(color.g + greenScale * newColor.g + greenOffset, 0.0, 1.0);"\
                            "newColor.b = clamp(color.b + blueScale * newColor.b + blueOffset, 0.0, 1.0);"\
                            "color = newColor;"\
                            "vec2 position = (gl_FragCoord.xy / resolution.xy) - vec2(0.5);"\
                            "float len = length(position);"\
                            "float vig = smoothstep(0.75, 0.75 - 0.45, len);"\
                            "color = mix(color, color * vig, vignette);"\
                            "gl_FragColor = vec4(color, 1.0);"\
                        "}"

// Version number relation functionality (i.e., "legacy" or not)
int xplmVersionNum = 0; // filled in at startup

// Global settings variables
int postProcesssingEnabled = DEFAULT_POST_PROCESSING_ENABLED;
int fpsLimiterEnabled = DEFAULT_FPS_LIMITER_ENABLED;
int controlCinemaVeriteEnabled = DEFAULT_CONTROL_CINEMA_VERITE_ENABLED;

float maxFps = DEFAULT_MAX_FRAME_RATE;
float disableCinemaVeriteTime = DEFAULT_DISABLE_CINEMA_VERITE_TIME;

float brightness = BLUfxPresets[PRESET_DEFAULT].brightness;
float contrast = BLUfxPresets[PRESET_DEFAULT].contrast;
float saturation = BLUfxPresets[PRESET_DEFAULT].saturation;
float redScale = BLUfxPresets[PRESET_DEFAULT].redScale;
float greenScale = BLUfxPresets[PRESET_DEFAULT].greenScale;
float blueScale = BLUfxPresets[PRESET_DEFAULT].blueScale;
float redOffset = BLUfxPresets[PRESET_DEFAULT].redOffset;
float greenOffset = BLUfxPresets[PRESET_DEFAULT].greenOffset;
float blueOffset = BLUfxPresets[PRESET_DEFAULT].blueOffset;
float vignette = BLUfxPresets[PRESET_DEFAULT].vignette;
float raleighScale = DEFAULT_RALEIGH_SCALE;

static inline bool FloatNear(float a, float b, float eps = 0.0005f)
{
    return (a - b < eps) && (b - a < eps);
}

// Determines whether a given BLUfxPreset matches the current settings globals:
bool isActivePreset(const BLUfxPreset_t* preset)
{
    return (!preset ? false :
            FloatNear(preset->brightness, brightness) &&
            FloatNear(preset->saturation, saturation) &&
            FloatNear(preset->blueOffset, blueOffset) &&
            FloatNear(preset->blueScale, blueScale) &&
            FloatNear(preset->contrast, contrast) &&
            FloatNear(preset->greenOffset, greenOffset) &&
            FloatNear(preset->greenScale, greenScale) &&
            (IS_XP12 || FloatNear(preset->raleighScale, raleighScale)) &&
            FloatNear(preset->redOffset, redOffset) &&
            FloatNear(preset->redScale, redScale) &&
            FloatNear(preset->vignette, vignette));
}

void ApplyPreset(int presetIdx)
{
    if (presetIdx < 0 || presetIdx >= PRESET_MAX)
        return;

    if (presetIdx == PRESET_DEFAULT || presetIdx == PRESET_USER)
        ClearPrePresetScratchpad();

    brightness  = BLUfxPresets[presetIdx].brightness;
    contrast    = BLUfxPresets[presetIdx].contrast;
    saturation  = BLUfxPresets[presetIdx].saturation;
    redScale    = BLUfxPresets[presetIdx].redScale;
    greenScale  = BLUfxPresets[presetIdx].greenScale;
    blueScale   = BLUfxPresets[presetIdx].blueScale;
    redOffset   = BLUfxPresets[presetIdx].redOffset;
    greenOffset = BLUfxPresets[presetIdx].greenOffset;
    blueOffset  = BLUfxPresets[presetIdx].blueOffset;
    vignette    = BLUfxPresets[presetIdx].vignette;
#ifdef INCLUDE_SETTINGS_IN_PRESETS /* not really loaded, except for reload .ini */
    raleighScale = BLUfxPresets[presetIdx].raleighScale;
    maxFps       = BLUfxPresets[presetIdx].maxFps;
    disableCinemaVeriteTime = BLUfxPresets[presetIdx].disableCinemaVeriteTime;
#endif
}

// Global internal variables
static int lastResolutionX = 0, lastResolutionY = 0, bringFakeWindowToFront = 0, overrideControlCinemaVerite = 0;
static GLuint textureId = 0, program = 0, fragmentShader = 0;
static float startTimeFlight = 0.0f, endTimeFlight = 0.0f, startTimeDraw = 0.0f, endTimeDraw = 0.0f, lastMouseUsageTime = 0.0f;
static XPLMWindowID fakeWindow = NULL;

// Global dataref variables
static XPLMDataRef cinemaVeriteDataRef = NULL, viewTypeDataRef = NULL, raleighScaleDataRef = NULL, overrideControlCinemaVeriteDataRef = NULL;
static XPLMDataRef xplmVersionDataRef = NULL;

// Modern ImgWindow v2 Settings Window instance & toggle commands:
static std::unique_ptr<SettingsImgWindow> g_SettingsWindow;
static XPLMCommandRef g_ToggleSettingsCmd = NULL;
XPLMCommandRef g_ToggleUserGuideCmd = NULL;
static XPLMCommandRef g_TogglePostProcessingCmd = NULL;
static XPLMCommandRef g_MomentaryPostProcessingCmd = NULL;
static XPLMMenuID g_BluFxMenu = NULL;
static int g_ToggleSettingsMenuItemIdx = -1;
static int g_ToggleUserGuideMenuItemIdx = -1;
static int g_TogglePostProcessingMenuItemIdx = -1;

void UpdateMenuCheckmarks(void)
{
    if (g_BluFxMenu == NULL) return;

    if (g_TogglePostProcessingMenuItemIdx >= 0) {
        XPLMCheckMenuItem(g_BluFxMenu, g_TogglePostProcessingMenuItemIdx,
            postProcesssingEnabled ? xplm_Menu_Checked : xplm_Menu_Unchecked);
    }
    if (g_ToggleSettingsMenuItemIdx >= 0) {
        bool settingsVisible = (g_SettingsWindow && g_SettingsWindow->GetVisible());
        XPLMCheckMenuItem(g_BluFxMenu, g_ToggleSettingsMenuItemIdx,
            settingsVisible ? xplm_Menu_Checked : xplm_Menu_Unchecked);
    }
    if (g_ToggleUserGuideMenuItemIdx >= 0) {
        bool docVisible = DocViewerImgWindow::IsOpen();
        XPLMCheckMenuItem(g_BluFxMenu, g_ToggleUserGuideMenuItemIdx,
            docVisible ? xplm_Menu_Checked : xplm_Menu_Unchecked);
    }
}

void UpdatePostProcessingMenuState(void)
{
    UpdateMenuCheckmarks();
}

// draw-callback that adds post-processing
static int PostProcessingCallback(XPLMDrawingPhase inPhase, int inIsBefore, void *inRefcon)
{
    if (!postProcesssingEnabled && !sMomentaryFullView) {
        return 1;
    }

    int x, y;
    XPLMGetScreenSize(&x, &y);

    if(textureId == 0 || lastResolutionX != x || lastResolutionY != y)
    {
        XPLMGenerateTextureNumbers((int *) &textureId, 1);
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        lastResolutionX = x;
        lastResolutionY = y;
    }
    else
    {
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_2D, textureId);
    }

    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, x, y);
    XPLMSetGraphicsState(0, 1, 0, 0, 0,  0, 0);

    glUseProgram(program);

    int brightnessLocation = glGetUniformLocation(program, "brightness");
    glUniform1f(brightnessLocation, brightness);

    int contrastLocation = glGetUniformLocation(program, "contrast");
    glUniform1f(contrastLocation, contrast);

    int saturationLocation = glGetUniformLocation(program, "saturation");
    glUniform1f(saturationLocation, saturation);

    int redScaleLocation = glGetUniformLocation(program, "redScale");
    glUniform1f(redScaleLocation, redScale);

    int greenScaleLocation = glGetUniformLocation(program, "greenScale");
    glUniform1f(greenScaleLocation, greenScale);

    int blueScaleLocation = glGetUniformLocation(program, "blueScale");
    glUniform1f(blueScaleLocation, blueScale);

    int redOffsetLocation = glGetUniformLocation(program, "redOffset");
    glUniform1f(redOffsetLocation, redOffset);

    int greenOffsetLocation = glGetUniformLocation(program, "greenOffset");
    glUniform1f(greenOffsetLocation, greenOffset);

    int blueOffsetLocation = glGetUniformLocation(program, "blueOffset");
    glUniform1f(blueOffsetLocation, blueOffset);

    int resolutionLocation = glGetUniformLocation(program, "resolution");
    glUniform2f(resolutionLocation, (float) x, (float) y);

    int vignetteLocation = glGetUniformLocation(program, "vignette");
    glUniform1f(vignetteLocation, vignette);

    int sceneLocation = glGetUniformLocation(program, "scene");
    glUniform1i(sceneLocation, 0);

    glPushAttrib(GL_VIEWPORT_BIT | GL_ENABLE_BIT);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0f, x, 0.0f, y, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glViewport(0, 0, x, y);

    int settingsWindowOpen = (g_SettingsWindow && g_SettingsWindow->GetVisible()) ? 1 : 0;
    float splitFrac = 0.0f;
    if (settingsWindowOpen && sPreviewMode == PREVIEW_MODE_SPLIT && !sMomentaryFullView) {
        splitFrac = (sPreviewSplitPosition < 0.05f) ? 0.05f : ((sPreviewSplitPosition > 0.95f) ? 0.95f : sPreviewSplitPosition);
    }

    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(splitFrac, 0.0f);
    glVertex2f((GLfloat)(x * splitFrac), 0.0f);
    glTexCoord2f(splitFrac, 1.0f);
    glVertex2f((GLfloat)(x * splitFrac), (GLfloat)y);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f((GLfloat)x, (GLfloat)y);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f((GLfloat)x, 0.0f);
    glEnd();

    // Optional vertical divider hairline at the split boundary:
    if (settingsWindowOpen && sPreviewMode == PREVIEW_MODE_SPLIT && !sMomentaryFullView && sShowSplitDivider && splitFrac > 0.01f && splitFrac < 0.99f) {
        glUseProgram(0);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        GLfloat divX = (GLfloat)(x * splitFrac);
        glLineWidth(1.0f);
        glBegin(GL_LINES);
        // Soft dark shadow line immediately to the left for contrast against bright scenes:
        glColor4f(0.0f, 0.0f, 0.0f, 0.35f);
        glVertex2f(divX - 1.0f, 0.0f);
        glVertex2f(divX - 1.0f, (GLfloat)y);
        // Crisp translucent white hairline:
        glColor4f(1.0f, 1.0f, 1.0f, 0.65f);
        glVertex2f(divX, 0.0f);
        glVertex2f(divX, (GLfloat)y);
        glEnd();
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glPopAttrib();

    glUseProgram(0);

    return 1;
}

// flightloop-callback that resizes and brings the fake window back to the front if needed
static float UpdateFakeWindowCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void *inRefcon)
{
    if (fakeWindow != NULL)
    {
        int x = 0, y = 0;
        XPLMGetScreenSize(&x, &y);
        XPLMSetWindowGeometry(fakeWindow, 0, y, x, 0);

        if (!bringFakeWindowToFront)
        {
            XPLMBringWindowToFront(fakeWindow);
            bringFakeWindowToFront = 1;
        }
    }

    return -1.0f;
}

// lets the thread sleep to achieve the set maximum frame rate
inline static void LimitFps(float dt)
{
    float t = 1.0f / maxFps - dt;

    if (t > 0.0f)
    {
#if IBM
        DWORD currentTime = timeGetTime();
        DWORD targetTime = currentTime + (DWORD) (t * 1000.0f);
        while (currentTime < targetTime)
        {
            Sleep(0);
            currentTime = timeGetTime();
        }
#else
        usleep((useconds_t)(t * 1000000.0f));
#endif
    }
}

// flightloop-callback that limits the number of flightcycles
static float LimiterFlightCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void *inRefcon)
{
    endTimeFlight = XPLMGetElapsedTime();
    LimitFps(endTimeFlight - startTimeFlight);
    startTimeFlight = XPLMGetElapsedTime();

    return -1.0f;
}

// draw-callback that limits the number of drawcycles
static int LimiterDrawCallback(XPLMDrawingPhase inPhase, int inIsBefore, void *inRefcon)
{
    endTimeDraw = XPLMGetElapsedTime();
    LimitFps(endTimeDraw - startTimeDraw);
    startTimeDraw = XPLMGetElapsedTime();

    return 1;
}

// flightloop-callback that auto-controls cinema-verite
static float ControlCinemaVeriteCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void *inRefcon)
{
    if (controlCinemaVeriteEnabled && !overrideControlCinemaVerite)
    {
        if (cinemaVeriteDataRef == NULL)
            ; // DO NOTHING if we're in a newer version of X-Plane (v12.1.4+)
        else if (XPLMGetDatai(viewTypeDataRef) == 1026) // 3D Cockpit
        {
            float elapsedTime = XPLMGetElapsedTime() - lastMouseUsageTime;

            if (elapsedTime <= disableCinemaVeriteTime)
                XPLMSetDatai(cinemaVeriteDataRef, 0);
            else
                XPLMSetDatai(cinemaVeriteDataRef, 1);
        }
        else
            XPLMSetDatai(cinemaVeriteDataRef, 1);
    }

    return -1.0f;
}

// removes the fragment-shader from video memory, if deleteProgram is set the shader-program is also removed
static void CleanupShader(int deleteProgram = 0)
{
    glDetachShader(program, fragmentShader);
    glDeleteShader(fragmentShader);

    if (deleteProgram)
        glDeleteProgram(program);
}

// function to load, compile and link the fragment-shader
static void InitShader(const char *fragmentShaderString)
{
    program = glCreateProgram();

    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderString, 0);
    glCompileShader(fragmentShader);
    glAttachShader(program, fragmentShader);
    GLint isFragmentShaderCompiled = GL_FALSE;
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &isFragmentShaderCompiled);
    if (isFragmentShaderCompiled == GL_FALSE)
    {
        GLsizei maxLength = 2048;
        GLchar *log = new GLchar[maxLength];
        glGetShaderInfoLog(fragmentShader, maxLength, &maxLength, log);
        XPLMDebugString(NAME_VERSION": The following error occured while compiling the fragment shader:\n");
        XPLMDebugString(NAME_VERSION_BLANK);  // indent to align where possible
        XPLMDebugString(log);
        delete[] log;

        CleanupShader(1);

        return;
    }

    glLinkProgram(program);
    GLint isProgramLinked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &isProgramLinked);
    if (isProgramLinked == GL_FALSE)
    {
        GLsizei maxLength = 2048;
        GLchar *log = new GLchar[maxLength];
        glGetShaderInfoLog(fragmentShader, maxLength, &maxLength, log);
        XPLMDebugString(NAME_VERSION": The following error occured while linking the shader program:\n");
        XPLMDebugString(NAME_VERSION_BLANK);  // indent to align where possible
        XPLMDebugString(log);
        delete[] log;

        CleanupShader(1);

        return;
    }

    CleanupShader(0);
}

// get accessor for override_cinema_verite_control DataRef
int GetOverrideControlCinemaVeriteDataRefCallback(void* inRefcon)
{
    return overrideControlCinemaVerite;
}

// set accessor for override_control_cinema_verite DataRef
void SetOverrideControlCinemaVeriteDataRefCallback(void* inRefcon, int inValue)
{
    overrideControlCinemaVerite = inValue;
}

// sets the raleigh scale dataref to the selected raleigh scale value, passing reset = 1 resets the dataref to its default value
void UpdateRaleighScale(int reset)
{
    if (!LEGACY_FEATURES)
        return; // return immediately if XP12.00 or later, since there is no Raleigh anymore
    
    if (raleighScaleDataRef == NULL)
        raleighScaleDataRef = XPLMFindDataRef("sim/private/controls/atmo/atmo_scale_raleigh");
    if (raleighScaleDataRef != NULL)
        XPLMSetDataf(raleighScaleDataRef, !reset ? raleighScale : DEFAULT_RALEIGH_SCALE);
}

void SetPostProcessingEnabled(bool enabled)
{
    int newState = enabled ? 1 : 0;
    if (postProcesssingEnabled == newState)
        return;

    postProcesssingEnabled = newState;
    if (!postProcesssingEnabled) {
        XPLMUnregisterDrawCallback(PostProcessingCallback, xplm_Phase_Window, 1, NULL);
        UpdateRaleighScale(1);
    } else {
        XPLMRegisterDrawCallback(PostProcessingCallback, xplm_Phase_Window, 1, NULL);
        UpdateRaleighScale(0);
    }
    UpdatePostProcessingMenuState();
}

void SetFpsLimiterEnabled(bool enabled)
{
    int newState = enabled ? 1 : 0;
    if (fpsLimiterEnabled == newState)
        return;

    fpsLimiterEnabled = newState;
    if (!fpsLimiterEnabled) {
        XPLMUnregisterFlightLoopCallback(LimiterFlightCallback, NULL);
        XPLMUnregisterDrawCallback(LimiterDrawCallback, xplm_Phase_Terrain, 1, NULL);
    } else {
        XPLMRegisterFlightLoopCallback(LimiterFlightCallback, -1, NULL);
        XPLMRegisterDrawCallback(LimiterDrawCallback, xplm_Phase_Terrain, 1, NULL);
    }
}

void SetControlCinemaVeriteEnabled(bool enabled)
{
    int newState = enabled ? 1 : 0;
    if (controlCinemaVeriteEnabled == newState)
        return;

    controlCinemaVeriteEnabled = newState;
    if (!controlCinemaVeriteEnabled) {
        XPLMUnregisterFlightLoopCallback(ControlCinemaVeriteCallback, NULL);
    } else {
        XPLMRegisterFlightLoopCallback(ControlCinemaVeriteCallback, -1, NULL);
    }
}

static BLUfxPreset sDiskPreset;
static bool sHasDiskSnapshot = false;

void UpdateDiskSnapshot(void)
{
    sDiskPreset.brightness = brightness;
    sDiskPreset.contrast = contrast;
    sDiskPreset.saturation = saturation;
    sDiskPreset.redScale = redScale;
    sDiskPreset.greenScale = greenScale;
    sDiskPreset.blueScale = blueScale;
    sDiskPreset.redOffset = redOffset;
    sDiskPreset.greenOffset = greenOffset;
    sDiskPreset.blueOffset = blueOffset;
    sDiskPreset.vignette = vignette;
    sDiskPreset.raleighScale = raleighScale;
    sDiskPreset.maxFps = maxFps;
    sDiskPreset.disableCinemaVeriteTime = disableCinemaVeriteTime;
    sHasDiskSnapshot = true;
}

bool HasUnsavedChanges(void)
{
    if (!sHasDiskSnapshot)
        return false;
    return !isActivePreset(&sDiskPreset);
}

static BLUfxPreset sLastDiscardedPreset;
static bool sHasDiscardedPreset = false;

void CaptureDiscardedPreset(void)
{
    sLastDiscardedPreset.brightness = brightness;
    sLastDiscardedPreset.contrast = contrast;
    sLastDiscardedPreset.saturation = saturation;
    sLastDiscardedPreset.redScale = redScale;
    sLastDiscardedPreset.greenScale = greenScale;
    sLastDiscardedPreset.blueScale = blueScale;
    sLastDiscardedPreset.redOffset = redOffset;
    sLastDiscardedPreset.greenOffset = greenOffset;
    sLastDiscardedPreset.blueOffset = blueOffset;
    sLastDiscardedPreset.vignette = vignette;
    sLastDiscardedPreset.raleighScale = raleighScale;
    sLastDiscardedPreset.maxFps = maxFps;
    sLastDiscardedPreset.disableCinemaVeriteTime = disableCinemaVeriteTime;
    sHasDiscardedPreset = true;
}

bool HasDiscardedPreset(void)
{
    return sHasDiscardedPreset;
}

void RestoreDiscardedPreset(void)
{
    if (!sHasDiscardedPreset)
        return;
    brightness  = sLastDiscardedPreset.brightness;
    contrast    = sLastDiscardedPreset.contrast;
    saturation  = sLastDiscardedPreset.saturation;
    redScale    = sLastDiscardedPreset.redScale;
    greenScale  = sLastDiscardedPreset.greenScale;
    blueScale   = sLastDiscardedPreset.blueScale;
    redOffset   = sLastDiscardedPreset.redOffset;
    greenOffset = sLastDiscardedPreset.greenOffset;
    blueOffset  = sLastDiscardedPreset.blueOffset;
    vignette    = sLastDiscardedPreset.vignette;
    raleighScale = sLastDiscardedPreset.raleighScale;
    UpdateRaleighScale(0);
}

static BLUfxPreset sPrePresetPreset;
static bool sHasPrePreset = false;

void SavePrePresetScratchpad(void)
{
    sPrePresetPreset.brightness = brightness;
    sPrePresetPreset.contrast = contrast;
    sPrePresetPreset.saturation = saturation;
    sPrePresetPreset.redScale = redScale;
    sPrePresetPreset.greenScale = greenScale;
    sPrePresetPreset.blueScale = blueScale;
    sPrePresetPreset.redOffset = redOffset;
    sPrePresetPreset.greenOffset = greenOffset;
    sPrePresetPreset.blueOffset = blueOffset;
    sPrePresetPreset.vignette = vignette;
    sPrePresetPreset.raleighScale = raleighScale;
    sPrePresetPreset.maxFps = maxFps;
    sPrePresetPreset.disableCinemaVeriteTime = disableCinemaVeriteTime;
    sHasPrePreset = true;
}

void RestorePrePresetScratchpad(void)
{
    if (!sHasPrePreset)
        return;
    brightness  = sPrePresetPreset.brightness;
    contrast    = sPrePresetPreset.contrast;
    saturation  = sPrePresetPreset.saturation;
    redScale    = sPrePresetPreset.redScale;
    greenScale  = sPrePresetPreset.greenScale;
    blueScale   = sPrePresetPreset.blueScale;
    redOffset   = sPrePresetPreset.redOffset;
    greenOffset = sPrePresetPreset.greenOffset;
    blueOffset  = sPrePresetPreset.blueOffset;
    vignette    = sPrePresetPreset.vignette;
    raleighScale = sPrePresetPreset.raleighScale;
    UpdateRaleighScale(0);
    sHasPrePreset = false;
}

bool HasPrePresetScratchpad(void)
{
    return sHasPrePreset;
}

void ClearPrePresetScratchpad(void)
{
    sHasPrePreset = false;
}

std::string sActiveProfileName = "blu_fx.ini";
std::string sStartupActiveProfile = "blu_fx.ini";
std::string sSessionInitialProfile = "blu_fx.ini";
std::string sUiMode = "sliders";
float sUiOpacity = 0.95f;
int sConfirmOnClose = 1; // 1 = prompt with confirmation dialog when closing with unsaved adjustments; 0 = silently keep in-memory
int sMirrorLegacyIni = 1; // 1 = mirror active profile to root blu_fx.ini for v1.x downgrade safety; 0 = disabled
int sWindowLeft = -1;
int sWindowTop = -1;
int sDocViewerLeft = -1;
int sDocViewerTop = -1;
int sDocViewerWidth = 0;
int sDocViewerHeight = 0;
float sDocViewerZoom = 1.0f;
int sPreviewMode = PREVIEW_MODE_SPLIT;
float sPreviewSplitPosition = 0.50f;
bool sMomentaryFullView = false;
bool sShowSplitDivider = true;

void SetMomentaryFullView(bool active)
{
    if (sMomentaryFullView == active) return;
    sMomentaryFullView = active;
    if (!postProcesssingEnabled) {
        if (sMomentaryFullView) {
            XPLMRegisterDrawCallback(PostProcessingCallback, xplm_Phase_Window, 1, NULL);
            UpdateRaleighScale(0);
        } else {
            XPLMUnregisterDrawCallback(PostProcessingCallback, xplm_Phase_Window, 1, NULL);
            UpdateRaleighScale(1);
        }
    }
}

void EnsureProfilesDirectoryExists(void)
{
    struct stat st;
    if (stat(PROFILES_PATH, &st) != 0) {
        BLUFX_MKDIR(PROFILES_PATH);
    }
}

void EnsurePresetsDirectoryExists(void)
{
    EnsureProfilesDirectoryExists();
}

std::string GetPrefsFilePath(void)
{
    char buf[512] = {0};
    XPLMGetPrefsPath(buf);
    XPLMExtractFileAndPath(buf);
    std::string path = buf;
    char sep = *XPLMGetDirectorySeparator();
    if (!path.empty() && path.back() != '/' && path.back() != '\\') {
        path += sep;
    }
    path += "blu_fx.prf";
    return path;
}

bool SavePreferences(void)
{
    std::string path = GetPrefsFilePath();
    std::fstream file;
    file.open(path.c_str(), std::ios_base::out | std::ios_base::trunc);
    if (!file.is_open()) return false;

    file << "# Blu-FX Application Preferences" << std::endl;
    file << "startupProfile=" << sStartupActiveProfile << std::endl;
    file << "activeProfile=" << sStartupActiveProfile << std::endl;
    file << "postProcessingEnabled=" << postProcesssingEnabled << std::endl;
    file << "uiMode=" << sUiMode << std::endl;
    file << "uiOpacity=" << std::fixed << std::setprecision(2) << sUiOpacity << std::endl;
    file << "confirmOnClose=" << sConfirmOnClose << std::endl;
    file << "mirrorLegacyIni=" << sMirrorLegacyIni << std::endl;
    file << "windowLeft=" << sWindowLeft << std::endl;
    file << "windowTop=" << sWindowTop << std::endl;
    if (sDocViewerLeft >= 0 && sDocViewerTop >= 0 && sDocViewerWidth >= 480 && sDocViewerHeight >= 300) {
        file << "docViewerLeft=" << sDocViewerLeft << std::endl;
        file << "docViewerTop=" << sDocViewerTop << std::endl;
        file << "docViewerWidth=" << sDocViewerWidth << std::endl;
        file << "docViewerHeight=" << sDocViewerHeight << std::endl;
        file << "docViewerZoom=" << std::fixed << std::setprecision(2) << sDocViewerZoom << std::endl;
    }
    file << "previewMode=" << (sPreviewMode == PREVIEW_MODE_FULL ? "full" : "split") << std::endl;
    file << "previewSplit=" << std::fixed << std::setprecision(2) << sPreviewSplitPosition << std::endl;
    file << "previewDivider=" << (sShowSplitDivider ? 1 : 0) << std::endl;
    int effectiveMaxFps = (fpsLimiterEnabled && maxFps > 0.0f) ? (int)maxFps : 0;
    file << "maxFps=" << effectiveMaxFps << std::endl;
    file << std::endl;
    file << "# Legacy Camera Settings (X-Plane 11)" << std::endl;
    file << "cinemaVeriteEnabled=" << controlCinemaVeriteEnabled << std::endl;
    file << "cinemaVeriteDisableTime=" << std::fixed << std::setprecision(1) << disableCinemaVeriteTime << std::endl;

    file.close();
    return true;
}

bool LoadPreferences(void)
{
    std::string path = GetPrefsFilePath();
    std::ifstream file;
    file.open(path.c_str());
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);

        while (!key.empty() && (key.back() == ' ' || key.back() == '\t' || key.back() == '\r')) key.pop_back();
        while (!val.empty() && (val.front() == ' ' || val.front() == '\t')) val.erase(0, 1);
        while (!val.empty() && (val.back() == ' ' || val.back() == '\t' || val.back() == '\r')) val.pop_back();

        std::istringstream iss(val);
        if (key == "startupProfile") {
            sStartupActiveProfile = val;
            sActiveProfileName = val;
        } else if (key == "activeProfile") {
            if (sStartupActiveProfile.empty() || sStartupActiveProfile == "blu_fx.ini") {
                sStartupActiveProfile = val;
                sActiveProfileName = val;
            }
        } else if (key == "postProcessingEnabled" || key == "postProcesssingEnabled") {
            iss >> postProcesssingEnabled;
        } else if (key == "uiMode") {
            sUiMode = val;
        } else if (key == "uiOpacity") {
            iss >> sUiOpacity;
            if (sUiOpacity < 0.20f) sUiOpacity = 0.20f;
            if (sUiOpacity > 1.00f) sUiOpacity = 1.00f;
        } else if (key == "confirmOnClose") {
            iss >> sConfirmOnClose;
        } else if (key == "mirrorLegacyIni") {
            iss >> sMirrorLegacyIni;
        } else if (key == "windowLeft" || key == "windowX") {
            iss >> sWindowLeft;
        } else if (key == "windowTop" || key == "windowY") {
            iss >> sWindowTop;
        } else if (key == "docViewerLeft") {
            iss >> sDocViewerLeft;
        } else if (key == "docViewerTop") {
            iss >> sDocViewerTop;
        } else if (key == "docViewerWidth") {
            iss >> sDocViewerWidth;
        } else if (key == "docViewerHeight") {
            iss >> sDocViewerHeight;
        } else if (key == "docViewerZoom") {
            iss >> sDocViewerZoom;
            if (sDocViewerZoom < 0.50f) sDocViewerZoom = 0.50f;
            if (sDocViewerZoom > 2.00f) sDocViewerZoom = 2.00f;
        } else if (key == "previewMode") {
            sPreviewMode = (val == "full") ? PREVIEW_MODE_FULL : PREVIEW_MODE_SPLIT;
        } else if (key == "previewSplit") {
            iss >> sPreviewSplitPosition;
            if (sPreviewSplitPosition < 0.05f) sPreviewSplitPosition = 0.05f;
            if (sPreviewSplitPosition > 0.95f) sPreviewSplitPosition = 0.95f;
        } else if (key == "previewDivider") {
            iss >> sShowSplitDivider;
        } else if (key == "maxFps") {
            int fps = 0;
            iss >> fps;
            if (fps > 0) {
                fpsLimiterEnabled = 1;
                maxFps = (float)fps;
            } else {
                fpsLimiterEnabled = 0;
            }
        } else if (key == "cinemaVeriteEnabled") {
            iss >> controlCinemaVeriteEnabled;
        } else if (key == "cinemaVeriteDisableTime") {
            iss >> disableCinemaVeriteTime;
        }
    }
    file.close();
    return true;
}

void MigrateLegacyConfigIfNeeded(void)
{
    EnsureProfilesDirectoryExists();

    std::string prefsPath = GetPrefsFilePath();
    struct stat st;
    bool hasPrefs = (stat(prefsPath.c_str(), &st) == 0);
    bool hasLegacyIni = (stat(CONFIG_PATH, &st) == 0);

    if (!hasPrefs) {
        // First run of v2.0!
        if (hasLegacyIni) {
            // Read legacy values first to capture maxFps, cinemaVerite, and color values:
            LoadSettingsFromFile(CONFIG_PATH);

            // Move legacy file into profiles/:
            std::string destProfile = std::string(PROFILES_PATH) + "/blu_fx.ini";
            if (stat(destProfile.c_str(), &st) == 0) {
                // Collision guard: if blu_fx.ini already exists in profiles, backup:
                destProfile = std::string(PROFILES_PATH) + "/blu_fx_backup.ini";
                sActiveProfileName = "blu_fx_backup.ini";
            } else {
                sActiveProfileName = "blu_fx.ini";
            }

            // Copy settings into profiles/ while preserving legacy blu_fx.ini for downgrade safety:
            SaveSettingsToFile(destProfile);
            SavePreferences();
            XPLMDebugString("Blu-FX: Migrated settings to profiles/ (preserved root blu_fx.ini) and created Output/preferences/blu_fx.prf\n");
        } else {
            // Fresh install with no legacy file:
            sActiveProfileName = "blu_fx.ini";
            std::string destProfile = std::string(PROFILES_PATH) + "/blu_fx.ini";
            if (stat(destProfile.c_str(), &st) != 0) {
                SaveSettingsToFile(destProfile);
            }
            SavePreferences();
            XPLMDebugString("Blu-FX: Created default Output/preferences/blu_fx.prf\n");
        }
    } else {
        // Preferences file already exists.
        // Check for manual "muscle memory" drop into root folder:
        if (hasLegacyIni) {
            std::string destProfile = std::string(PROFILES_PATH) + "/blu_fx.ini";
            if (stat(destProfile.c_str(), &st) == 0) {
                destProfile = std::string(PROFILES_PATH) + "/blu_fx_imported.ini";
                sActiveProfileName = "blu_fx_imported.ini";
            } else {
                sActiveProfileName = "blu_fx.ini";
            }

            SaveSettingsToFile(destProfile);
            SavePreferences();
            XPLMDebugString("Blu-FX: Adopted root blu_fx.ini into profiles/\n");
        }
    }
}

// saves current color grading adjustments to a specified profile file
bool SaveSettingsToFile(const std::string& path)
{
    EnsureProfilesDirectoryExists();
    std::fstream file;
    file.open(path.c_str(), std::ios_base::out | std::ios_base::trunc);

    if (file.is_open())
    {
        file << "brightness=" << std::fixed << std::setprecision(2) << brightness << std::endl;
        file << "contrast=" << std::fixed << std::setprecision(2) << contrast << std::endl;
        file << "saturation=" << std::fixed << std::setprecision(2) << saturation << std::endl;
        file << "redScale=" << std::fixed << std::setprecision(2) << redScale << std::endl;
        file << "greenScale=" << std::fixed << std::setprecision(2) << greenScale << std::endl;
        file << "blueScale=" << std::fixed << std::setprecision(2) << blueScale << std::endl;
        file << "redOffset=" << std::fixed << std::setprecision(2) << redOffset << std::endl;
        file << "greenOffset=" << std::fixed << std::setprecision(2) << greenOffset << std::endl;
        file << "blueOffset=" << std::fixed << std::setprecision(2) << blueOffset << std::endl;
        file << "vignette=" << std::fixed << std::setprecision(2) << vignette << std::endl;
        file << "raleighScale=" << std::fixed << std::setprecision(1) << raleighScale << std::endl;

        file.close();

        // Optional legacy mirror to root blu_fx.ini for v1.x downgrade safety:
        if (sMirrorLegacyIni && path != CONFIG_PATH) {
            std::fstream legFile;
            legFile.open(CONFIG_PATH, std::ios_base::out | std::ios_base::trunc);
            if (legFile.is_open()) {
                legFile << "# BLU-fx v1.x Legacy Mirror (Auto-generated from active profile: " << sActiveProfileName << ")" << std::endl;
                legFile << "brightness=" << std::fixed << std::setprecision(2) << brightness << std::endl;
                legFile << "contrast=" << std::fixed << std::setprecision(2) << contrast << std::endl;
                legFile << "saturation=" << std::fixed << std::setprecision(2) << saturation << std::endl;
                legFile << "redScale=" << std::fixed << std::setprecision(2) << redScale << std::endl;
                legFile << "greenScale=" << std::fixed << std::setprecision(2) << greenScale << std::endl;
                legFile << "blueScale=" << std::fixed << std::setprecision(2) << blueScale << std::endl;
                legFile << "redOffset=" << std::fixed << std::setprecision(2) << redOffset << std::endl;
                legFile << "greenOffset=" << std::fixed << std::setprecision(2) << greenOffset << std::endl;
                legFile << "blueOffset=" << std::fixed << std::setprecision(2) << blueOffset << std::endl;
                legFile << "vignette=" << std::fixed << std::setprecision(2) << vignette << std::endl;
                legFile << "raleighScale=" << std::fixed << std::setprecision(1) << raleighScale << std::endl;
                legFile << "maxFps=" << (int)maxFps << std::endl;
                legFile << "cinemaVeriteEnabled=" << controlCinemaVeriteEnabled << std::endl;
                legFile << "cinemaVeriteDisableTime=" << std::fixed << std::setprecision(1) << disableCinemaVeriteTime << std::endl;
                legFile.close();
            }
        }

        return true;
    }
    return false;
}

// loads color grading adjustments from a specified file (profile or legacy)
bool LoadSettingsFromFile(const std::string& path)
{
    ClearPrePresetScratchpad();

    std::ifstream file;
    file.open(path.c_str());

    if(file.is_open())
    {
        std::string line;

        while(getline(file, line))
        {
            if (line.empty() || line[0] == '#' || line[0] == ';') continue;
            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;

            std::string key = line.substr(0, eq);
            std::string val = line.substr(eq + 1);

            while (!key.empty() && (key.back() == ' ' || key.back() == '\t' || key.back() == '\r')) key.pop_back();
            while (!val.empty() && (val.front() == ' ' || val.front() == '\t')) val.erase(0, 1);
            while (!val.empty() && (val.back() == ' ' || val.back() == '\t' || val.back() == '\r')) val.pop_back();

            std::istringstream iss(val);

            if(key == "brightness")
                iss >> brightness;
            else if(key == "contrast")
                iss >> contrast;
            else if(key == "saturation")
                iss >> saturation;
            else if(key == "redScale")
                iss >> redScale;
            else if(key == "greenScale")
                iss >> greenScale;
            else if(key == "blueScale")
                iss >> blueScale;
            else if(key == "redOffset")
                iss >> redOffset;
            else if(key == "greenOffset")
                iss >> greenOffset;
            else if(key == "blueOffset")
                iss >> blueOffset;
            else if(key == "vignette")
                iss >> vignette;
            else if(key == "raleighScale")
                iss >> raleighScale;
            else if(key == "postProcesssingEnabled" || key == "postProcessingEnabled")
                iss >> postProcesssingEnabled;
            else if(key == "fpsLimiterEnabled")
                iss >> fpsLimiterEnabled;
            else if(key == "controlCinemaVeriteEnabled" || key == "cinemaVeriteEnabled")
                iss >> controlCinemaVeriteEnabled;
            else if(key == "maxFps")
                iss >> maxFps;
            else if(key == "disableCinemaVeriteTime" || key == "cinemaVeriteDisableTime")
                iss >> disableCinemaVeriteTime;
        }

        file.close();
        UpdateRaleighScale(0);
        return true;
    }
    return false;
}

// saves current settings to active profile in profiles/ and updates blu_fx.prf
void SaveSettings(void)
{
    if (!sActiveProfileName.empty() && sActiveProfileName.rfind("preset:", 0) != 0) {
        std::string fullPath = std::string(PROFILES_PATH) + "/" + sActiveProfileName;
        SaveSettingsToFile(fullPath);
    }
    SavePreferences();
    UpdateDiskSnapshot();
}

// loads startup settings: migrates if needed, loads .prf, then loads active profile
void LoadSettings(void)
{
    EnsureProfilesDirectoryExists();
    MigrateLegacyConfigIfNeeded();
    LoadPreferences();

    bool loadedFromPreset = false;
    if (sActiveProfileName.rfind("preset:", 0) == 0) {
        std::string presetName = sActiveProfileName.substr(7);
        for (int i = 0; i < PRESET_MAX; ++i) {
            if (presetName == BLUfxPresetNames[i]) {
                ApplyPreset(i);
                loadedFromPreset = true;
                break;
            }
        }
    }

    if (!loadedFromPreset) {
        std::string profilePath;
        if (!sActiveProfileName.empty()) {
            profilePath = std::string(PROFILES_PATH) + "/" + sActiveProfileName;
        }

        struct stat st;
        if (profilePath.empty() || stat(profilePath.c_str(), &st) != 0) {
            // Fallback: look for blu_fx.ini in profiles:
            profilePath = std::string(PROFILES_PATH) + "/blu_fx.ini";
            if (stat(profilePath.c_str(), &st) == 0) {
                sActiveProfileName = "blu_fx.ini";
            } else {
                ApplyPreset(PRESET_DEFAULT);
                sActiveProfileName.clear();
            }
        }

        if (!profilePath.empty() && stat(profilePath.c_str(), &st) == 0) {
            LoadSettingsFromFile(profilePath);
        }
    }

    // saves the initial configuration throughout the session
    static bool sIsFirstLoad = true;
    if (sIsFirstLoad) {
        sIsFirstLoad = false;
        sSessionInitialProfile = sActiveProfileName;

        // copies to User preset attached to Restore button
        BLUfxPresets[PRESET_USER].brightness = brightness;
        BLUfxPresets[PRESET_USER].contrast = contrast;
        BLUfxPresets[PRESET_USER].saturation = saturation;
        BLUfxPresets[PRESET_USER].redScale = redScale;
        BLUfxPresets[PRESET_USER].greenScale = greenScale;
        BLUfxPresets[PRESET_USER].blueScale = blueScale;
        BLUfxPresets[PRESET_USER].redOffset = redOffset;
        BLUfxPresets[PRESET_USER].greenOffset = greenOffset;
        BLUfxPresets[PRESET_USER].blueOffset = blueOffset;
        BLUfxPresets[PRESET_USER].vignette = vignette;
        BLUfxPresets[PRESET_USER].raleighScale = raleighScale;
        BLUfxPresets[PRESET_USER].maxFps = maxFps;
        BLUfxPresets[PRESET_USER].disableCinemaVeriteTime = disableCinemaVeriteTime;
    }

    UpdateDiskSnapshot();
}

static void ToggleSettingsWindow()
{
    if (!g_SettingsWindow) {
        g_SettingsWindow = std::make_unique<SettingsImgWindow>();
    }
    bool isVisible = g_SettingsWindow->GetVisible();
    if (isVisible) {
        g_SettingsWindow->RequestClose();
    } else {
        g_SettingsWindow->SetVisible(true);
    }
    UpdateMenuCheckmarks();
}

int ToggleUserGuideCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void *inRefcon)
{
    if (inPhase == xplm_CommandBegin) {
        DocViewerImgWindow::ToggleInstance();
    }
    return 0;
}

// handles the menu-entries (fallback for when XPLMAppendMenuItemWithCommand is not used)
void MenuHandlerCallback(void *inMenuRef, void *inItemRef)
{
    if ((intptr_t) inItemRef == 0) {
        ToggleSettingsWindow();
    } else if ((intptr_t) inItemRef == 1) {
        SetPostProcessingEnabled(!postProcesssingEnabled);
        SavePreferences();
    } else if ((intptr_t) inItemRef == 2) {
        DocViewerImgWindow::ToggleInstance();
    }
}

static void DrawWindow(XPLMWindowID inWindowID, void *inRefcon)
{
}

static void HandleKey(XPLMWindowID inWindowID, char inKey, XPLMKeyFlags inFlags, char inVirtualKey, void *inRefcon, int losingFocus)
{
}

static int HandleMouseClick(XPLMWindowID inWindowID, int x, int y, XPLMMouseStatus inMouse, void *inRefcon)
{
    lastMouseUsageTime = XPLMGetElapsedTime();

    return 0;
}

static XPLMCursorStatus HandleCursor(XPLMWindowID inWindowID, int x, int y, void *inRefcon)
{
    static int lastX = x, lastY = y;

    if (x != lastX || y != lastY)
    {
        lastMouseUsageTime = XPLMGetElapsedTime();
        lastX = x;
        lastY = y;
    }

    return xplm_CursorDefault;
}

static int HandleMouseWheel(XPLMWindowID inWindowID, int x, int y, int wheel, int clicks, void *inRefcon)
{
    lastMouseUsageTime = XPLMGetElapsedTime();

    return 0;
}

int toggleSettingsHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void *inRefcon)
{
    if (inPhase == xplm_CommandBegin) {
        ToggleSettingsWindow();
    }

    return 0;   // 0 = we own and consume this command across all phases
}

int TogglePostProcessingCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void *inRefcon)
{
    if (inPhase == xplm_CommandBegin) {
        SetPostProcessingEnabled(!postProcesssingEnabled);
        SavePreferences();
    }

    return 0;
}

int MomentaryPostProcessingCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void *inRefcon)
{
    if (inPhase == xplm_CommandBegin) {
        SetMomentaryFullView(true);
    } else if (inPhase == xplm_CommandEnd) {
        SetMomentaryFullView(false);
    }

    return 0;
}

PLUGIN_API int XPluginStart(char *outName, char *outSig, char *outDesc)
{
    // set plugin info
    strcpy(outName, NAME_VERSION);
    strcpy(outSig, "de.bwravencl." NAME_LOWERCASE); // TODO: new signature?
#if _DEBUG || DEBUG
    strcpy(outDesc, NAME_VERSION ":dbg built on " __DATE__ ".");
#else
    strcpy(outDesc, NAME_VERSION " built on " __DATE__ ".");
#endif
    
    // Get version of X-Plane:
    xplmVersionDataRef = XPLMFindDataRef("sim/version/xplane_internal_version");
    xplmVersionNum = XPLMGetDatai(xplmVersionDataRef);
    XPLMDebugString(NAME_VERSION ": Initializing " NAME_LOWERCASE " plugin (v" VERSION ", 64-bit):\n");
    XPLMDebugString(NAME_VERSION_BLANK "This updated version supports modern graphics drivers for both\n");
    XPLMDebugString(NAME_VERSION_BLANK "X-Plane 11 and X-Plane 12, and will run on X-Plane 11 in OpenGL\n");
    XPLMDebugString(NAME_VERSION_BLANK "as well as Vulkan/Metal. (Please note: as this binary is 64-bit\n");
    XPLMDebugString(NAME_VERSION_BLANK "only. As such, X-Plane 10 and older XP versions are no longer\n");
    XPLMDebugString(NAME_VERSION_BLANK "supported by " NAME ", starting with v1.1 and later.)\n");
#if APL /* Let users know a little more about the Mac build if they are on macOS: */
    XPLMDebugString(NAME_VERSION ": Special notice for macOS users:\n");
    XPLMDebugString(NAME_VERSION_BLANK "This is a notarized \"universal\" binary, for macOS on both Intel\n");
    XPLMDebugString(NAME_VERSION_BLANK "and Apple Silicon, built by Steve Goldberg (PM @slgoldberg on\n");
    XPLMDebugString(NAME_VERSION_BLANK "X-Plane.org forums). " NAME " v1.0 was created by Matteo Hausner,\n");
    XPLMDebugString(NAME_VERSION_BLANK "with all future updates from v1.1 onward (including this release)\n");
    XPLMDebugString(NAME_VERSION_BLANK "by Steve Goldberg. From both of us: you're welcome!\n");
#else   /* Show this general multi-line "about" message for Linux and Windows builds: */
    XPLMDebugString(NAME_VERSION ": About this release (" NAME_LOWERCASE ".xpl version " VERSION "):\n");
    XPLMDebugString(NAME_VERSION_BLANK "This updated 64-bit version is distributed in the modern X-Plane\n");
    XPLMDebugString(NAME_VERSION_BLANK "universal plugin binary format, and includes feature improvements,\n");
    XPLMDebugString(NAME_VERSION_BLANK "all of which were added by Steve Goldberg (PM @slgoldberg on the\n");
    XPLMDebugString(NAME_VERSION_BLANK "X-Plane.org forums), with the original plugin created by Matteo\n");
    XPLMDebugString(NAME_VERSION_BLANK "Hausner). From both of us: you're welcome!\n");
#endif

    // obtain datarefs
    EnsurePresetsDirectoryExists();
    if (!IS_XP12_1_4)
        cinemaVeriteDataRef = XPLMFindDataRef("sim/graphics/view/cinema_verite");
    
    viewTypeDataRef = XPLMFindDataRef("sim/graphics/view/view_type");

    // register own dataref
    overrideControlCinemaVeriteDataRef = XPLMRegisterDataAccessor(NAME_LOWERCASE "/override_control_cinema_verite", xplmType_Int,  1, GetOverrideControlCinemaVeriteDataRefCallback, SetOverrideControlCinemaVeriteDataRefCallback,  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    // register our commands
    g_ToggleSettingsCmd = XPLMCreateCommand(NAME_LOWERCASE "/toggle_settings", "toggle " NAME " settings window open/closed");
    XPLMRegisterCommandHandler(g_ToggleSettingsCmd, toggleSettingsHandler, 1, NULL);

    g_ToggleUserGuideCmd = XPLMCreateCommand(NAME_LOWERCASE "/user_guide/toggle", "toggle " NAME " User Guide window");
    XPLMRegisterCommandHandler(g_ToggleUserGuideCmd, ToggleUserGuideCommandHandler, 1, NULL);

    g_TogglePostProcessingCmd = XPLMCreateCommand(NAME_LOWERCASE "/post_processing/toggle", "toggle " NAME " post-processing on/off");
    XPLMRegisterCommandHandler(g_TogglePostProcessingCmd, TogglePostProcessingCommandHandler, 1, NULL);

    g_MomentaryPostProcessingCmd = XPLMCreateCommand(NAME_LOWERCASE "/post_processing/momentary", "momentary " NAME " full-screen post-processing preview while held");
    XPLMRegisterCommandHandler(g_MomentaryPostProcessingCmd, MomentaryPostProcessingCommandHandler, 1, NULL);
    
    // create menu-entries
    int subMenuItem = XPLMAppendMenuItem(XPLMFindPluginsMenu(), NAME, 0, 1);
    g_BluFxMenu = XPLMCreateMenu(NAME, XPLMFindPluginsMenu(), subMenuItem, MenuHandlerCallback, 0);
    
    // before adding modern menu option, check to be sure XPLM 301 is available:
    if (XPLMFindSymbol("XPLMAppendMenuItemWithCommand")) {
        g_ToggleSettingsMenuItemIdx = XPLMAppendMenuItemWithCommand(g_BluFxMenu, "Settings window", g_ToggleSettingsCmd);
        g_ToggleUserGuideMenuItemIdx = XPLMAppendMenuItemWithCommand(g_BluFxMenu, "User Guide", g_ToggleUserGuideCmd);
        g_TogglePostProcessingMenuItemIdx = XPLMAppendMenuItemWithCommand(g_BluFxMenu, "Post-processing", g_TogglePostProcessingCmd);
    } else {
        g_ToggleSettingsMenuItemIdx = XPLMAppendMenuItem(g_BluFxMenu, "Settings window", (void*)0, 1);
        g_ToggleUserGuideMenuItemIdx = XPLMAppendMenuItem(g_BluFxMenu, "User Guide", (void*)2, 1);
        g_TogglePostProcessingMenuItemIdx = XPLMAppendMenuItem(g_BluFxMenu, "Post-processing", (void*)1, 1);
    }

    // read and apply config file
    LoadSettings();

    // sync menu check indicator with loaded post-processing preference:
    UpdatePostProcessingMenuState();

    return 1;
}

PLUGIN_API void XPluginStop(void)
{
    if (g_ToggleSettingsCmd) {
        XPLMUnregisterCommandHandler(g_ToggleSettingsCmd, toggleSettingsHandler, 1, NULL);
        g_ToggleSettingsCmd = NULL;
    }
    if (g_ToggleUserGuideCmd) {
        XPLMUnregisterCommandHandler(g_ToggleUserGuideCmd, ToggleUserGuideCommandHandler, 1, NULL);
        g_ToggleUserGuideCmd = NULL;
    }
    if (g_TogglePostProcessingCmd) {
        XPLMUnregisterCommandHandler(g_TogglePostProcessingCmd, TogglePostProcessingCommandHandler, 1, NULL);
        g_TogglePostProcessingCmd = NULL;
    }
    if (g_MomentaryPostProcessingCmd) {
        XPLMUnregisterCommandHandler(g_MomentaryPostProcessingCmd, MomentaryPostProcessingCommandHandler, 1, NULL);
        g_MomentaryPostProcessingCmd = NULL;
    }
    if (g_BluFxMenu) {
        XPLMDestroyMenu(g_BluFxMenu);
        g_BluFxMenu = NULL;
        g_TogglePostProcessingMenuItemIdx = -1;
    }

    // (Note: Removed auto-save on exit so user settings are only saved under explicit user control)
}

PLUGIN_API void XPluginDisable(void)
{
    SavePreferences();
    // Destroy SettingsImgWindow and DocViewerImgWindow, then flush shared font atlas & deferred GPU resources:
    g_SettingsWindow.reset();
    DocViewerImgWindow::DestroyInstance();
    TeardownImGui();

    if (fakeWindow != NULL) {
        XPLMDestroyWindow(fakeWindow);
        fakeWindow = NULL;
    }

    UpdateRaleighScale(1);

    CleanupShader(1);

    // unregister own DataRef
    XPLMUnregisterDataAccessor(overrideControlCinemaVeriteDataRef);

    // unregister flight loop callbacks
    XPLMUnregisterFlightLoopCallback(UpdateFakeWindowCallback, NULL);
    if (fpsLimiterEnabled)
        XPLMUnregisterFlightLoopCallback(LimiterFlightCallback, NULL);
    if (controlCinemaVeriteEnabled)
        XPLMUnregisterFlightLoopCallback(ControlCinemaVeriteCallback, NULL);

    // unregister draw callbacks
    if (postProcesssingEnabled)
        XPLMUnregisterDrawCallback(PostProcessingCallback, xplm_Phase_Window, 1, NULL);
    if (fpsLimiterEnabled)
        XPLMUnregisterDrawCallback(LimiterDrawCallback, xplm_Phase_Terrain, 1, NULL);
}

PLUGIN_API int XPluginEnable(void)
{
    // Initialize shared ImgWindow v2 font atlas & Markdown fonts:
    InitializeImGui();

    // prepare fragment-shader
    // (Note: this was moved from XPluginStart() to minimize risk, especially in XP12.)
    InitShader(FRAGMENT_SHADER);

    // create fake window
    XPLMCreateWindow_t fakeWindowParameters;
    // hack: XPLM300 windows seem to be unable to pass clicks through - the struct size defines which API version is used, by removing the parameters introduced with XPLM300 we can trick X-Plane into thinking we are an XPLM200 plugin for which the click passthrough works
    fakeWindowParameters.structSize = sizeof(fakeWindowParameters) - (sizeof(fakeWindowParameters.decorateAsFloatingWindow) + sizeof(fakeWindowParameters.layer) + sizeof(fakeWindowParameters.handleRightClickFunc));
    XPLMGetScreenBoundsGlobal(&fakeWindowParameters.left, &fakeWindowParameters.top, &fakeWindowParameters.right, &fakeWindowParameters.bottom);
    fakeWindowParameters.visible = 1;
    fakeWindowParameters.drawWindowFunc = DrawWindow;
    fakeWindowParameters.handleKeyFunc = HandleKey;
    fakeWindowParameters.handleMouseClickFunc = HandleMouseClick;
    fakeWindowParameters.handleCursorFunc = HandleCursor;
    fakeWindowParameters.handleMouseWheelFunc = HandleMouseWheel;
    fakeWindowParameters.decorateAsFloatingWindow = xplm_WindowDecorationNone;
    fakeWindowParameters.layer = xplm_WindowLayerFlightOverlay;
    fakeWindowParameters.handleRightClickFunc = HandleMouseClick;
    fakeWindow = XPLMCreateWindowEx(&fakeWindowParameters);
    XPLMSetWindowPositioningMode(fakeWindow, xplm_WindowFullScreenOnAllMonitors, -1);

    // register flight loop callbacks (note: the "fake window" callback is less frequent)
    XPLMRegisterFlightLoopCallback(UpdateFakeWindowCallback, -6, NULL);
    if (fpsLimiterEnabled)
        XPLMRegisterFlightLoopCallback(LimiterFlightCallback, -1, NULL);
    if (controlCinemaVeriteEnabled)
        XPLMRegisterFlightLoopCallback(ControlCinemaVeriteCallback, -1, NULL);

    // register draw callbacks
    if (postProcesssingEnabled)
        XPLMRegisterDrawCallback(PostProcessingCallback, xplm_Phase_Window, 1, NULL);
    if (fpsLimiterEnabled)
        XPLMRegisterDrawCallback(LimiterDrawCallback, xplm_Phase_Terrain, 1, NULL);

    return 1;
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, long inMessage, void *inParam)
{
    if (inMessage == XPLM_MSG_PLANE_LOADED)
        bringFakeWindowToFront = 0;
    else if (inMessage == XPLM_MSG_SCENERY_LOADED)
        UpdateRaleighScale(0);
}
