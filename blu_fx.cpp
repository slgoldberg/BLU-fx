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

#include <cstdio>
#include <cstdlib>
#include <cstring>
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
                        "uniform float shadows;"\
                        "uniform float highlights;"\
                        "uniform float gammaVal;"\
                        "uniform float temperature;"\
                        "uniform float tint;"\
                        "uniform float vibrance;"\
                        "uniform float redScale;"\
                        "uniform float greenScale;"\
                        "uniform float blueScale;"\
                        "uniform float redOffset;"\
                        "uniform float greenOffset;"\
                        "uniform float blueOffset;"\
                        "uniform vec2 resolution;"\
                        "uniform float vignette;"\
                        "uniform float sharpen;"\
                        "uniform float filmGrain;"\
                        "uniform float filmicRolloff;"\
                        "uniform float nightFilter;"\
                        "uniform float frameSeed;"\
                        "uniform sampler2D scene;"\
                        "void main()"\
                        "{"\
                            "vec3 color = texture2D(scene, gl_TexCoord[0].st).rgb;"\
                            "if (sharpen > 0.001) {"\
                                "vec2 px = 1.0 / resolution.xy;"\
                                "vec3 n = texture2D(scene, gl_TexCoord[0].st + vec2(0.0, px.y)).rgb;"\
                                "vec3 s = texture2D(scene, gl_TexCoord[0].st - vec2(0.0, px.y)).rgb;"\
                                "vec3 e = texture2D(scene, gl_TexCoord[0].st + vec2(px.x, 0.0)).rgb;"\
                                "vec3 w = texture2D(scene, gl_TexCoord[0].st - vec2(px.x, 0.0)).rgb;"\
                                "vec3 avg = (n + s + e + w) * 0.25;"\
                                "vec3 minC = min(color, min(min(n, s), min(e, w)));"\
                                "vec3 maxC = max(color, max(max(n, s), max(e, w)));"\
                                "vec3 amp = clamp(min(minC, vec3(1.0) - maxC) / max(maxC, vec3(0.001)), 0.0, 1.0);"\
                                "vec3 sharp = color + (color - avg) * (sharpen * 1.8) * (0.35 + 0.65 * sqrt(amp));"\
                                "color = clamp(sharp, minC * 0.85, min(vec3(1.0), maxC * 1.15));"\
                            "}"\
                            "color *= contrast;"\
                            "color += vec3(brightness, brightness, brightness);"\
                            "if (abs(shadows) > 0.001 || abs(highlights) > 0.001) {"\
                                "vec3 c0 = clamp(color, 0.0, 1.0);"\
                                "float luma = dot(c0, lumCoeff);"\
                                "float shadowMask = (1.0 - luma) * (1.0 - luma);"\
                                "float highMask = luma * luma;"\
                                "color += c0 * (shadows * 0.55 * shadowMask) + vec3(shadows * 0.10 * shadowMask);"\
                                "color += (vec3(1.0) - c0 * 0.35) * (highlights * 0.45 * highMask);"\
                            "}"\
                            "if (abs(gammaVal - 1.0) > 0.001) {"\
                                "color = pow(max(color, vec3(0.0)), vec3(1.0 / max(gammaVal, 0.1)));"\
                            "}"\
                            "if (abs(temperature) > 0.001 || abs(tint) > 0.001) {"\
                                "vec3 wb = vec3(1.0 + temperature * 0.18 + tint * 0.08, 1.0 - tint * 0.14, 1.0 - temperature * 0.18 + tint * 0.08);"\
                                "float origLum = dot(max(color, vec3(0.0)), lumCoeff);"\
                                "color *= wb;"\
                                "float newLum = dot(max(color, vec3(0.0)), lumCoeff);"\
                                "if (newLum > 0.0001) {"\
                                    "color *= mix(1.0, origLum / newLum, 0.65);"\
                                "}"\
                            "}"\
                            "vec3 intensity = vec3(dot(color, lumCoeff));"\
                            "color = mix(intensity, color, saturation);"\
                            "if (abs(vibrance) > 0.001) {"\
                                "vec3 c0 = clamp(color, 0.0, 1.0);"\
                                "float mx = max(c0.r, max(c0.g, c0.b));"\
                                "float mn = min(c0.r, min(c0.g, c0.b));"\
                                "float sat = mx - mn;"\
                                "float vibFactor = 1.0 + vibrance * (1.0 - sat);"\
                                "vec3 vLum = vec3(dot(color, lumCoeff));"\
                                "color = mix(vLum, color, vibFactor);"\
                            "}"\
                            "if (filmicRolloff > 0.5) {"\
                                "const float knee = 0.72;"\
                                "vec3 over = max(color - vec3(knee), vec3(0.0));"\
                                "vec3 rolled = vec3(knee) + over / (vec3(1.0) + over / (1.0 - knee));"\
                                "color = mix(color, rolled, step(vec3(knee), color));"\
                            "}"\
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
                            "if (filmGrain > 0.001) {"\
                                "vec2 gp = gl_FragCoord.xy + vec2(frameSeed * 17.3, frameSeed * 31.7);"\
                                "float gn = fract(sin(dot(gp, vec2(12.9898, 78.233))) * 43758.5453) - 0.5;"\
                                "float gLum = dot(color, lumCoeff);"\
                                "float gMask = 4.0 * gLum * (1.0 - gLum);"\
                                "color = clamp(color + vec3(gn * filmGrain * 0.14 * (0.25 + 0.75 * gMask)), 0.0, 1.0);"\
                            "}"\
                            "if (nightFilter > 0.001) {"\
                                "vec3 nightMul = vec3(1.0 - 0.15 * nightFilter, 1.0 - 0.34 * nightFilter, 1.0 - 0.62 * nightFilter);"\
                                "color *= nightMul;"\
                            "}"\
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

// v2.1 extended adjustments & effects:
float shadows = BLUfxPresets[PRESET_DEFAULT].shadows;
float highlights = BLUfxPresets[PRESET_DEFAULT].highlights;
float gammaVal = BLUfxPresets[PRESET_DEFAULT].gammaVal;
float temperature = BLUfxPresets[PRESET_DEFAULT].temperature;
float tint = BLUfxPresets[PRESET_DEFAULT].tint;
float vibrance = BLUfxPresets[PRESET_DEFAULT].vibrance;
float sharpen = BLUfxPresets[PRESET_DEFAULT].sharpen;
float filmGrain = BLUfxPresets[PRESET_DEFAULT].filmGrain;
int filmicRolloff = BLUfxPresets[PRESET_DEFAULT].filmicRolloff;
int nightFilterEnabled = BLUfxPresets[PRESET_DEFAULT].nightFilterEnabled;
float nightFilterStrength = BLUfxPresets[PRESET_DEFAULT].nightFilterStrength;

// Session-only per-section bypass flags:
bool sBypassTone = false;
bool sBypassRgb = false;
bool sBypassOptics = false;

static inline bool FloatNear(float a, float b, float eps = 0.0005f)
{
    return (a - b < eps) && (b - a < eps);
}

static void CaptureCurrentToPreset(BLUfxPreset& dst)
{
    dst.brightness = brightness;
    dst.contrast = contrast;
    dst.saturation = saturation;
    dst.redScale = redScale;
    dst.greenScale = greenScale;
    dst.blueScale = blueScale;
    dst.redOffset = redOffset;
    dst.greenOffset = greenOffset;
    dst.blueOffset = blueOffset;
    dst.vignette = vignette;
    dst.raleighScale = raleighScale;
    dst.maxFps = maxFps;
    dst.disableCinemaVeriteTime = disableCinemaVeriteTime;
    dst.shadows = shadows;
    dst.highlights = highlights;
    dst.gammaVal = gammaVal;
    dst.temperature = temperature;
    dst.tint = tint;
    dst.vibrance = vibrance;
    dst.sharpen = sharpen;
    dst.filmGrain = filmGrain;
    dst.filmicRolloff = filmicRolloff;
    dst.nightFilterEnabled = nightFilterEnabled;
    dst.nightFilterStrength = nightFilterStrength;
}

static void RestoreFromPreset(const BLUfxPreset& src)
{
    brightness  = src.brightness;
    contrast    = src.contrast;
    saturation  = src.saturation;
    redScale    = src.redScale;
    greenScale  = src.greenScale;
    blueScale   = src.blueScale;
    redOffset   = src.redOffset;
    greenOffset = src.greenOffset;
    blueOffset  = src.blueOffset;
    vignette    = src.vignette;
    shadows     = src.shadows;
    highlights  = src.highlights;
    gammaVal    = src.gammaVal;
    temperature = src.temperature;
    tint        = src.tint;
    vibrance    = src.vibrance;
    sharpen     = src.sharpen;
    filmGrain   = src.filmGrain;
    filmicRolloff = src.filmicRolloff;
    nightFilterEnabled = src.nightFilterEnabled;
    nightFilterStrength = src.nightFilterStrength;
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
            FloatNear(preset->vignette, vignette) &&
            FloatNear(preset->shadows, shadows) &&
            FloatNear(preset->highlights, highlights) &&
            FloatNear(preset->gammaVal, gammaVal) &&
            FloatNear(preset->temperature, temperature) &&
            FloatNear(preset->tint, tint) &&
            FloatNear(preset->vibrance, vibrance) &&
            FloatNear(preset->sharpen, sharpen) &&
            FloatNear(preset->filmGrain, filmGrain) &&
            (preset->filmicRolloff == filmicRolloff) &&
            (preset->nightFilterEnabled == nightFilterEnabled) &&
            (!nightFilterEnabled || FloatNear(preset->nightFilterStrength, nightFilterStrength)));
}

void ApplyPreset(int presetIdx)
{
    if (presetIdx < 0 || presetIdx >= PRESET_MAX)
        return;

    if (presetIdx == PRESET_DEFAULT || presetIdx == PRESET_USER)
        ClearPrePresetScratchpad();

    RestoreFromPreset(BLUfxPresets[presetIdx]);
    if (presetIdx == PRESET_DEFAULT || presetIdx == PRESET_USER) {
        raleighScale = BLUfxPresets[presetIdx].raleighScale;
        if (postProcesssingEnabled || sMomentaryFullView) {
            UpdateRaleighScale(0);
        }
    }
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

    const float effBrightness  = sBypassTone   ? 0.0f : brightness;
    const float effContrast    = sBypassTone   ? 1.0f : contrast;
    const float effSaturation  = sBypassTone   ? 1.0f : saturation;
    const float effShadows     = sBypassTone   ? 0.0f : shadows;
    const float effHighlights  = sBypassTone   ? 0.0f : highlights;
    const float effGamma       = sBypassTone   ? 1.0f : gammaVal;
    const float effTemperature = sBypassTone   ? 0.0f : temperature;
    const float effTint        = sBypassTone   ? 0.0f : tint;
    const float effVibrance    = sBypassTone   ? 0.0f : vibrance;

    const float effRedScale    = sBypassRgb    ? 0.0f : redScale;
    const float effGreenScale  = sBypassRgb    ? 0.0f : greenScale;
    const float effBlueScale   = sBypassRgb    ? 0.0f : blueScale;
    const float effRedOffset   = sBypassRgb    ? 0.0f : redOffset;
    const float effGreenOffset = sBypassRgb    ? 0.0f : greenOffset;
    const float effBlueOffset  = sBypassRgb    ? 0.0f : blueOffset;

    const float effVignette    = sBypassOptics ? 0.0f : vignette;
    const float effSharpen     = sBypassOptics ? 0.0f : sharpen;
    const float effFilmGrain   = sBypassOptics ? 0.0f : filmGrain;
    const float effFilmic      = (sBypassOptics || !filmicRolloff) ? 0.0f : 1.0f;
    const float effNightFilter = (sBypassOptics || !nightFilterEnabled) ? 0.0f : nightFilterStrength;

    static float sGrainFrameCounter = 0.0f;
    if (effFilmGrain > 0.001f) {
        sGrainFrameCounter += 1.0f;
        if (sGrainFrameCounter > 1000.0f) sGrainFrameCounter = 0.0f;
    }

    glUniform1f(glGetUniformLocation(program, "brightness"), effBrightness);
    glUniform1f(glGetUniformLocation(program, "contrast"), effContrast);
    glUniform1f(glGetUniformLocation(program, "saturation"), effSaturation);
    glUniform1f(glGetUniformLocation(program, "shadows"), effShadows);
    glUniform1f(glGetUniformLocation(program, "highlights"), effHighlights);
    glUniform1f(glGetUniformLocation(program, "gammaVal"), effGamma);
    glUniform1f(glGetUniformLocation(program, "temperature"), effTemperature);
    glUniform1f(glGetUniformLocation(program, "tint"), effTint);
    glUniform1f(glGetUniformLocation(program, "vibrance"), effVibrance);

    glUniform1f(glGetUniformLocation(program, "redScale"), effRedScale);
    glUniform1f(glGetUniformLocation(program, "greenScale"), effGreenScale);
    glUniform1f(glGetUniformLocation(program, "blueScale"), effBlueScale);
    glUniform1f(glGetUniformLocation(program, "redOffset"), effRedOffset);
    glUniform1f(glGetUniformLocation(program, "greenOffset"), effGreenOffset);
    glUniform1f(glGetUniformLocation(program, "blueOffset"), effBlueOffset);

    glUniform2f(glGetUniformLocation(program, "resolution"), (float) x, (float) y);
    glUniform1f(glGetUniformLocation(program, "vignette"), effVignette);
    glUniform1f(glGetUniformLocation(program, "sharpen"), effSharpen);
    glUniform1f(glGetUniformLocation(program, "filmGrain"), effFilmGrain);
    glUniform1f(glGetUniformLocation(program, "filmicRolloff"), effFilmic);
    glUniform1f(glGetUniformLocation(program, "nightFilter"), effNightFilter);
    glUniform1f(glGetUniformLocation(program, "frameSeed"), sGrainFrameCounter);
    glUniform1i(glGetUniformLocation(program, "scene"), 0);

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
    bool isSplitActive = (settingsWindowOpen && sPreviewMode == PREVIEW_MODE_SPLIT && !sMomentaryFullView);
    float splitFrac = 0.0f;
    if (isSplitActive) {
        splitFrac = (sPreviewSplitPosition < 0.05f) ? 0.05f : ((sPreviewSplitPosition > 0.95f) ? 0.95f : sPreviewSplitPosition);
    }

    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    if (isSplitActive && sPreviewSplitOrientation == SPLIT_ORIENT_HORIZONTAL) {
        // Horizontal split boundary sits at y * (1 - splitFrac) from bottom (i.e. splitFrac down from top)
        float boundaryFrac = 1.0f - splitFrac;
        if (!sPreviewSplitFlipped) {
            // Raw on top, active BLU-fx grade on bottom (0 .. boundaryFrac)
            glTexCoord2f(0.0f, 0.0f);
            glVertex2f(0.0f, 0.0f);
            glTexCoord2f(0.0f, boundaryFrac);
            glVertex2f(0.0f, (GLfloat)(y * boundaryFrac));
            glTexCoord2f(1.0f, boundaryFrac);
            glVertex2f((GLfloat)x, (GLfloat)(y * boundaryFrac));
            glTexCoord2f(1.0f, 0.0f);
            glVertex2f((GLfloat)x, 0.0f);
        } else {
            // Active BLU-fx grade on top (boundaryFrac .. 1.0), raw on bottom
            glTexCoord2f(0.0f, boundaryFrac);
            glVertex2f(0.0f, (GLfloat)(y * boundaryFrac));
            glTexCoord2f(0.0f, 1.0f);
            glVertex2f(0.0f, (GLfloat)y);
            glTexCoord2f(1.0f, 1.0f);
            glVertex2f((GLfloat)x, (GLfloat)y);
            glTexCoord2f(1.0f, boundaryFrac);
            glVertex2f((GLfloat)x, (GLfloat)(y * boundaryFrac));
        }
    } else if (isSplitActive && sPreviewSplitFlipped) {
        // Vertical split flipped: active BLU-fx grade on left (0 .. splitFrac), raw on right
        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(0.0f, 0.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex2f(0.0f, (GLfloat)y);
        glTexCoord2f(splitFrac, 1.0f);
        glVertex2f((GLfloat)(x * splitFrac), (GLfloat)y);
        glTexCoord2f(splitFrac, 0.0f);
        glVertex2f((GLfloat)(x * splitFrac), 0.0f);
    } else {
        // Vertical split (or full screen when splitFrac == 0): raw on left, active grade on right
        glTexCoord2f(splitFrac, 0.0f);
        glVertex2f((GLfloat)(x * splitFrac), 0.0f);
        glTexCoord2f(splitFrac, 1.0f);
        glVertex2f((GLfloat)(x * splitFrac), (GLfloat)y);
        glTexCoord2f(1.0f, 1.0f);
        glVertex2f((GLfloat)x, (GLfloat)y);
        glTexCoord2f(1.0f, 0.0f);
        glVertex2f((GLfloat)x, 0.0f);
    }
    glEnd();

    // Optional divider hairline + embossed RAW / BLU-fx legend badges at the split boundary:
    bool drawSplitBadges = (isSplitActive && sShowSplitDivider && splitFrac > 0.01f && splitFrac < 0.99f);
    float rawTextX = 0.0f, rawTextY = 0.0f;
    float fxTextX  = 0.0f, fxTextY  = 0.0f;

    if (drawSplitBadges) {
        glUseProgram(0);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glLineWidth(1.0f);
        glBegin(GL_LINES);
        if (sPreviewSplitOrientation == SPLIT_ORIENT_HORIZONTAL) {
            GLfloat divY = (GLfloat)(y * (1.0f - splitFrac));
            glColor4f(0.0f, 0.0f, 0.0f, 0.35f);
            glVertex2f(0.0f, divY + 1.0f);
            glVertex2f((GLfloat)x, divY + 1.0f);
            glColor4f(1.0f, 1.0f, 1.0f, 0.65f);
            glVertex2f(0.0f, divY);
            glVertex2f((GLfloat)x, divY);
        } else {
            GLfloat divX = (GLfloat)(x * splitFrac);
            // Soft dark shadow line immediately to the left for contrast against bright scenes:
            glColor4f(0.0f, 0.0f, 0.0f, 0.35f);
            glVertex2f(divX - 1.0f, 0.0f);
            glVertex2f(divX - 1.0f, (GLfloat)y);
            // Crisp translucent white hairline:
            glColor4f(1.0f, 1.0f, 1.0f, 0.65f);
            glVertex2f(divX, 0.0f);
            glVertex2f(divX, (GLfloat)y);
        }
        glEnd();

        // Measure proportional font labels for RAW and BLU-fx badges:
        int fontH = 10;
        XPLMGetFontDimensions(xplmFont_Proportional, NULL, &fontH, NULL);
        const float rawW = XPLMMeasureString(xplmFont_Proportional, "RAW", 3);
        const float fxW  = XPLMMeasureString(xplmFont_Proportional, "BLU-fx", 6);
        const float padX = 7.0f;
        const float padY = 4.0f;
        const float pillH = (float)fontH + padY * 2.0f;
        const float rawPillW = rawW + padX * 2.0f;
        const float fxPillW  = fxW  + padX * 2.0f;
        const float gapFromLine = 8.0f;

        float rawBoxL = 0.0f, rawBoxB = 0.0f;
        float fxBoxL  = 0.0f, fxBoxB  = 0.0f;

        if (sPreviewSplitOrientation == SPLIT_ORIENT_HORIZONTAL) {
            float divY = (float)y * (1.0f - splitFrac);
            float centerX = (float)x * 0.5f;
            float topB = divY + gapFromLine;
            float botB = divY - gapFromLine - pillH;
            if (topB + pillH > (float)y - 24.0f) topB = (float)y - 24.0f - pillH;
            if (botB < 12.0f) botB = 12.0f;
            if (!sPreviewSplitFlipped) {
                rawBoxL = centerX - rawPillW * 0.5f; rawBoxB = topB;
                fxBoxL  = centerX - fxPillW * 0.5f;  fxBoxB  = botB;
            } else {
                fxBoxL  = centerX - fxPillW * 0.5f;  fxBoxB  = topB;
                rawBoxL = centerX - rawPillW * 0.5f; rawBoxB = botB;
            }
        } else {
            float divX = (float)x * splitFrac;
            float baseB = (float)y - 54.0f - pillH;
            if (baseB < 20.0f) baseB = 20.0f;
            if (!sPreviewSplitFlipped) {
                rawBoxL = divX - gapFromLine - rawPillW; rawBoxB = baseB;
                fxBoxL  = divX + gapFromLine;            fxBoxB  = baseB;
            } else {
                fxBoxL  = divX - gapFromLine - fxPillW;  fxBoxB  = baseB;
                rawBoxL = divX + gapFromLine;            rawBoxB = baseB;
            }
            if (rawBoxL < 8.0f) rawBoxL = 8.0f;
            if (rawBoxL + rawPillW > (float)x - 8.0f) rawBoxL = (float)x - 8.0f - rawPillW;
            if (fxBoxL < 8.0f) fxBoxL = 8.0f;
            if (fxBoxL + fxPillW > (float)x - 8.0f) fxBoxL = (float)x - 8.0f - fxPillW;
        }

        auto drawPillBox = [](float l, float b, float w, float h, float br, float bg, float bb, float ba) {
            // Subtle dark translucent backing plate:
            glColor4f(0.06f, 0.08f, 0.12f, 0.60f);
            glBegin(GL_QUADS);
            glVertex2f(l,     b);
            glVertex2f(l,     b + h);
            glVertex2f(l + w, b + h);
            glVertex2f(l + w, b);
            glEnd();
            // Crisp 1px accent outline:
            glColor4f(br, bg, bb, ba);
            glBegin(GL_LINE_LOOP);
            glVertex2f(l + 0.5f,     b + 0.5f);
            glVertex2f(l + 0.5f,     b + h - 0.5f);
            glVertex2f(l + w - 0.5f, b + h - 0.5f);
            glVertex2f(l + w - 0.5f, b + 0.5f);
            glEnd();
        };

        drawPillBox(rawBoxL, rawBoxB, rawPillW, pillH, 0.78f, 0.82f, 0.88f, 0.45f);
        drawPillBox(fxBoxL,  fxBoxB,  fxPillW,  pillH, 0.32f, 0.74f, 1.00f, 0.80f);

        rawTextX = rawBoxL + padX;
        rawTextY = rawBoxB + padY + 1.0f;
        fxTextX  = fxBoxL  + padX;
        fxTextY  = fxBoxB  + padY + 1.0f;
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glPopAttrib();

    glUseProgram(0);

    if (drawSplitBadges) {
        float rawCol[3] = { 0.86f, 0.89f, 0.94f };
        float fxCol[3]  = { 0.52f, 0.88f, 1.00f };
        XPLMDrawString(rawCol, (int)rawTextX, (int)rawTextY, "RAW", NULL, xplmFont_Proportional);
        XPLMDrawString(fxCol,  (int)fxTextX,  (int)fxTextY,  "BLU-fx", NULL, xplmFont_Proportional);
    }

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
    CaptureCurrentToPreset(sDiskPreset);
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
    CaptureCurrentToPreset(sLastDiscardedPreset);
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
    RestoreFromPreset(sLastDiscardedPreset);
    raleighScale = sLastDiscardedPreset.raleighScale;
    UpdateRaleighScale(0);
}

static BLUfxPreset sPrePresetPreset;
static bool sHasPrePreset = false;

void SavePrePresetScratchpad(void)
{
    CaptureCurrentToPreset(sPrePresetPreset);
    sHasPrePreset = true;
}

void RestorePrePresetScratchpad(void)
{
    if (!sHasPrePreset)
        return;
    RestoreFromPreset(sPrePresetPreset);
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

std::string sActiveProfileName = "";
std::string sStartupActiveProfile = "";
std::string sSessionInitialProfile = "";
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
int sDocViewerTheme = 0;
float sDocViewerOpacity = 0.80f;
int sDocViewerTransparent = 1;
int sPreviewMode = PREVIEW_MODE_SPLIT;
float sPreviewSplitPosition = 0.50f;
int sPreviewSplitOrientation = SPLIT_ORIENT_VERTICAL;
bool sPreviewSplitFlipped = false;
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

static void EnsurePreferencesDirectoryExists(void)
{
    struct stat st;
#if IBM
    if (stat(".\\Output", &st) != 0) _mkdir(".\\Output");
    if (stat(".\\Output\\preferences", &st) != 0) _mkdir(".\\Output\\preferences");
#else
    if (stat("./Output", &st) != 0) mkdir("./Output", 0755);
    if (stat("./Output/preferences", &st) != 0) mkdir("./Output/preferences", 0755);
#endif
}

std::string GetPrefsFilePath(void)
{
    return PREFS_PATH;
}

bool SavePreferences(void)
{
    EnsurePreferencesDirectoryExists();
    std::string path = GetPrefsFilePath();
    FILE* fp = fopen(path.c_str(), "w");
    if (!fp) return false;

    fprintf(fp, "# Blu-FX Application Preferences\n");
    fprintf(fp, "startupProfile=%s\n", sStartupActiveProfile.c_str());
    fprintf(fp, "activeProfile=%s\n", sStartupActiveProfile.c_str());
    fprintf(fp, "postProcessingEnabled=%d\n", postProcesssingEnabled);
    fprintf(fp, "uiMode=%s\n", sUiMode.c_str());
    fprintf(fp, "uiOpacity=%.2f\n", sUiOpacity);
    fprintf(fp, "confirmOnClose=%d\n", sConfirmOnClose);
    fprintf(fp, "mirrorLegacyIni=%d\n", sMirrorLegacyIni);
    fprintf(fp, "windowLeft=%d\n", sWindowLeft);
    fprintf(fp, "windowTop=%d\n", sWindowTop);
    if (sDocViewerLeft >= 0 && sDocViewerTop >= 0 && sDocViewerWidth >= 480 && sDocViewerHeight >= 300) {
        fprintf(fp, "docViewerLeft=%d\n", sDocViewerLeft);
        fprintf(fp, "docViewerTop=%d\n", sDocViewerTop);
        fprintf(fp, "docViewerWidth=%d\n", sDocViewerWidth);
        fprintf(fp, "docViewerHeight=%d\n", sDocViewerHeight);
        fprintf(fp, "docViewerZoom=%.2f\n", sDocViewerZoom);
        fprintf(fp, "docViewerTheme=%d\n", sDocViewerTheme);
        fprintf(fp, "docViewerOpacity=%.2f\n", sDocViewerOpacity);
        fprintf(fp, "docViewerTransparent=%d\n", sDocViewerTransparent);
    }
    fprintf(fp, "previewMode=%s\n", (sPreviewMode == PREVIEW_MODE_FULL ? "full" : "split"));
    fprintf(fp, "previewSplit=%.2f\n", sPreviewSplitPosition);
    fprintf(fp, "previewOrientation=%s\n", (sPreviewSplitOrientation == SPLIT_ORIENT_HORIZONTAL ? "horizontal" : "vertical"));
    fprintf(fp, "previewFlipped=%d\n", (sPreviewSplitFlipped ? 1 : 0));
    fprintf(fp, "previewDivider=%d\n", (sShowSplitDivider ? 1 : 0));
    int effectiveMaxFps = (fpsLimiterEnabled && maxFps > 0.0f) ? (int)maxFps : 0;
    fprintf(fp, "maxFps=%d\n", effectiveMaxFps);
    fprintf(fp, "\n# Legacy Camera Settings (X-Plane 11)\n");
    fprintf(fp, "cinemaVeriteEnabled=%d\n", controlCinemaVeriteEnabled);
    fprintf(fp, "cinemaVeriteDisableTime=%.1f\n", disableCinemaVeriteTime);

    fclose(fp);
    return true;
}

bool LoadPreferences(void)
{
    std::string path = GetPrefsFilePath();
    FILE* fp = fopen(path.c_str(), "r");
    if (!fp) return false;

    char lineBuf[1024];
    while (fgets(lineBuf, sizeof(lineBuf), fp)) {
        char* p = lineBuf;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0' || *p == '#' || *p == ';' || *p == '\r' || *p == '\n') continue;

        char* eq = strchr(p, '=');
        if (!eq) continue;
        *eq = '\0';
        char* key = p;
        char* val = eq + 1;

        // Trim key trailing whitespace:
        char* endKey = key + strlen(key) - 1;
        while (endKey >= key && (*endKey == ' ' || *endKey == '\t' || *endKey == '\r' || *endKey == '\n')) {
            *endKey = '\0';
            endKey--;
        }

        // Trim val leading whitespace:
        while (*val == ' ' || *val == '\t') val++;
        // Trim val trailing whitespace:
        char* endVal = val + strlen(val) - 1;
        while (endVal >= val && (*endVal == ' ' || *endVal == '\t' || *endVal == '\r' || *endVal == '\n')) {
            *endVal = '\0';
            endVal--;
        }

        if (strcmp(key, "startupProfile") == 0) {
            sStartupActiveProfile = val;
            sActiveProfileName = val;
        } else if (strcmp(key, "activeProfile") == 0) {
            if (sStartupActiveProfile.empty() || sStartupActiveProfile == "blu_fx.ini") {
                sStartupActiveProfile = val;
                sActiveProfileName = val;
            }
        } else if (strcmp(key, "postProcessingEnabled") == 0 || strcmp(key, "postProcesssingEnabled") == 0) {
            postProcesssingEnabled = atoi(val);
        } else if (strcmp(key, "uiMode") == 0) {
            sUiMode = val;
        } else if (strcmp(key, "uiOpacity") == 0) {
            sUiOpacity = (float)atof(val);
            if (sUiOpacity < 0.20f) sUiOpacity = 0.20f;
            if (sUiOpacity > 1.00f) sUiOpacity = 1.00f;
        } else if (strcmp(key, "confirmOnClose") == 0) {
            sConfirmOnClose = atoi(val);
        } else if (strcmp(key, "mirrorLegacyIni") == 0) {
            sMirrorLegacyIni = atoi(val);
        } else if (strcmp(key, "windowLeft") == 0 || strcmp(key, "windowX") == 0) {
            sWindowLeft = atoi(val);
        } else if (strcmp(key, "windowTop") == 0 || strcmp(key, "windowY") == 0) {
            sWindowTop = atoi(val);
        } else if (strcmp(key, "docViewerLeft") == 0) {
            sDocViewerLeft = atoi(val);
        } else if (strcmp(key, "docViewerTop") == 0) {
            sDocViewerTop = atoi(val);
        } else if (strcmp(key, "docViewerWidth") == 0) {
            sDocViewerWidth = atoi(val);
        } else if (strcmp(key, "docViewerHeight") == 0) {
            sDocViewerHeight = atoi(val);
        } else if (strcmp(key, "docViewerZoom") == 0) {
            sDocViewerZoom = (float)atof(val);
            if (sDocViewerZoom < 0.50f) sDocViewerZoom = 0.50f;
            if (sDocViewerZoom > 2.50f) sDocViewerZoom = 2.50f;
        } else if (strcmp(key, "docViewerTheme") == 0) {
            sDocViewerTheme = atoi(val);
            if (sDocViewerTheme < 0 || sDocViewerTheme >= 5) sDocViewerTheme = 0;
        } else if (strcmp(key, "docViewerOpacity") == 0) {
            sDocViewerOpacity = (float)atof(val);
            if (sDocViewerOpacity < 0.10f) sDocViewerOpacity = 0.10f;
            if (sDocViewerOpacity > 1.00f) sDocViewerOpacity = 1.00f;
        } else if (strcmp(key, "docViewerTransparent") == 0) {
            sDocViewerTransparent = (atoi(val) != 0) ? 1 : 0;
        } else if (strcmp(key, "previewMode") == 0) {
            sPreviewMode = (strcmp(val, "full") == 0) ? PREVIEW_MODE_FULL : PREVIEW_MODE_SPLIT;
        } else if (strcmp(key, "previewSplit") == 0) {
            sPreviewSplitPosition = (float)atof(val);
            if (sPreviewSplitPosition < 0.05f) sPreviewSplitPosition = 0.05f;
            if (sPreviewSplitPosition > 0.95f) sPreviewSplitPosition = 0.95f;
        } else if (strcmp(key, "previewOrientation") == 0) {
            sPreviewSplitOrientation = (strcmp(val, "horizontal") == 0) ? SPLIT_ORIENT_HORIZONTAL : SPLIT_ORIENT_VERTICAL;
        } else if (strcmp(key, "previewFlipped") == 0) {
            sPreviewSplitFlipped = (atoi(val) != 0);
        } else if (strcmp(key, "previewDivider") == 0) {
            sShowSplitDivider = (atoi(val) != 0);
        } else if (strcmp(key, "maxFps") == 0) {
            int fps = atoi(val);
            if (fps > 0) {
                maxFps = (float)fps;
                SetFpsLimiterEnabled(true);
            } else {
                SetFpsLimiterEnabled(false);
            }
        } else if (strcmp(key, "cinemaVeriteEnabled") == 0) {
            SetControlCinemaVeriteEnabled(atoi(val) != 0);
        } else if (strcmp(key, "cinemaVeriteDisableTime") == 0) {
            disableCinemaVeriteTime = (float)atof(val);
        }
    }
    fclose(fp);
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
        // First run of v2.0 (detected by absence of Output/preferences/blu_fx.prf)!
        std::string destProfile = std::string(PROFILES_PATH) + "/blu_fx.ini";
        if (hasLegacyIni) {
            // If profiles/blu_fx.ini already exists on disk, check whether its color settings differ
            // from the root blu_fx.ini before backing it up to blu_fx_backup.ini:
            if (stat(destProfile.c_str(), &st) == 0) {
                LoadSettingsFromFile(destProfile);
                BLUfxPreset existingProf;
                CaptureCurrentToPreset(existingProf);
                LoadSettingsFromFile(CONFIG_PATH);
                if (!isActivePreset(&existingProf)) {
                    std::string backupProfile = std::string(PROFILES_PATH) + "/blu_fx_backup.ini";
                    rename(destProfile.c_str(), backupProfile.c_str());
                }
            } else {
                // Read legacy values first to capture maxFps, cinemaVerite, and color values:
                LoadSettingsFromFile(CONFIG_PATH);
            }

            sActiveProfileName = "blu_fx.ini";
            sStartupActiveProfile = "blu_fx.ini";

            // Copy settings into profiles/blu_fx.ini while preserving root blu_fx.ini for downgrade safety:
            SaveSettingsToFile(destProfile);
            SavePreferences();
            XPLMDebugString("Blu-FX: Migrated settings to profiles/blu_fx.ini (preserved root blu_fx.ini) and created Output/preferences/blu_fx.prf\n");
        } else if (stat(destProfile.c_str(), &st) == 0) {
            // No root blu_fx.ini, but profiles/blu_fx.ini exists: adopt it as startup profile
            LoadSettingsFromFile(destProfile);
            sActiveProfileName = "blu_fx.ini";
            sStartupActiveProfile = "blu_fx.ini";
            SavePreferences();
            XPLMDebugString("Blu-FX: Adopted existing profiles/blu_fx.ini and created Output/preferences/blu_fx.prf\n");
        } else {
            // Fresh install with no legacy file: start clean with identity defaults and (None) profile
            sActiveProfileName.clear();
            sStartupActiveProfile.clear();
            SavePreferences();
            XPLMDebugString("Blu-FX: Created default Output/preferences/blu_fx.prf\n");
        }
    }
}

// synchronizes active color adjustments to the root blu_fx.ini for legacy v1.x downgrade compatibility
void SyncLegacyIniMirror(void)
{
    if (!sMirrorLegacyIni) return;

    FILE* fp = fopen(CONFIG_PATH, "w");
    if (!fp) return;

    fprintf(fp, "# BLU-fx v1.x Legacy Mirror (Auto-generated from active profile: %s)\n", (sActiveProfileName.empty() ? "Custom" : sActiveProfileName.c_str()));
    fprintf(fp, "brightness=%.2f\n", brightness);
    fprintf(fp, "contrast=%.2f\n", contrast);
    fprintf(fp, "saturation=%.2f\n", saturation);
    fprintf(fp, "redScale=%.2f\n", redScale);
    fprintf(fp, "greenScale=%.2f\n", greenScale);
    fprintf(fp, "blueScale=%.2f\n", blueScale);
    fprintf(fp, "redOffset=%.2f\n", redOffset);
    fprintf(fp, "greenOffset=%.2f\n", greenOffset);
    fprintf(fp, "blueOffset=%.2f\n", blueOffset);
    fprintf(fp, "vignette=%.2f\n", vignette);
    fprintf(fp, "raleighScale=%.1f\n", raleighScale);
    fprintf(fp, "maxFps=%d\n", (int)maxFps);
    fprintf(fp, "cinemaVeriteEnabled=%d\n", controlCinemaVeriteEnabled);
    fprintf(fp, "cinemaVeriteDisableTime=%.1f\n", disableCinemaVeriteTime);
    fclose(fp);
}

// saves current color grading adjustments to a specified profile file
bool SaveSettingsToFile(const std::string& path)
{
    EnsureProfilesDirectoryExists();
    FILE* fp = fopen(path.c_str(), "w");
    if (!fp) return false;

    fprintf(fp, "brightness=%.2f\n", brightness);
    fprintf(fp, "contrast=%.2f\n", contrast);
    fprintf(fp, "saturation=%.2f\n", saturation);
    fprintf(fp, "redScale=%.2f\n", redScale);
    fprintf(fp, "greenScale=%.2f\n", greenScale);
    fprintf(fp, "blueScale=%.2f\n", blueScale);
    fprintf(fp, "redOffset=%.2f\n", redOffset);
    fprintf(fp, "greenOffset=%.2f\n", greenOffset);
    fprintf(fp, "blueOffset=%.2f\n", blueOffset);
    fprintf(fp, "vignette=%.2f\n", vignette);
    fprintf(fp, "raleighScale=%.1f\n", raleighScale);
    fprintf(fp, "shadows=%.2f\n", shadows);
    fprintf(fp, "highlights=%.2f\n", highlights);
    fprintf(fp, "gamma=%.2f\n", gammaVal);
    fprintf(fp, "temperature=%.2f\n", temperature);
    fprintf(fp, "tint=%.2f\n", tint);
    fprintf(fp, "vibrance=%.2f\n", vibrance);
    fprintf(fp, "sharpen=%.2f\n", sharpen);
    fprintf(fp, "filmGrain=%.2f\n", filmGrain);
    fprintf(fp, "filmicRolloff=%d\n", filmicRolloff);
    fprintf(fp, "nightFilterEnabled=%d\n", nightFilterEnabled);
    fprintf(fp, "nightFilterStrength=%.2f\n", nightFilterStrength);
    fclose(fp);

    // Optional legacy mirror to root blu_fx.ini for v1.x downgrade safety:
    if (sMirrorLegacyIni && path != CONFIG_PATH) {
        SyncLegacyIniMirror();
    }

    return true;
}

// loads color grading adjustments from a specified file (profile or legacy)
bool LoadSettingsFromFile(const std::string& path)
{
    ClearPrePresetScratchpad();

    FILE* fp = fopen(path.c_str(), "r");
    if (!fp) return false;

    // Reset v2.1 extended adjustments to neutral defaults before reading so loading older
    // v1.x / v2.0 .ini profiles never retains stale extended settings from a previous profile:
    shadows = 0.0f;
    highlights = 0.0f;
    gammaVal = DEFAULT_GAMMA;
    temperature = 0.0f;
    tint = 0.0f;
    vibrance = 0.0f;
    sharpen = 0.0f;
    filmGrain = 0.0f;
    filmicRolloff = 0;
    nightFilterEnabled = 0;
    nightFilterStrength = DEFAULT_NIGHT_FILTER_STRENGTH;

    char lineBuf[1024];
    while (fgets(lineBuf, sizeof(lineBuf), fp)) {
        char* p = lineBuf;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0' || *p == '#' || *p == ';' || *p == '\r' || *p == '\n') continue;

        char* eq = strchr(p, '=');
        if (!eq) continue;
        *eq = '\0';
        char* key = p;
        char* val = eq + 1;

        // Trim key trailing whitespace:
        char* endKey = key + strlen(key) - 1;
        while (endKey >= key && (*endKey == ' ' || *endKey == '\t' || *endKey == '\r' || *endKey == '\n')) {
            *endKey = '\0';
            endKey--;
        }

        // Trim val leading whitespace:
        while (*val == ' ' || *val == '\t') val++;
        // Trim val trailing whitespace:
        char* endVal = val + strlen(val) - 1;
        while (endVal >= val && (*endVal == ' ' || *endVal == '\t' || *endVal == '\r' || *endVal == '\n')) {
            *endVal = '\0';
            endVal--;
        }

        if (strcmp(key, "brightness") == 0) brightness = (float)atof(val);
        else if (strcmp(key, "contrast") == 0) contrast = (float)atof(val);
        else if (strcmp(key, "saturation") == 0) saturation = (float)atof(val);
        else if (strcmp(key, "redScale") == 0) redScale = (float)atof(val);
        else if (strcmp(key, "greenScale") == 0) greenScale = (float)atof(val);
        else if (strcmp(key, "blueScale") == 0) blueScale = (float)atof(val);
        else if (strcmp(key, "redOffset") == 0) redOffset = (float)atof(val);
        else if (strcmp(key, "greenOffset") == 0) greenOffset = (float)atof(val);
        else if (strcmp(key, "blueOffset") == 0) blueOffset = (float)atof(val);
        else if (strcmp(key, "vignette") == 0) vignette = (float)atof(val);
        else if (strcmp(key, "raleighScale") == 0) raleighScale = (float)atof(val);
        else if (strcmp(key, "shadows") == 0) shadows = (float)atof(val);
        else if (strcmp(key, "highlights") == 0) highlights = (float)atof(val);
        else if (strcmp(key, "gamma") == 0) gammaVal = (float)atof(val);
        else if (strcmp(key, "temperature") == 0) temperature = (float)atof(val);
        else if (strcmp(key, "tint") == 0) tint = (float)atof(val);
        else if (strcmp(key, "vibrance") == 0) vibrance = (float)atof(val);
        else if (strcmp(key, "sharpen") == 0) sharpen = (float)atof(val);
        else if (strcmp(key, "filmGrain") == 0) filmGrain = (float)atof(val);
        else if (strcmp(key, "filmicRolloff") == 0) filmicRolloff = atoi(val) ? 1 : 0;
        else if (strcmp(key, "nightFilterEnabled") == 0) nightFilterEnabled = atoi(val) ? 1 : 0;
        else if (strcmp(key, "nightFilterStrength") == 0) nightFilterStrength = (float)atof(val);
        else if (strcmp(key, "postProcesssingEnabled") == 0 || strcmp(key, "postProcessingEnabled") == 0) postProcesssingEnabled = atoi(val);
        else if (strcmp(key, "fpsLimiterEnabled") == 0) fpsLimiterEnabled = atoi(val);
        else if (strcmp(key, "maxFps") == 0) maxFps = (float)atof(val);
        // Note: Cinema Verite settings (controlCinemaVeriteEnabled / disableCinemaVeriteTime)
        // in legacy .ini files are intentionally ignored in v2.0 so Cinema Verite always starts
        // OFF by default and is controlled exclusively via Output/preferences/blu_fx.prf.
    }

    fclose(fp);
    UpdateRaleighScale(0);
    return true;
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
            // Fallback: look for blu_fx.ini in profiles ONLY IF a profile was requested or blu_fx.ini exists:
            if (!sActiveProfileName.empty()) {
                profilePath = std::string(PROFILES_PATH) + "/blu_fx.ini";
                if (stat(profilePath.c_str(), &st) == 0) {
                    sActiveProfileName = "blu_fx.ini";
                } else {
                    ApplyPreset(PRESET_DEFAULT);
                    sActiveProfileName.clear();
                }
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
        CaptureCurrentToPreset(BLUfxPresets[PRESET_USER]);
    }

    // Downgrade mirror management: ensure root blu_fx.ini exists if mirroring is active,
    // or remove it if user opted out of backward compatibility:
    if (sMirrorLegacyIni) {
        struct stat stRoot;
        if (stat(CONFIG_PATH, &stRoot) != 0) {
            SyncLegacyIniMirror();
            XPLMDebugString("Blu-FX: Materialized missing root blu_fx.ini from active startup profile for downgrade safety\n");
        }
    } else {
        struct stat stRoot;
        if (stat(CONFIG_PATH, &stRoot) == 0) {
            remove(CONFIG_PATH);
        }
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
    // Force native POSIX paths on macOS (no Carbon colons):
    XPLMEnableFeature("XPLM_USE_NATIVE_PATHS", 1);

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
