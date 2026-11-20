#include "doc-viewer-img-window.h"
#include "settings-img-window.h"
#include "blu_fx.h"
#include "XPLMUtilities.h"
#include "XPLMPlugin.h"
#include "XPLMDisplay.h"
#include "XPLMProcessing.h"

#include <cstdio>
#include <algorithm>
#include <cmath>

std::unique_ptr<DocViewerImgWindow> DocViewerImgWindow::sInstance = nullptr;

static XPLMFlightLoopID s_DocViewerFlightLoop = nullptr;
static bool s_PendingShow = false;
static bool s_PendingToggle = false;

float DocViewerImgWindow::DeferredFlightLoopCB(float, float, int, void*)
{
    if (s_PendingShow) {
        s_PendingShow = false;
        if (!sInstance) {
            sInstance = std::make_unique<DocViewerImgWindow>();
        }
        sInstance->SetVisible(true);
        sInstance->BringWindowToFront();
        UpdateMenuCheckmarks();
    } else if (s_PendingToggle) {
        s_PendingToggle = false;
        if (!sInstance) {
            sInstance = std::make_unique<DocViewerImgWindow>();
            sInstance->SetVisible(true);
            sInstance->BringWindowToFront();
            UpdateMenuCheckmarks();
        } else {
            bool vis = sInstance->GetVisible();
            sInstance->SetVisible(!vis);
            if (!vis) {
                sInstance->BringWindowToFront();
            } else {
                sInstance->onDocumentClosed();
            }
            UpdateMenuCheckmarks();
        }
    }
    return 0.0f;
}

static void ScheduleDocViewerAction(bool show, bool toggle)
{
    if (show) s_PendingShow = true;
    if (toggle) s_PendingToggle = true;

    if (!s_DocViewerFlightLoop) {
        XPLMCreateFlightLoop_t flDef = {
            sizeof(flDef),
            xplm_FlightLoop_Phase_BeforeFlightModel,
            DocViewerImgWindow::DeferredFlightLoopCB,
            nullptr
        };
        s_DocViewerFlightLoop = XPLMCreateFlightLoop(&flDef);
    }
    XPLMScheduleFlightLoop(s_DocViewerFlightLoop, -1.0f, 1);
}

static const char* s_FallbackMarkdown =
    "# BLU-fx User Guide\n\n"
    "**BLU-fx** is a lightweight, real-time post-processing color grading plugin for X-Plane 12 and X-Plane 11 across macOS, Windows, and Linux.\n\n"
    "---\n\n"
    "## 1. Quick Start\n\n"
    "1. Open the simulator menu and select **Plugins -> BLU-fx -> Settings window**.\n"
    "2. Flip the **Enable Post-Processing** toggle at the top left to turn filters ON.\n"
    "3. In the **Presets** section, click any preset to immediately transform your simulator scenery.\n"
    "4. By default, BLU-fx opens in **Split** preview mode. Drag the **Wipe** slider back and forth to compare before and after!\n\n"
    "---\n\n"
    "## 2. Live Preview Controls\n\n"
    "* **Split Mode:** Shows unmodified scenery on the left and color graded scenery on the right.\n"
    "* **Full Mode:** Applies color grading across 100% of the display while settings are open.\n"
    "* **Wipe Slider:** Adjusts the boundary divider between raw and graded views.\n"
    "* **Hold for Full View:** Temporarily previews full-screen color grading while held down.\n\n"
    "---\n\n"
    "## 3. Keyboard & Joystick Commands\n\n"
    "* `blu_fx/settings/toggle` -- Toggle the BLU-fx Settings window.\n"
    "* `blu_fx/post_processing/toggle` -- Toggle color grading on or off.\n"
    "* `blu_fx/post_processing/momentary` -- Momentary full-screen preview while held.\n"
    "* `blu_fx/user_guide/toggle` -- Toggle this User Guide window.\n\n"
    "---\n\n"
    "*(Full documentation with embedded illustrations is available in the docs/ folder)*\n";

static WindowGeometry CalcDefaultDocViewerGeometry()
{
    int screenL = 0, screenT = 0, screenR = 0, screenB = 0;
    XPLMGetScreenBoundsGlobal(&screenL, &screenT, &screenR, &screenB);

    int w = (sDocViewerWidth >= 480) ? sDocViewerWidth : DocViewerImgWindow::DEFAULT_WIDTH;
    int h = (sDocViewerHeight >= 300) ? sDocViewerHeight : DocViewerImgWindow::DEFAULT_HEIGHT;

    if (sDocViewerLeft >= 0 && sDocViewerTop >= 0) {
        return WindowGeometry::Sanitize(
            sDocViewerLeft, sDocViewerTop,
            sDocViewerLeft + w, sDocViewerTop - h,
            w, h, 40,
            WindowSanitizeMode::KeepVisibleMargin
        );
    }

    // Center on main display with slight right offset so it does not obscure Settings window
    int centerX = screenL + (screenR - screenL) / 2 + 80;
    int centerY = screenB + (screenT - screenB) / 2;

    int left = centerX - w / 2;
    int top  = centerY + h / 2;

    return WindowGeometry::Sanitize(
        left, top, left + w, top - h,
        w, h, 40,
        WindowSanitizeMode::KeepVisibleMargin
    );
}

DocViewerImgWindow::DocViewerImgWindow()
    : ImgDocWindow(NAME " -- User Guide",
                   CalcDefaultDocViewerGeometry(),
                   xplm_WindowDecorationSelfDecoratedResizable,
                   xplm_WindowLayerFloatingWindows,
                   true)
{
    // Wire BLU-fx typography into MarkdownConfig:
    const ImGui::MarkdownConfig* defCfg = ImGui::GetDefaultMarkdownConfig();
    if (defCfg) {
        mMdConfig = *defCfg;
    }
    if (IMG_TITLE_FONT)      mMdConfig.h1Font = IMG_TITLE_FONT;
    if (IM_BOLD_LARGER_FONT) mMdConfig.h2Font = IM_BOLD_LARGER_FONT;
    if (IM_BOLD_FONT)        mMdConfig.h3Font = IM_BOLD_FONT;
    if (IM_BOLD_FONT)        mMdConfig.boldFont = IM_BOLD_FONT;
    if (IM_ITALIC_FONT)      mMdConfig.italicFont = IM_ITALIC_FONT;
    if (IM_MONO_NORMAL_FONT) mMdConfig.monoFont = IM_MONO_NORMAL_FONT;
    GetViewerConfig().markdownConfig = &mMdConfig;

#if defined(IMGUI_FONT_ROLES_H)
    if (IMG_TITLE_FONT)      ImGui::SetFontRole(ImGui::ImFontRole::Title,    IMG_TITLE_FONT);
    if (IMG_TITLE_FONT)      ImGui::SetFontRole(ImGui::ImFontRole::Heading1, IMG_TITLE_FONT);
    if (IM_BOLD_LARGER_FONT) ImGui::SetFontRole(ImGui::ImFontRole::Heading2, IM_BOLD_LARGER_FONT);
    if (IM_BOLD_FONT)        ImGui::SetFontRole(ImGui::ImFontRole::Heading3, IM_BOLD_FONT);
    if (IM_BOLD_FONT)        ImGui::SetFontRole(ImGui::ImFontRole::Bold,     IM_BOLD_FONT);
    if (IM_ITALIC_FONT)      ImGui::SetFontRole(ImGui::ImFontRole::Italic,   IM_ITALIC_FONT);
    if (IM_MONO_NORMAL_FONT) ImGui::SetFontRole(ImGui::ImFontRole::Monospace, IM_MONO_NORMAL_FONT);
    if (IM_SMALLER_FONT)     ImGui::SetFontRole(ImGui::ImFontRole::Small,    IM_SMALLER_FONT);
#endif

    SetFallbackMarkdown(s_FallbackMarkdown);
    LoadMarkdownFile("docs/BLUFX_User_Guide.md");
    SetCompanionPdfPath("docs/BLUFX_User_Guide.pdf");

    if (sDocViewerZoom >= 0.50f && sDocViewerZoom <= 2.50f) {
        SetZoom(sDocViewerZoom, false);
    }
    if (sDocViewerTheme >= 0 && sDocViewerTheme < 5) {
        SetTheme(sDocViewerTheme);
    }
    if (sDocViewerOpacity >= 0.10f && sDocViewerOpacity <= 1.00f) {
        SetWindowOpacity(sDocViewerOpacity);
    }
    GetViewerState().transparent_hud = (sDocViewerTransparent != 0);

    UpdateMenuCheckmarks();
}

DocViewerImgWindow::~DocViewerImgWindow()
{
    UpdateMenuCheckmarks();
}

DocViewerImgWindow* DocViewerImgWindow::GetInstance()
{
    return sInstance.get();
}

void DocViewerImgWindow::ShowInstance()
{
    ScheduleDocViewerAction(true, false);
}

void DocViewerImgWindow::ToggleInstance()
{
    ScheduleDocViewerAction(false, true);
}

void DocViewerImgWindow::DestroyInstance()
{
    if (s_DocViewerFlightLoop) {
        XPLMDestroyFlightLoop(s_DocViewerFlightLoop);
        s_DocViewerFlightLoop = nullptr;
    }
    s_PendingShow = false;
    s_PendingToggle = false;
    sInstance.reset();
}

bool DocViewerImgWindow::IsOpen()
{
    return (sInstance && sInstance->GetVisible());
}

void DocViewerImgWindow::RequestClose()
{
    SetVisible(false);
    onDocumentClosed();
}

void DocViewerImgWindow::buildInterface()
{
    ImgDocWindow::buildInterface();

    // Continuously remember in-simulator (boxel) window position, size, and zoom:
    if (!IsPoppedOut() && !IsInVR()) {
        int l = 0, t = 0, r = 0, b = 0;
        GetWindowGeometry(l, t, r, b);
        int w = r - l;
        int h = t - b;
        if (w >= 480 && h >= 300) {
            sDocViewerLeft = l;
            sDocViewerTop = t;
            sDocViewerWidth = w;
            sDocViewerHeight = h;
            sDocViewerZoom = GetZoom();
        }
    }
}

void DocViewerImgWindow::onDocumentClosed()
{
    SavePreferences();
    UpdateMenuCheckmarks();
}

void DocViewerImgWindow::onDocumentZoomChanged(float /*oldZoom*/, float newZoom)
{
    sDocViewerZoom = newZoom;
    SavePreferences();
}

void DocViewerImgWindow::onDocumentThemeChanged(int themeIndex, bool transparentHud, float opacity)
{
    sDocViewerTheme = themeIndex;
    sDocViewerTransparent = transparentHud ? 1 : 0;
    sDocViewerOpacity = opacity;
    SavePreferences();
}
