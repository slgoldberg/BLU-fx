/* Copyright (C) 2018  Matteo Hausner
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

#include "XPLMDataAccess.h"
#include "XPLMDefs.h"

#define XPLM_DEPRECATED     /* for now, unless we want to rewrite lots! */
#include "XPLMDisplay.h"
#undef XPLM_DEPRECATED

#include "XPLMGraphics.h"
#include "XPLMMenus.h"
#include "XPLMPlugin.h"
#include "XPLMProcessing.h"
#include "XPLMUtilities.h"
#include "XPStandardWidgets.h"
#include "XPWidgets.h"

#include <fstream>
#include <sstream>

#if !IBM
#include <string.h>
#include <unistd.h>
#endif

#if APL
#include <OpenGL/gl.h>
#elif IBM
#include "GLee.h"
#pragma comment( lib, "winmm.lib")   
#elif LIN
#include <GL/gl.h>
#endif

// define name
#define NAME "BLU-fx"
#define NAME_BLANK "      "           /* to align multi-line log entries :-) */
#define NAME_LOWERCASE "blu_fx"

// define version
// v1.0 = original release forked by @slgoldberg from github repo
// v1.1 = 64-bit release by @slgoldberg (with Pull Request *denied* by author
//        who has abandoned the original repository; changes included fixing
//        build for "modern" environment (plugin is now 64-bit only), as well
//        as UI improvements including "revert" button so that changes don't
//        automatically get saved to .ini file at quit if user doesn't want to
//        keep it.
// v1.2 = updated release by @slgoldberg to support XP12, arm64/x86 universal
//        build for macOS with both Apple Silicon and Intel, updated CMake
//        configuration to deploy to X-Plane 11.10+ plugin format (though
//        for now, this ignores older platforms since I don't think it would
//        work there anyway), including updating the widget implementation to
//        sit on "modern" XPLM31 windows that obey "UI zoom" scaling, etc.
#define VERSION "1.2"
#define VERSION_BLANK "   "           /* to align multi-line log entries :-) */

// define qualified name that include version (e.g., "BLU-fx v1.2")
#define NAME_VERSION NAME" v" VERSION
#define NAME_VERSION_BLANK NAME_BLANK"  " VERSION_BLANK "  "

// define config file path
#if IBM
#define CONFIG_PATH ".\\Resources\\plugins\\" NAME_LOWERCASE "\\" NAME_LOWERCASE ".ini"
#else
#define CONFIG_PATH "./Resources/plugins/" NAME_LOWERCASE "/" NAME_LOWERCASE ".ini"
#endif

#define DEFAULT_POST_PROCESSING_ENABLED 1
#define DEFAULT_FPS_LIMITER_ENABLED 0
#define DEFAULT_CONTROL_CINEMA_VERITE_ENABLED 0 /* was 1 by default in 32-bit version */
#define DEFAULT_RALEIGH_SCALE 13.0f
#define DEFAULT_MAX_FRAME_RATE 30.0f
#define DEFAULT_DISABLE_CINEMA_VERITE_TIME 5.0f

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
};
typedef BLUfxPreset_t BLUfxPreset;

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

// global settings variables
static int postProcesssingEnabled = DEFAULT_POST_PROCESSING_ENABLED, fpsLimiterEnabled = DEFAULT_FPS_LIMITER_ENABLED, controlCinemaVeriteEnabled = DEFAULT_CONTROL_CINEMA_VERITE_ENABLED;
static float maxFps = DEFAULT_MAX_FRAME_RATE, disableCinemaVeriteTime = DEFAULT_DISABLE_CINEMA_VERITE_TIME;
static float brightness = BLUfxPresets[PRESET_DEFAULT].brightness, contrast = BLUfxPresets[PRESET_DEFAULT].contrast, saturation = BLUfxPresets[PRESET_DEFAULT].saturation, redScale = BLUfxPresets[PRESET_DEFAULT].redScale, greenScale = BLUfxPresets[PRESET_DEFAULT].greenScale, blueScale = BLUfxPresets[PRESET_DEFAULT].blueScale, redOffset = BLUfxPresets[PRESET_DEFAULT].redOffset, greenOffset = BLUfxPresets[PRESET_DEFAULT].greenOffset, blueOffset = BLUfxPresets[PRESET_DEFAULT].blueOffset, vignette = BLUfxPresets[PRESET_DEFAULT].vignette, raleighScale = DEFAULT_RALEIGH_SCALE;

// global internal variables
static int lastResolutionX = 0, lastResolutionY = 0, bringFakeWindowToFront = 0, overrideControlCinemaVerite = 0;
static GLuint textureId = 0, program = 0, fragmentShader = 0;
static float startTimeFlight = 0.0f, endTimeFlight = 0.0f, startTimeDraw = 0.0f, endTimeDraw = 0.0f, lastMouseUsageTime = 0.0f;
static XPLMWindowID fakeWindow = NULL;
static int xplmVersionNum = 0;

// macros for version number relation functionality (i.e., "legacy" or not)
#define IS_XP12         (xplmVersionNum >= 120000)
#define LEGACY_FEATURES (!IS_XP12)

// global dataref variables
static XPLMDataRef cinemaVeriteDataRef = NULL, viewTypeDataRef = NULL, raleighScaleDataRef = NULL, overrideControlCinemaVeriteDataRef = NULL, ignitionKeyDataRef = NULL;
static XPLMDataRef xplmVersionDataRef = XPLMFindDataRef("sim/version/xplane_internal_version");

// global widget variables
static XPWidgetID settingsWidget = NULL, postProcessingCheckbox = NULL, fpsLimiterCheckbox = NULL, controlCinemaVeriteCheckbox = NULL, brightnessCaption = NULL, contrastCaption = NULL, saturationCaption = NULL, redScaleCaption = NULL, greenScaleCaption = NULL, blueScaleCaption = NULL, redOffsetCaption = NULL, greenOffsetCaption = NULL, blueOffsetCaption = NULL, vignetteCaption = NULL, raleighScaleCaption = NULL, maxFpsCaption = NULL, disableCinemaVeriteTimeCaption, brightnessSlider = NULL, contrastSlider = NULL, saturationSlider = NULL, redScaleSlider = NULL, greenScaleSlider = NULL, blueScaleSlider = NULL, redOffsetSlider = NULL, greenOffsetSlider = NULL, blueOffsetSlider = NULL, vignetteSlider = NULL, raleighScaleSlider = NULL, maxFpsSlider = NULL, disableCinemaVeriteTimeSlider = NULL, presetButtons[PRESET_MAX] = {NULL}, resetRaleighScaleButton = NULL, saveButton = NULL, loadButton = NULL;

// draw-callback that adds post-processing
static int PostProcessingCallback(XPLMDrawingPhase inPhase, int inIsBefore, void *inRefcon)
{
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

    glPushAttrib(GL_VIEWPORT_BIT);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0f, x, 0.0f, y, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glViewport(0, 0, x, y);

    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    int settingsWindowOpen = XPIsWidgetVisible(settingsWidget);
    glTexCoord2f((!settingsWindowOpen ? 0.0f : 0.5f), 0.0f);
    glVertex2f((!settingsWindowOpen ? 0.0f : (GLfloat) (x / 2.0f)), 0.0f);
    glTexCoord2f((!settingsWindowOpen ? 0.0f : 0.5f), 1.0f);
    glVertex2f((!settingsWindowOpen ? 0.0f : (GLfloat) (x / 2.0f)), (GLfloat) y);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f((GLfloat) x, (GLfloat) y);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f((GLfloat) x, 0.0f);
    glEnd();

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
    
    // For good measure, also check to make sure the Settings window is not in a bad place:
    // (Note: this allows the user to drag the settings window off the main monitor, but
    // can't actually check to make sure it's still in a legal position if the dimensions
    // of that monitor aren't the same as the main one. However, the window position isn't
    // persistent, so this should be okay in most cases... Otherwise, we could reposition
    // to the default location each time the user hides/re-shows the window.)
    bool isVisible(settingsWidget && XPIsWidgetVisible(settingsWidget));
    bool isDragging(isVisible && XPGetWidgetProperty(settingsWidget, xpProperty_Dragging, NULL));
    if (isVisible && !isDragging) {
        int screenL = 0, screenT = 0, screenR = 0, screenB = 0;
        XPLMGetScreenBoundsGlobal(&screenL, &screenT, &screenR, &screenB);
        
        int l = 0, t = 0, r = 0, b = 0;
        XPGetWidgetGeometry(settingsWidget, &l, &t, &r, &b);
        
        int nl = l, nt = t, nr = r, nb = b;
        int w = r - l;
        int h = t - b;
        
        const int MINVIS = 20;              // minimum remaining window visible
        const int TOPVIS = (MINVIS > 25 ? MINVIS : 25);
        
        // Do the jigger jagger:
        if (l > screenR - MINVIS) {
            nl = screenR - MINVIS;
            nr = nl + w;
        }
        else if (r < screenL + MINVIS) {
            nr = screenL + MINVIS;
            nl = nr - w;
        }
        
        if (t < screenB + MINVIS) {
            nt = screenB + MINVIS;
            nb = nt - h;
        }
        else if (t > screenT - TOPVIS) {
            nt = screenT - TOPVIS;
            nb = nt - h;
        }
                
        // Reposition the window only if necessary based on above:
        if (nl != l || nr != r || nt != t || nb != b)
          XPSetWidgetGeometry(settingsWidget, nl, nt, nr, nb);
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
    // Don't assume this callback should be called, since it could change before the callback is unregistered when the user opts out,
    // and of course also ignore it if the override flag dataref is set:
    if (controlCinemaVeriteEnabled && !overrideControlCinemaVerite)
    {
        if (XPLMGetDatai(viewTypeDataRef) == 1026) // 3D Cockpit
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

// returns a float rounded to two decimal places
static float Round(const float f)
{
    return ((int) (f * 100.0f)) / 100.0f;
}

// sets the raleigh scale dataref to the selected raleigh scale value, passing reset = 1 resets the dataref to its default value
static void UpdateRaleighScale(int reset)
{
    if (!LEGACY_FEATURES)
        return; // return immediately if XP12.00 or later, since there is no Raleigh anymore
    
    if (raleighScaleDataRef == NULL)
        raleighScaleDataRef = XPLMFindDataRef("sim/private/controls/atmo/atmo_scale_raleigh");
    if (raleighScaleDataRef != NULL)
        XPLMSetDataf(raleighScaleDataRef, !reset ? raleighScale : DEFAULT_RALEIGH_SCALE);
}

// updates all caption widgets and slider positions associated with settings variables
static void UpdateSettingsWidgets(void)
{
    XPSetWidgetProperty(postProcessingCheckbox, xpProperty_ButtonState, postProcesssingEnabled);
    XPSetWidgetProperty(fpsLimiterCheckbox, xpProperty_ButtonState, fpsLimiterEnabled);
    XPSetWidgetProperty(controlCinemaVeriteCheckbox, xpProperty_ButtonState, controlCinemaVeriteEnabled);

    char stringBrightness[32];
    snprintf(stringBrightness, 32, "Brightness: %.2f", brightness);
    XPSetWidgetDescriptor(brightnessCaption, stringBrightness);

    char stringContrast[32];
    snprintf(stringContrast, 32, "Contrast: %.2f", contrast);
    XPSetWidgetDescriptor(contrastCaption, stringContrast);

    char stringSaturation[32];
    snprintf(stringSaturation, 32, "Saturation: %.2f", saturation);
    XPSetWidgetDescriptor(saturationCaption, stringSaturation);

    char stringRedScale[32];
    snprintf(stringRedScale, 32, "Red Scale: %.2f", redScale);
    XPSetWidgetDescriptor(redScaleCaption, stringRedScale);

    char stringGreenScale[32];
    snprintf(stringGreenScale, 32, "Green Scale: %.2f", greenScale);
    XPSetWidgetDescriptor(greenScaleCaption, stringGreenScale);

    char stringBlueScale[32];
    snprintf(stringBlueScale, 32, "Blue Scale: %.2f", blueScale);
    XPSetWidgetDescriptor(blueScaleCaption, stringBlueScale);

    char stringRedOffset[32];
    snprintf(stringRedOffset, 32, "Red Offset: %.2f", redOffset);
    XPSetWidgetDescriptor(redOffsetCaption, stringRedOffset);

    char stringGreenOffset[32];
    snprintf(stringGreenOffset, 32, "Green Offset: %.2f", greenOffset);
    XPSetWidgetDescriptor(greenOffsetCaption, stringGreenOffset);

    char stringBlueOffset[32];
    snprintf(stringBlueOffset, 32, "Blue Offset: %.2f", blueOffset);
    XPSetWidgetDescriptor(blueOffsetCaption, stringBlueOffset);

    char stringVignette[32];
    snprintf(stringVignette, 32, "Vignette: %.2f", vignette);
    XPSetWidgetDescriptor(vignetteCaption, stringVignette);

    if (LEGACY_FEATURES) {
        // Raleigh is not a thing in XP12, so this is only for pre-XP12:
        char stringRaleighScale[32];
        snprintf(stringRaleighScale, 32, "Raleigh Scale: %.2f", raleighScale);
        XPSetWidgetDescriptor(raleighScaleCaption, stringRaleighScale);
    }

    char stringMaxFps[32];
    snprintf(stringMaxFps, 32, "Max FPS: %.0f", maxFps);
    XPSetWidgetDescriptor(maxFpsCaption, stringMaxFps);

    char stringDisableCinemaVeriteTime[32];
    snprintf(stringDisableCinemaVeriteTime, 32, "On input disable for: %.0f sec", disableCinemaVeriteTime);
    XPSetWidgetDescriptor(disableCinemaVeriteTimeCaption, stringDisableCinemaVeriteTime);

    XPSetWidgetProperty(brightnessSlider, xpProperty_ScrollBarSliderPosition, (intptr_t) ((brightness + 0.5f) * 1000.0f));
    XPSetWidgetProperty(contrastSlider, xpProperty_ScrollBarSliderPosition, (intptr_t) (contrast * 100.0f));
    XPSetWidgetProperty(saturationSlider, xpProperty_ScrollBarSliderPosition, (intptr_t) (saturation * 100.0f));
    XPSetWidgetProperty(redScaleSlider, xpProperty_ScrollBarSliderPosition, (intptr_t) (redScale * 100.0f));
    XPSetWidgetProperty(greenScaleSlider, xpProperty_ScrollBarSliderPosition, (intptr_t) (greenScale * 100.0f));
    XPSetWidgetProperty(blueScaleSlider, xpProperty_ScrollBarSliderPosition, (intptr_t) (blueScale * 100.0f));
    XPSetWidgetProperty(redOffsetSlider, xpProperty_ScrollBarSliderPosition, (intptr_t) (redOffset * 100.0f));
    XPSetWidgetProperty(greenOffsetSlider, xpProperty_ScrollBarSliderPosition, (intptr_t) (greenOffset * 100.0f));
    XPSetWidgetProperty(blueOffsetSlider, xpProperty_ScrollBarSliderPosition, (intptr_t) (blueOffset * 100.0f));
    XPSetWidgetProperty(vignetteSlider, xpProperty_ScrollBarSliderPosition, (intptr_t) (vignette * 100.0f));
    XPSetWidgetProperty(raleighScaleSlider, xpProperty_ScrollBarSliderPosition, (intptr_t) raleighScale);
    XPSetWidgetProperty(maxFpsSlider, xpProperty_ScrollBarSliderPosition, (intptr_t) (maxFps));
    XPSetWidgetProperty(disableCinemaVeriteTimeSlider, xpProperty_ScrollBarSliderPosition, (intptr_t) (disableCinemaVeriteTime));
}

// saves current settings to the config file
static void SaveSettings(void)
{
    std::fstream file;
    file.open(CONFIG_PATH, std::ios_base::out | std::ios_base::trunc);

    if(file.is_open())
    {
        file << "postProcesssingEnabled=" << postProcesssingEnabled << std::endl;
        file << "fpsLimiterEnabled=" << fpsLimiterEnabled << std::endl;
        file << "controlCinemaVeriteEnabled=" << controlCinemaVeriteEnabled << std::endl;
        file << "brightness=" << brightness << std::endl;
        file << "contrast=" << contrast << std::endl;
        file << "saturation=" << saturation << std::endl;
        file << "redScale=" << redScale << std::endl;
        file << "greenScale=" << greenScale << std::endl;
        file << "blueScale=" << blueScale << std::endl;
        file << "redOffset=" << redOffset << std::endl;
        file << "greenOffset=" << greenOffset << std::endl;
        file << "blueOffset=" << blueOffset << std::endl;
        file << "vignette=" << vignette << std::endl;
        file << "raleighScale=" << raleighScale << std::endl;
        file << "maxFps=" << maxFps << std::endl;
        file << "disableCinemaVeriteTime=" << disableCinemaVeriteTime << std::endl;

        file.close();
    }
}

// loads settings from the config file
static void LoadSettings(void)
{
    std::ifstream file;
    file.open(CONFIG_PATH);

    if(file.is_open())
    {
        std::string line;

        while(getline(file, line))
        {
            std::string val = line.substr(line.find("=") + 1);
            std::istringstream iss(val);

            if(line.find("postProcesssingEnabled") != std::string::npos)
                iss >> postProcesssingEnabled;
            else if(line.find("fpsLimiterEnabled") != std::string::npos)
                iss >> fpsLimiterEnabled;
            else if(line.find("controlCinemaVeriteEnabled") != std::string::npos)
                iss >> controlCinemaVeriteEnabled;
            else if(line.find("brightness") != std::string::npos)
                iss >> brightness;
            else if(line.find("contrast") != std::string::npos)
                iss >> contrast;
            else if(line.find("saturation") != std::string::npos)
                iss >> saturation;
            else if(line.find("redScale") != std::string::npos)
                iss >> redScale;
            else if(line.find("greenScale") != std::string::npos)
                iss >> greenScale;
            else if(line.find("blueScale") != std::string::npos)
                iss >> blueScale;
            else if(line.find("redOffset") != std::string::npos)
                iss >> redOffset;
            else if(line.find("greenOffset") != std::string::npos)
                iss >> greenOffset;
            else if(line.find("blueOffset") != std::string::npos)
                iss >> blueOffset;
            else if(line.find("vignette") != std::string::npos)
                iss >> vignette;
            else if(line.find("raleighScale") != std::string::npos)
                iss >> raleighScale;
            else if(line.find("maxFps") != std::string::npos)
                iss >> maxFps;
            else if(line.find("disableCinemaVeriteTime") != std::string::npos)
                iss >> disableCinemaVeriteTime;
        }

        file.close();
        
        // saves the initial configuration throughout the session
        static bool sIsFirstLoad = true;
        if (sIsFirstLoad) {
            sIsFirstLoad = false;
            
            // copies to User preset attached to Restore button
            BLUfxPresets[PRESET_USER].brightness = brightness;
            BLUfxPresets[PRESET_USER].contrast = contrast;
            BLUfxPresets[PRESET_USER].saturation = saturation;
            BLUfxPresets[PRESET_USER].redScale = redScale;
            BLUfxPresets[PRESET_USER].greenScale = greenScale;
            BLUfxPresets[PRESET_USER].blueScale = blueScale;
            BLUfxPresets[PRESET_USER].redOffset = redOffset;
            BLUfxPresets[PRESET_USER].blueOffset = blueOffset;
            BLUfxPresets[PRESET_USER].vignette = vignette;
            BLUfxPresets[PRESET_USER].raleighScale = raleighScale;
            BLUfxPresets[PRESET_USER].maxFps = maxFps;
            BLUfxPresets[PRESET_USER].disableCinemaVeriteTime = disableCinemaVeriteTime;
        }
    }
}

// handles the settings widget
static int SettingsWidgetHandler(XPWidgetMessage inMessage, XPWidgetID inWidget, long inParam1, long inParam2)
{
    if (inMessage == xpMessage_CloseButtonPushed)
    {
        if (XPIsWidgetVisible(settingsWidget))
        {
            //SaveSettings();   // now explicit save button and save in XPluginStop()
            XPHideWidget(settingsWidget);
        }
    }
    else if (inMessage == xpMsg_ButtonStateChanged)
    {
        if (inParam1 == (long) postProcessingCheckbox)
        {
            postProcesssingEnabled = (int) XPGetWidgetProperty(postProcessingCheckbox, xpProperty_ButtonState, 0);

            if (!postProcesssingEnabled)
            {
                XPLMUnregisterDrawCallback(PostProcessingCallback, xplm_Phase_Window, 1, NULL);
                UpdateRaleighScale(1);      // note: only happens in for pre-XP12 (otherwise no-op)
            }
            else
            {
                XPLMRegisterDrawCallback(PostProcessingCallback, xplm_Phase_Window, 1, NULL);
                UpdateRaleighScale(0);      // note: only happens in for pre-XP12 (otherwise no-op)
            }

        }
        else if (inParam1 == (long) fpsLimiterCheckbox)
        {
            fpsLimiterEnabled = (int) XPGetWidgetProperty(fpsLimiterCheckbox, xpProperty_ButtonState, 0);

            if (!fpsLimiterEnabled)
            {
                XPLMUnregisterFlightLoopCallback(LimiterFlightCallback, NULL);
                XPLMUnregisterDrawCallback(LimiterDrawCallback, xplm_Phase_Terrain, 1, NULL);
            }
            else
            {
                XPLMRegisterFlightLoopCallback(LimiterFlightCallback, -1, NULL);
                XPLMRegisterDrawCallback(LimiterDrawCallback, xplm_Phase_Terrain, 1, NULL);
            }

        }
        else if (inParam1 == (long) controlCinemaVeriteCheckbox)
        {
            controlCinemaVeriteEnabled = (int) XPGetWidgetProperty(controlCinemaVeriteCheckbox, xpProperty_ButtonState, 0);

            if (!controlCinemaVeriteEnabled)
                XPLMUnregisterFlightLoopCallback(ControlCinemaVeriteCallback, NULL);
            else
                XPLMRegisterFlightLoopCallback(ControlCinemaVeriteCallback, -1, NULL);

        }
    }
    else if (inMessage == xpMsg_ScrollBarSliderPositionChanged)
    {
#define minMax(a,b,c) (std::min((std::max((a), (b))),(c)))
        if (inParam1 == (long) brightnessSlider)
            brightness = minMax(-0.5f, Round(XPGetWidgetProperty(brightnessSlider, xpProperty_ScrollBarSliderPosition, 0) / 1000.0f) - 0.5f, 0.5f);
        else if (inParam1 == (long) contrastSlider)
            contrast = Round(XPGetWidgetProperty(contrastSlider, xpProperty_ScrollBarSliderPosition, 0) / 100.0f);
        else if (inParam1 == (long) saturationSlider)
            saturation = Round(XPGetWidgetProperty(saturationSlider, xpProperty_ScrollBarSliderPosition, 0) / 100.0f);
        else if (inParam1 == (long) redScaleSlider)
            redScale = Round(XPGetWidgetProperty(redScaleSlider, xpProperty_ScrollBarSliderPosition, 0) / 100.0f);
        else if (inParam1 == (long) greenScaleSlider)
            greenScale = Round(XPGetWidgetProperty(greenScaleSlider, xpProperty_ScrollBarSliderPosition, 0) / 100.0f);
        else if (inParam1 == (long) blueScaleSlider)
            blueScale = Round(XPGetWidgetProperty(blueScaleSlider, xpProperty_ScrollBarSliderPosition, 0) / 100.0f);
        else if (inParam1 == (long) redOffsetSlider)
            redOffset = Round(XPGetWidgetProperty(redOffsetSlider, xpProperty_ScrollBarSliderPosition, 0) / 100.0f);
        else if (inParam1 == (long) greenOffsetSlider)
            greenOffset = Round(XPGetWidgetProperty(greenOffsetSlider, xpProperty_ScrollBarSliderPosition, 0) / 100.0f);
        else if (inParam1 == (long) blueOffsetSlider)
            blueOffset = Round(XPGetWidgetProperty(blueOffsetSlider, xpProperty_ScrollBarSliderPosition, 0) / 100.0f);
        else if (inParam1 == (long) vignetteSlider)
            vignette = Round(XPGetWidgetProperty(vignetteSlider, xpProperty_ScrollBarSliderPosition, 0) / 100.0f);
        else if (inParam1 == (long) raleighScaleSlider)
        {
            raleighScale = Round((float) XPGetWidgetProperty(raleighScaleSlider, xpProperty_ScrollBarSliderPosition, 0));
            UpdateRaleighScale(0);      // note: only happens in for pre-XP12 (otherwise no-op)
        }
        else if (inParam1 == (long) maxFpsSlider)
            maxFps = (float) (int) XPGetWidgetProperty(maxFpsSlider, xpProperty_ScrollBarSliderPosition, 0);
        else if (inParam1 == (long) disableCinemaVeriteTimeSlider)
            disableCinemaVeriteTime = (float) (int) XPGetWidgetProperty(disableCinemaVeriteTimeSlider, xpProperty_ScrollBarSliderPosition, 0);

        UpdateSettingsWidgets();
    }
    else if (inMessage == xpMsg_PushButtonPressed)
    {
        if (inParam1 == (long) resetRaleighScaleButton)
        {
            raleighScale = DEFAULT_RALEIGH_SCALE;
            UpdateRaleighScale(1);
        }
        else if (inParam1 == (long) loadButton)
        {
            LoadSettings();
        }
        else if (inParam1 == (long) saveButton)
        {
            SaveSettings();
        }
        else
        {
            int i;
            for (i=0; i < PRESET_MAX; i++)
            {
                if ((long) presetButtons[i] == (long) inParam1)
                {
                    brightness = BLUfxPresets[i].brightness;
                    contrast = BLUfxPresets[i].contrast;
                    saturation = BLUfxPresets[i].saturation;
                    redScale = BLUfxPresets[i].redScale;
                    greenScale = BLUfxPresets[i].greenScale;
                    blueScale = BLUfxPresets[i].blueScale;
                    redOffset = BLUfxPresets[i].redOffset;
                    greenOffset = BLUfxPresets[i].greenOffset;
                    blueOffset = BLUfxPresets[i].blueOffset;
                    vignette = BLUfxPresets[i].vignette;
#ifdef INCLUDE_SETTINGS_IN_PRESETS  /* not really loaded, except for reload .ini */
                    raleighScale = BLUfxPresets[i].raleighScale;
                    maxFps = BLUfxPresets[i].maxFps;
                    disableCinemaVeriteTime = BLUfxPresets[i].disableCinemaVeriteTime;
#endif

                    break;
                }
            }
        }

        UpdateSettingsWidgets();
    }

    return 0;
}

// handles the menu-entries
void MenuHandlerCallback(void *inMenuRef, void *inItemRef)
{
    // settings menu entry
    if ((long) inItemRef == 0)
    {
        if (settingsWidget == NULL)
        {
            // create settings widget
            int x = 10, y = 0, w = 370, h = 783;
            
            // get screen bounds:
            int screenLeft = 0, screenTop = 0, screenRight = 0, screenBottom = 0;
            XPLMGetScreenBoundsGlobal(&screenLeft, &screenTop, &screenRight, &screenBottom);
            int screenWidth = (screenRight - screenLeft);
            int screenHeight = (screenTop - screenBottom);

            y = screenTop - 30;     // start near the top, but give some space

            // Similarly, account for hiding the whole Raleigh slider section
            // for anything XP12.0 or later, since it doesn't make sense to
            // do that anyore (but we still want it to work for XP11):
            // (Note: we *could* replace the Raleigh adjustment with another
            // adjustment either to lighting or something else, but it won't
            // be the same as Raleigh when applied to XP12, no matter what.)
            if (!LEGACY_FEATURES) {
                y = std::min(y - 25, screenTop - 25);
                h -= 60;
            }
            
            int x2 = x + w;
            int y2 = y - h;
            
            // widget window (title will be e.g., "BLU-fx v1.2 - Settings")
            const int windowStyle = xpMainWindowStyle_Translucent;  // xpWidgetClass_MainWindow;
            settingsWidget = XPCreateWidget(x, y, x2, y2, 1, NAME " v" VERSION " - Settings", 1, 0, windowStyle);

            // add close box (which requires setting "MainWindowType" to honor translucent style above)
            XPSetWidgetProperty(settingsWidget, xpProperty_MainWindowType, 1);  // honor translucent style, if active
            XPSetWidgetProperty(settingsWidget, xpProperty_MainWindowHasCloseBoxes, 1);
            
            // add post-processing sub window
            y += 9;
            XPCreateWidget(x + 10, y - 30, x2 - 10, y - 516 - 10, 1, "Post-Processing Settings:", 0, settingsWidget, xpWidgetClass_SubWindow);

            // Add small left/right margin for inner content:
            x += 3;
            x2 -= 3;
            
            y -= 3;
            
            // add post-processing settings caption
            XPCreateWidget(x + 10, y - 30, x2 - 20, y - 45, 1, "Post-Processing Settings:", 0, settingsWidget, xpWidgetClass_Caption);

            {
                x2 -= 30;    // slide whole attribution plate left a little
                y -= 1;

                // Add backdrop around version attribution:
                XPCreateWidget(x2 - 40 - 70 - 2, y - 29, x2 - 15 + 2, y - 60 - 15 - 4, 1, "version_backdrop", 0, settingsWidget, xpWidgetClass_SubWindow);
            
                // add version attribution caption
                XPCreateWidget(x2 - 33 - 70, y - 30, x2 - 15, y - 45, 1, "64-bit version", 0, settingsWidget, xpWidgetClass_Caption);
                XPCreateWidget(x2 - 33 - 70, y - 30, x2 - 15, y - 45, 1, "64-bit version", 0, settingsWidget, xpWidgetClass_Caption);   // faux bold (overstrike)
                XPCreateWidget(x2 - 33 - 64, y - 45, x2 - 15, y - 45 - 15, 1, "2019-2024", 0, settingsWidget, xpWidgetClass_Caption);
                XPCreateWidget(x2 - 33 - 74, y - 60, x2 - 15, y - 60 - 15, 1, "by @slgoldberg", 0, settingsWidget, xpWidgetClass_Caption);
                
                x2 += 30;    // restore margin after above slide-over
                y += 1;
            }

            // add post-processing checkbox         // ------------------v (is it here)? (at the "v")
            postProcessingCheckbox = XPCreateWidget(x + 5, y - 60, x2 - 35, y - 75, 1, "Enable Post-Processing", 0, settingsWidget, xpWidgetClass_Button);
            XPSetWidgetProperty(postProcessingCheckbox, xpProperty_ButtonType, xpRadioButton);
            XPSetWidgetProperty(postProcessingCheckbox, xpProperty_ButtonBehavior, xpButtonBehaviorCheckBox);
            
            y += 3;
            
            // add brightness caption
            char stringBrightness[32];
            snprintf(stringBrightness, 32, "Brightness: %.2f", brightness);
            brightnessCaption = XPCreateWidget(x + 30, y - 90 + 3, x2 - 50, y - 105 + 3, 1, stringBrightness, 0, settingsWidget, xpWidgetClass_Caption);

            // add brightness slider
            brightnessSlider = XPCreateWidget(x + 195, y - 90, x2 - 15, y - 105, 1, "Brightness", 0, settingsWidget, xpWidgetClass_ScrollBar);
            XPSetWidgetProperty(brightnessSlider, xpProperty_ScrollBarMin, 1);
            XPSetWidgetProperty(brightnessSlider, xpProperty_ScrollBarMax, 1000);
            XPSetWidgetProperty(brightnessSlider, xpProperty_ScrollBarPageAmount, 50);   // 50 units = 0.05 increase/decrease

            y += 3;
            
            // add contrast caption
            char stringContrast[32];
            snprintf(stringContrast, 32, "Contrast: %.2f", contrast);
            contrastCaption = XPCreateWidget(x + 30, y - 110, x2 - 50, y - 125, 1, stringContrast, 0, settingsWidget, xpWidgetClass_Caption);

            // add contrast slider
            contrastSlider = XPCreateWidget(x + 195, y - 110, x2 - 15, y - 125, 1, "Contrast", 0, settingsWidget, xpWidgetClass_ScrollBar);
            XPSetWidgetProperty(contrastSlider, xpProperty_ScrollBarMin, 5);
            XPSetWidgetProperty(contrastSlider, xpProperty_ScrollBarMax, 200);

            y += 3;
            
            // add saturation caption
            char stringSaturation[32];
            snprintf(stringSaturation, 32, "Saturation: %.2f", saturation);
            saturationCaption = XPCreateWidget(x + 30, y - 130, x2 - 50, y - 145, 1, stringSaturation, 0, settingsWidget, xpWidgetClass_Caption);

            // add saturation slider
            saturationSlider = XPCreateWidget(x + 195, y - 130, x2 - 15, y - 145, 1, "Saturation", 0, settingsWidget, xpWidgetClass_ScrollBar);
            XPSetWidgetProperty(saturationSlider, xpProperty_ScrollBarMin, 0);
            XPSetWidgetProperty(saturationSlider, xpProperty_ScrollBarMax, 250);

            y += 3;
            
            // add red scale caption
            char stringRedScale[32];
            snprintf(stringRedScale, 32, "Red Scale: %.2f", redScale);
            redScaleCaption = XPCreateWidget(x + 30, y - 150, x2 - 50, y - 165, 1, stringRedScale, 0, settingsWidget, xpWidgetClass_Caption);

            // add red scale slider
            redScaleSlider = XPCreateWidget(x + 195, y - 150, x2 - 15, y - 165, 1, "Red Scale", 0, settingsWidget, xpWidgetClass_ScrollBar);
            XPSetWidgetProperty(redScaleSlider, xpProperty_ScrollBarMin, -75);
            XPSetWidgetProperty(redScaleSlider, xpProperty_ScrollBarMax, 75);

            y += 3;
            
            // add green scale caption
            char stringGreenScale[32];
            snprintf(stringGreenScale, 32, "Green Scale: %.2f", greenScale);
            greenScaleCaption = XPCreateWidget(x + 30, y - 170, x2 - 50, y - 185, 1, stringGreenScale, 0, settingsWidget, xpWidgetClass_Caption);

            // add green scale slider
            greenScaleSlider = XPCreateWidget(x + 195, y - 170, x2 - 15, y - 185, 1, "Green Scale", 0, settingsWidget, xpWidgetClass_ScrollBar);
            XPSetWidgetProperty(greenScaleSlider, xpProperty_ScrollBarMin, -75);
            XPSetWidgetProperty(greenScaleSlider, xpProperty_ScrollBarMax, 75);

            y += 3;
            
            // add blue scale caption
            char stringBlueScale[32];
            snprintf(stringBlueScale, 32, "Blue Scale: %.2f", blueScale);
            blueScaleCaption = XPCreateWidget(x + 30, y - 190, x2 - 50, y - 205, 1, stringBlueScale, 0, settingsWidget, xpWidgetClass_Caption);

            // add blue scale slider
            blueScaleSlider = XPCreateWidget(x + 195, y - 190, x2 - 15, y - 205, 1, "Blue Scale", 0, settingsWidget, xpWidgetClass_ScrollBar);
            XPSetWidgetProperty(blueScaleSlider, xpProperty_ScrollBarMin, -75);
            XPSetWidgetProperty(blueScaleSlider, xpProperty_ScrollBarMax, 75);

            y += 3;
            
            // add red offset caption
            char stringRedOffset[32];
            snprintf(stringRedOffset, 32, "Red Offset: %.2f", redOffset);
            redOffsetCaption = XPCreateWidget(x + 30, y - 210, x2 - 50, y - 225, 1, stringRedOffset, 0, settingsWidget, xpWidgetClass_Caption);

            // add red offset slider
            redOffsetSlider = XPCreateWidget(x + 195, y - 210, x2 - 15, y - 225, 1, "Red Offset", 0, settingsWidget, xpWidgetClass_ScrollBar);
            XPSetWidgetProperty(redOffsetSlider, xpProperty_ScrollBarMin, -50);
            XPSetWidgetProperty(redOffsetSlider, xpProperty_ScrollBarMax, 50);

            y += 3;
            
            // add green offset caption
            char stringGreenOffset[32];
            snprintf(stringGreenOffset, 32, "Green Offset: %.2f", greenOffset);
            greenOffsetCaption = XPCreateWidget(x + 30, y - 230, x2 - 50, y - 245, 1, stringGreenOffset, 0, settingsWidget, xpWidgetClass_Caption);

            // add green offset slider
            greenOffsetSlider = XPCreateWidget(x + 195, y - 230, x2 - 15, y - 245, 1, "Green Offset", 0, settingsWidget, xpWidgetClass_ScrollBar);
            XPSetWidgetProperty(greenOffsetSlider, xpProperty_ScrollBarMin, -50);
            XPSetWidgetProperty(greenOffsetSlider, xpProperty_ScrollBarMax, 50);

            y += 3;
            
            // add blue offset caption
            char stringBlueOffset[32];
            snprintf(stringBlueOffset, 32, "Blue Offset: %.2f", blueOffset);
            blueOffsetCaption = XPCreateWidget(x + 30, y - 250, x2 - 50, y - 265, 1, stringBlueOffset, 0, settingsWidget, xpWidgetClass_Caption);

            // add blue offset slider
            blueOffsetSlider = XPCreateWidget(x + 195, y - 250, x2 - 15, y - 265, 1, "Blue Offset", 0, settingsWidget, xpWidgetClass_ScrollBar);
            XPSetWidgetProperty(blueOffsetSlider, xpProperty_ScrollBarMin, -50);
            XPSetWidgetProperty(blueOffsetSlider, xpProperty_ScrollBarMax, 50);

            y += 3;
            
            // add vignette caption
            char stringVignette[32];
            snprintf(stringVignette, 32, "Vignette: %.2f", vignette);
            vignetteCaption = XPCreateWidget(x + 30, y - 270, x2 - 50, y - 285, 1, stringVignette, 0, settingsWidget, xpWidgetClass_Caption);

            // add vignette slider
            vignetteSlider = XPCreateWidget(x + 195, y - 270, x2 - 15, y - 285, 1, "Vignette", 0, settingsWidget, xpWidgetClass_ScrollBar);
            XPSetWidgetProperty(vignetteSlider, xpProperty_ScrollBarMin, 0);
            XPSetWidgetProperty(vignetteSlider, xpProperty_ScrollBarMax, 100);
            
            y += 3;
            
            // add reset button
            presetButtons[PRESET_DEFAULT] = XPCreateWidget(x + 30, y - 295, x + 30 + 80, y - 310, 1, "Reset", 0, settingsWidget, xpWidgetClass_Button);
            XPSetWidgetProperty(presetButtons[PRESET_DEFAULT], xpProperty_ButtonType, xpPushButton);

            // add buttons for managing custom settings:
            
            // add custom preset reload, save, and restore buttons
            presetButtons[PRESET_USER] = XPCreateWidget(x2 - 51 - 65, y - 295, x2 - 51, y - 310, 1, "Restore", 0, settingsWidget, xpWidgetClass_Button);
            XPSetWidgetProperty(presetButtons[PRESET_DEFAULT], xpProperty_ButtonType, xpPushButton);
            
            y += 3;
            
            saveButton = XPCreateWidget(x2 - 17 - 60, y - 334 + 15, x2 - 17, y - 334, 1, "Save .ini", 0, settingsWidget, xpWidgetClass_Button);
            XPSetWidgetProperty(saveButton, xpProperty_ButtonType, xpPushButton);
            
            loadButton = XPCreateWidget(x2 - 86 - 63, y - 334 + 15, x2 - 86, y - 334, 1, "Load .ini", 0, settingsWidget, xpWidgetClass_Button);
            XPSetWidgetProperty(saveButton, xpProperty_ButtonType, xpPushButton);
            
            const int BUTTON_INSET = 30;    // define button array inset (which we'll also use to widen after)
            
            x += BUTTON_INSET; x2 -= BUTTON_INSET;  // shrink width so buttons are closer together
            
            // add post-processing presets caption
            XPCreateWidget(x + 10, y - 330, x2 - 20, y - 345, 1, "Post-Processing Presets:", 0, settingsWidget, xpWidgetClass_Caption);

            y += 10; // start button group closer to title to save vertical space
            
            // first preset button column
            
            int top = y;
            
            // add polaroid preset button
            presetButtons[PRESET_POLAROID] = XPCreateWidget(x + 20, y - 360, x + 20 + 125, y - 375, 1, "Polaroid", 0, settingsWidget, xpWidgetClass_Button);
            XPSetWidgetProperty(presetButtons[PRESET_POLAROID], xpProperty_ButtonType, xpPushButton);
            y += 2;
            
            // add fogged up preset button
            presetButtons[PRESET_FOGGED_UP] = XPCreateWidget(x + 20, y - 385, x + 20 + 125, y - 400, 1, "Fogged Up", 0, settingsWidget, xpWidgetClass_Button);
            XPSetWidgetProperty(presetButtons[PRESET_FOGGED_UP], xpProperty_ButtonType, xpPushButton);
            y += 2;
            
            // add high dynamic range preset button
            presetButtons[PRESET_HIGH_DYNAMIC_RANGE] = XPCreateWidget(x + 20, y - 410, x + 20 + 125, y - 425, 1, "High Dynamic Range", 0, settingsWidget, xpWidgetClass_Button);
            XPSetWidgetProperty(presetButtons[PRESET_HIGH_DYNAMIC_RANGE], xpProperty_ButtonType, xpPushButton);
            y += 2;

            // add editor's choice drab preset button
            presetButtons[PRESET_EDITORS_CHOICE] = XPCreateWidget(x + 20, y - 435, x + 20 + 125, y - 450, 1, "Editor's Choice", 0, settingsWidget, xpWidgetClass_Button);
            XPSetWidgetProperty(presetButtons[PRESET_EDITORS_CHOICE], xpProperty_ButtonType, xpPushButton);
            y += 2;

            // add slightly enhanced preset button
            presetButtons[PRESET_SLIGHTLY_ENHANCED] = XPCreateWidget(x + 20, y - 460, x + 20 + 125, y - 475, 1, "Slightly Enhanced", 0, settingsWidget, xpWidgetClass_Button);
            XPSetWidgetProperty(presetButtons[PRESET_SLIGHTLY_ENHANCED], xpProperty_ButtonType, xpPushButton);
            y += 2;

            // add extra gloomy preset button
            presetButtons[PRESET_EXTRA_GLOOMY] = XPCreateWidget(x + 20, y - 485, x + 20 + 125, y - 500, 1, "Extra Gloomy", 0, settingsWidget, xpWidgetClass_Button);
            XPSetWidgetProperty(presetButtons[PRESET_EXTRA_GLOOMY], xpProperty_ButtonType, xpPushButton);
            y += 2;

            // add red shift preset button
            presetButtons[PRESET_RED_ISH] = XPCreateWidget(x + 20, y - 510, x + 20 + 125, y - 525, 1, "Red-ish", 0, settingsWidget, xpWidgetClass_Button);
            XPSetWidgetProperty(presetButtons[PRESET_RED_ISH], xpProperty_ButtonType, xpPushButton);
            y += 2;

            // add green shift preset button
            presetButtons[PRESET_GREEN_ISH] = XPCreateWidget(x + 20, y - 535, x + 20 + 125, y - 550, 1, "Green-ish", 0, settingsWidget, xpWidgetClass_Button);
            XPSetWidgetProperty(presetButtons[PRESET_GREEN_ISH], xpProperty_ButtonType, xpPushButton);
            y += 2;

            // add blue shift preset button
            presetButtons[PRESET_BLUE_ISH] = XPCreateWidget(x + 20, y - 560, x + 20 + 125, y - 575, 1, "Blue-ish", 0, settingsWidget, xpWidgetClass_Button);
            XPSetWidgetProperty(presetButtons[PRESET_BLUE_ISH], xpProperty_ButtonType, xpPushButton);
            
            // second preset button column
            
            y = top;
            
            // add shiny california preset button
            presetButtons[PRESET_SHINY_CALIFORNIA] = XPCreateWidget(x2 - 20 - 125, y - 360, x2 - 20, y - 375, 1, "Shiny California", 0, settingsWidget, xpWidgetClass_Button);
            XPSetWidgetProperty(presetButtons[PRESET_SHINY_CALIFORNIA], xpProperty_ButtonType, xpPushButton);
            y += 2;

            // add dusty dry preset button
            presetButtons[PRESET_DUSTY_DRY] = XPCreateWidget(x2 - 20 - 125, y - 385, x2 - 20, y - 400, 1, "Dusty Dry", 0, settingsWidget, xpWidgetClass_Button);
            XPSetWidgetProperty(presetButtons[PRESET_DUSTY_DRY], xpProperty_ButtonType, xpPushButton);
            y += 2;

            // add gray winter preset button
            presetButtons[PRESET_GRAY_WINTER] = XPCreateWidget(x2 - 20 - 125, y - 410, x2 - 20, y - 425, 1, "Gray Winter", 0, settingsWidget, xpWidgetClass_Button);
            XPSetWidgetProperty(presetButtons[PRESET_GRAY_WINTER], xpProperty_ButtonType, xpPushButton);
            y += 2;

            // add fancy imagination dreams preset button
            presetButtons[PRESET_FANCY_IMAGINATION] = XPCreateWidget(x2 - 20 - 125, y - 435, x2 - 20, y - 450, 1, "Fancy Imagination", 0, settingsWidget, xpWidgetClass_Button);
            XPSetWidgetProperty(presetButtons[PRESET_FANCY_IMAGINATION], xpProperty_ButtonType, xpPushButton);
            y += 2;

            // add sixties normal preset button
            presetButtons[PRESET_SIXTIES] = XPCreateWidget(x2 - 20 - 125, y - 460, x2 - 20, y - 475, 1, "Sixties", 0, settingsWidget, xpWidgetClass_Button);
            XPSetWidgetProperty(presetButtons[PRESET_SIXTIES], xpProperty_ButtonType, xpPushButton);
            y += 2;

            // add cold winter preset button
            presetButtons[PRESET_COLD_WINTER] = XPCreateWidget(x2 - 20 - 125, y - 485, x2 - 20, y - 500, 1, "Cold Winter", 0, settingsWidget, xpWidgetClass_Button);
            XPSetWidgetProperty(presetButtons[PRESET_COLD_WINTER], xpProperty_ButtonType, xpPushButton);
            y += 2;

            // add vintage film preset button
            presetButtons[PRESET_VINTAGE_FILM] = XPCreateWidget(x2 - 20 - 125, y - 510, x2 - 20, y - 525, 1, "Vintage Film", 0, settingsWidget, xpWidgetClass_Button);
            XPSetWidgetProperty(presetButtons[PRESET_VINTAGE_FILM], xpProperty_ButtonType, xpPushButton);
            y += 2;

            // add colorless preset button
            presetButtons[PRESET_COLORLESS] = XPCreateWidget(x2 - 20 - 125, y - 535, x2 - 20, y - 550, 1, "Colorless", 0, settingsWidget, xpWidgetClass_Button);
            XPSetWidgetProperty(presetButtons[PRESET_COLORLESS], xpProperty_ButtonType, xpPushButton);
            y += 2;

            // add monochrome preset button
            presetButtons[PRESET_MONOCHROME] = XPCreateWidget(x2 - 20 - 125, y - 560, x2 - 20, y - 575, 1, "Monochrome", 0, settingsWidget, xpWidgetClass_Button);
            XPSetWidgetProperty(presetButtons[PRESET_MONOCHROME], xpProperty_ButtonType, xpPushButton);

            x -= BUTTON_INSET; x2 += BUTTON_INSET;  // re-expand to normal width as we continue
            y = top + 30;

            // Restore left/right margin from whole section above:
            x -= 3;
            x2 += 3;

            // Only show raleigh scale slider in versions prior to XP12:
            if (LEGACY_FEATURES) {
                
                // add raleigh scale sub window
                XPCreateWidget(x + 10, y - 600, x2 - 10, y - 650 - 10, 1, "Raleigh Scale:", 0, settingsWidget, xpWidgetClass_SubWindow);

                // Add small left/right margin for the inner text:
                x += 2;
                x2 -= 2;
                
                y += 4; // tighter gutter than what the original author designed

                // add raleigh scale caption
                XPCreateWidget(x + 10, y - 605, x2 - 20, y - 620, 1, "Raleigh Scale:", 0, settingsWidget, xpWidgetClass_Caption);

                y += 10;
                
                // add raleigh scale caption
                char stringRaleighScale[32];
                snprintf(stringRaleighScale, 32, "Raleigh Scale: %.0f", raleighScale);
                raleighScaleCaption = XPCreateWidget(x + 30, y - 630, x2 - 50, y - 645, 1, stringRaleighScale, 0, settingsWidget, xpWidgetClass_Caption);

                // add raleigh scale slider
                raleighScaleSlider = XPCreateWidget(x + 195, y - 630, x2 - 15, y - 645, 1, "Raleigh Scale", 0, settingsWidget, xpWidgetClass_ScrollBar);
                XPSetWidgetProperty(raleighScaleSlider, xpProperty_ScrollBarMin, 1);
                XPSetWidgetProperty(raleighScaleSlider, xpProperty_ScrollBarMax, 100);
                
                y += 10;

                // add raleigh scale reset button
                resetRaleighScaleButton = XPCreateWidget(x + 30, y - 660, x + 30 + 80, y - 675, 1, "Reset", 0, settingsWidget, xpWidgetClass_Button);
                XPSetWidgetProperty(resetRaleighScaleButton, xpProperty_ButtonType, xpPushButton);
                
                // Restore left/right margin:
                x -= 2;
                x2 += 2;
                y -= 4; // offset adjustment for gutter above
            }
            else {
                y += 81;    // shift origin up to account for now-missing section
            }
            
            // add fps-limiter sub window
            y += 19;
            XPCreateWidget(x + 10, y - 700, x2 - 10, y - 750 - 10, 1, "FPS-Limiter:", 0, settingsWidget, xpWidgetClass_SubWindow);
            
            // Add small left/right margin for the inner text:
            x += 2;
            x2 -= 2;

            // add fps-limiter caption
            XPCreateWidget(x + 10, y - 700, x2 - 20, y - 715, 1, "FPS-Limiter:", 0, settingsWidget, xpWidgetClass_Caption);
            
            // add fps-limiter checkbox
            fpsLimiterCheckbox = XPCreateWidget(x + 20, y - 720, x2 - 20, y - 735, 1, "Enable FPS-Limiter", 0, settingsWidget, xpWidgetClass_Button);
            XPSetWidgetProperty(fpsLimiterCheckbox, xpProperty_ButtonType, xpRadioButton);
            XPSetWidgetProperty(fpsLimiterCheckbox, xpProperty_ButtonBehavior, xpButtonBehaviorCheckBox);
            
            // add max fps caption
            char stringMaxFps[32];
            snprintf(stringMaxFps, 32, "Max FPS: %.0f", maxFps);
            maxFpsCaption = XPCreateWidget(x + 30, y - 735, x2 - 50, y - 750, 1, stringMaxFps, 0, settingsWidget, xpWidgetClass_Caption);
            
            // add max fps slider
            maxFpsSlider = XPCreateWidget(x + 195, y - 735, x2 - 15, y - 750, 1, "Max FPS", 0, settingsWidget, xpWidgetClass_ScrollBar);
            XPSetWidgetProperty(maxFpsSlider, xpProperty_ScrollBarMin, 20);
            XPSetWidgetProperty(maxFpsSlider, xpProperty_ScrollBarMax, 200);

            // Restore left/right margin:
            x -= 2;
            x2 += 2;
            
            y += 39;    // get back compressed space for next sections (i.e., to minimize changes to previous offsets below)

            // add auto disable enable cinema verite sub window
            XPCreateWidget(x + 10, y - 800, x2 - 10, y - 850 - 10, 1, "Auto disable / enable Cinema Verite:", 0, settingsWidget, xpWidgetClass_SubWindow);
            
            // Add small left/right margin for the inner text:
            x += 2;
            x2 -= 2;

            // add auto disable enable cinema verite caption
            XPCreateWidget(x + 10, y - 800, x2 - 20, y - 815, 1, "Auto disable / enable Cinema Verite:", 0, settingsWidget, xpWidgetClass_Caption);
                        
            // add control cinema verite checkbox
            y += 4;
            controlCinemaVeriteCheckbox = XPCreateWidget(x + 20, y - 830, x2 - 20, y - 835, 1, "Control Cinema Verite", 0, settingsWidget, xpWidgetClass_Button);
            XPSetWidgetProperty(controlCinemaVeriteCheckbox, xpProperty_ButtonType, xpRadioButton);
            XPSetWidgetProperty(controlCinemaVeriteCheckbox, xpProperty_ButtonBehavior, xpButtonBehaviorCheckBox);
            
            // add disable cinema verite time caption
            char stringDisableCinemaVeriteTime[32];
            snprintf(stringDisableCinemaVeriteTime, 32, "On input disable for: %.0f sec", disableCinemaVeriteTime);
            disableCinemaVeriteTimeCaption = XPCreateWidget(x + 30, y - 838, x2 - 50, y - 853, 1, stringDisableCinemaVeriteTime, 0, settingsWidget, xpWidgetClass_Caption);
            
            // add disable cinema verite time slider
            disableCinemaVeriteTimeSlider = XPCreateWidget(x + 195, y - 838, x2 - 15, y - 853, 1, "Disable Cinema Verite Timer", 0, settingsWidget, xpWidgetClass_ScrollBar);
            XPSetWidgetProperty(disableCinemaVeriteTimeSlider, xpProperty_ScrollBarMin, 1);
            XPSetWidgetProperty(disableCinemaVeriteTimeSlider, xpProperty_ScrollBarMax, 30);
            
            // Restore left/right margin:
            x -= 2;
            x2 += 2;

            y += 36;        // final adjustment for compressed CV section to footer below (for consistency)
            
            // add about sub window
            XPCreateWidget(x + 10, y - 900, x2 - 10, y - 957 - 16, 1, "About:", 0, settingsWidget, xpWidgetClass_SubWindow);
            
            // Add small left/right margin for the inner text:
            x += 2;
            x2 -= 2;
            
            // add about caption
            y -= 2; // give a little gutter to center vertically within sub-window
            XPCreateWidget(x + 10, y - 900, x2 - 20, y - 915, 1, "About " NAME " " VERSION ":", 0, settingsWidget, xpWidgetClass_Caption);
            y -= 2; // ditto for the "body"
            x += 20;
            x2 -= 20;
            XPCreateWidget(x + 10, y - 915, x2 - 20, y - 930, 1, "Thank you for using " NAME ", created by Matteo Hausner,", 0, settingsWidget, xpWidgetClass_Caption);
            XPCreateWidget(x + 10, y - 930, x2 - 20, y - 945, 1, "with updates and new features by Steve Goldberg", 0, settingsWidget, xpWidgetClass_Caption);
            XPCreateWidget(x + 10, y - 945, x2 - 20, y - 960, 1, "(PM @slgoldberg on x-plane.org).", 0, settingsWidget, xpWidgetClass_Caption);
            x -= 20;    // restore extra inset
            x2 += 20;
            
            // Restore left/right margin: (in case we add more below)
            x -= 2;
            x2 += 2;

            // init checkbox and slider positions
            UpdateSettingsWidgets();

            // register widget handler
            XPAddWidgetCallback(settingsWidget, (XPWidgetFunc_t) SettingsWidgetHandler);
        }
        else
        {
            // settings widget already created
            if (!XPIsWidgetVisible(settingsWidget))
                XPShowWidget(settingsWidget);
        }
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
        if (settingsWidget && XPIsWidgetVisible(settingsWidget))
            XPHideWidget(settingsWidget);       // toggle display off
        else
            MenuHandlerCallback(NULL, NULL);    // dispatch command via legacy menu handler
    }

    return 1;   // allow others to listen to this command if they like
}

PLUGIN_API int XPluginStart(char *outName, char *outSig, char *outDesc)
{
    // set plugin info
    strcpy(outName, NAME_VERSION);
    strcpy(outSig, "de.bwravencl." NAME_LOWERCASE); // TODO: new signature?
    strcpy(outDesc, NAME_VERSION " enhances your X-Plane experience!");
    
    // Update widgets library to use modern windows:
    // (Note: this allows users with UI Zoom to get correct results!)
    XPLMEnableFeature("XPLM_USE_NATIVE_WIDGET_WINDOWS", 1);	// depends on XPLM301+
    
    // Get version of X-Plane:
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
    XPLMDebugString(NAME_VERSION_BLANK "X-Plane.org forums). " NAME " v1.0 was created by Matteo Hausner,");
    XPLMDebugString(NAME_VERSION_BLANK "with all future updates from v1.1 onward (including this release)");
    XPLMDebugString(NAME_VERSION_BLANK "by Steve Goldberg. From both of us: you're welcome!\n");
#else   /* Show this general multi-line "about" message for Linux and Windows builds: */
    XPLMDebugString(NAME_VERSION ": About this release (" NAME_LOWERCASE ".xpl version " VERSION "):\n");
    XPLMDebugString(NAME_VERSION_BLANK "This updated 64-bit version is distributed in the modern X-Plane\n");
    XPLMDebugString(NAME_VERSION_BLANK "universal plugin binary format, and includes feature improvements,\n");
    XPLMDebugString(NAME_VERSION_BLANK "all of which were added by Steve Goldberg (PM @slgoldberg on the\n");
    XPLMDebugString(NAME_VERSION_BLANK "X-Plane.org forums), with the original plugin created by Matteo\n");
    XPLMDebugString(NAME_VERSION_BLANK "Hausner). From both of us: you're welcome!\n");
#endif
    
    // prepare fragment-shader
    InitShader(FRAGMENT_SHADER);

    // obtain datarefs
    cinemaVeriteDataRef = XPLMFindDataRef("sim/graphics/view/cinema_verite");
    viewTypeDataRef = XPLMFindDataRef("sim/graphics/view/view_type");
    ignitionKeyDataRef = XPLMFindDataRef("sim/cockpit2/engine/actuators/ignition_key");

    // register own dataref
    overrideControlCinemaVeriteDataRef = XPLMRegisterDataAccessor(NAME_LOWERCASE "/override_control_cinema_verite", xplmType_Int,  1, GetOverrideControlCinemaVeriteDataRefCallback, SetOverrideControlCinemaVeriteDataRefCallback,  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    // register our own commandref
    XPLMCommandRef toggleSettingsCmd = XPLMCreateCommand(NAME_LOWERCASE "/toggle_settings", "toggle " NAME " settings window open/closed");
    XPLMRegisterCommandHandler(toggleSettingsCmd, toggleSettingsHandler, 1, NULL);
    
    // create menu-entries
    int subMenuItem = XPLMAppendMenuItem(XPLMFindPluginsMenu(), NAME, 0, 1);
    XPLMMenuID menu = XPLMCreateMenu(NAME, XPLMFindPluginsMenu(), subMenuItem, MenuHandlerCallback, 0);
    
    // before adding modern menu option, check to be sure XPLM 301 is available:
    if (XPLMFindSymbol("XPLMAppendMenuItemWithCommand"))
        XPLMAppendMenuItemWithCommand(menu, "Settings", toggleSettingsCmd);
    else
        XPLMAppendMenuItem(menu,"Settings", NULL, 1);

    // read and apply config file
    LoadSettings();

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

PLUGIN_API void XPluginStop(void)
{
    // save settings on exit to auto-restore on next startup
    SaveSettings();
    
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

PLUGIN_API void XPluginDisable(void)
{
    UpdateRaleighScale(1);
}

PLUGIN_API int XPluginEnable(void)
{
    return 1;
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, long inMessage, void *inParam)
{
    if (inMessage == XPLM_MSG_PLANE_LOADED)
        bringFakeWindowToFront = 0;
    else if (inMessage == XPLM_MSG_SCENERY_LOADED)
        UpdateRaleighScale(0);
}
