#include "doc-viewer-img-window.h"
#include "settings-img-window.h"
#include "blu_fx.h"
#include "XPLMUtilities.h"
#include "XPLMPlugin.h"
#include "XPLMDisplay.h"
#include "XPLMProcessing.h"

#include <cstdio>
#include <sys/stat.h>
#include <algorithm>
#include <cmath>

std::unique_ptr<DocViewerImgWindow> DocViewerImgWindow::sInstance = nullptr;

static XPLMFlightLoopID s_DocViewerFlightLoop = nullptr;
static bool s_PendingShow = false;
static bool s_PendingToggle = false;

static bool ToolbarButton(const char* label, const char* tip = nullptr, const ImVec2& size = ImVec2(0, 0))
{
    bool clicked = ImGui::Button(label, size);
    bool hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    if (!clicked && hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        clicked = true;
    }
    if (tip && hovered) {
        ImGui::TimedTooltip::TextMDProxy(ImGui::GetItemID(), hovered, "%s", tip);
    }
    return clicked;
}

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
                SavePreferences();
            }
            UpdateMenuCheckmarks();
        }
    }

    if (sInstance && (int)sInstance->mNextWinPosMode >= 0) {
        XPLMWindowPositioningMode targetMode = sInstance->mNextWinPosMode;
        if (targetMode == xplm_WindowPopOut && !sInstance->IsPoppedOut()) {
            sInstance->GetWindowGeometry(sInstance->mSavedSimLeft, sInstance->mSavedSimTop, sInstance->mSavedSimRight, sInstance->mSavedSimBottom);
            sInstance->mSavedSimZoom = sInstance->mFontAdjust;
            sInstance->mHasSavedSimGeometry = (sInstance->mSavedSimRight > sInstance->mSavedSimLeft && sInstance->mSavedSimTop > sInstance->mSavedSimBottom);
        }
        sInstance->SetWindowPositioningMode(targetMode);
        if (targetMode == xplm_WindowPositionFree && sInstance->mHasSavedSimGeometry) {
            sInstance->SetWindowGeometry(sInstance->mSavedSimLeft, sInstance->mSavedSimTop, sInstance->mSavedSimRight, sInstance->mSavedSimBottom);
        }
        sInstance->mNextWinPosMode = (XPLMWindowPositioningMode)-1;
    }

    return 0.0f; // 0 = unschedule until next request
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
    : ImgWindow(CalcDefaultDocViewerGeometry(),
                xplm_WindowDecorationSelfDecoratedResizable,
                xplm_WindowLayerFloatingWindows,
                true)
{
    SetWindowTitle(NAME " - User Guide");
    SetWindowResizingLimits(480, 400, 2560, 1600);
    SetBaselineWindowSize(DEFAULT_WIDTH, DEFAULT_HEIGHT);
    SetUnscaledChrome(0, 48);
    SetWindowDragArea(DEFAULT_WIDTH / 2 - 60, 0, DEFAULT_WIDTH / 2 + 60, 16);

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    if (sDocViewerZoom >= 0.50f && sDocViewerZoom <= 2.00f) {
        mFontAdjust = sDocViewerZoom;
        mLastZoom = sDocViewerZoom;
    }

    // Latch baseline geometry snapshot for 100% baseline zoom reset
    GetWindowGeometry(_simBaselineRect.l, _simBaselineRect.t, _simBaselineRect.r, _simBaselineRect.b);
    _simBaselineRect.valid = (_simBaselineRect.r > _simBaselineRect.l && _simBaselineRect.t > _simBaselineRect.b);
    mSavedSimLeft = _simBaselineRect.l;
    mSavedSimTop = _simBaselineRect.t;
    mSavedSimRight = _simBaselineRect.r;
    mSavedSimBottom = _simBaselineRect.b;
    mSavedSimZoom = mFontAdjust;
    mHasSavedSimGeometry = _simBaselineRect.valid;

    ImGui::MarkdownImageService::ClearTextureCache();
    loadDocument();
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
    SavePreferences();
    UpdateMenuCheckmarks();
}

void DocViewerImgWindow::ReloadDocument()
{
    mLoaded = false;
    ImGui::MarkdownImageService::ClearTextureCache();
    loadDocument();
}

void DocViewerImgWindow::loadDocument()
{
    std::vector<std::string> searchPaths = {
        "docs/BLUFX_User_Guide.md",
        "./Resources/plugins/" NAME_LOWERCASE "/docs/BLUFX_User_Guide.md",
        "Resources/plugins/" NAME_LOWERCASE "/docs/BLUFX_User_Guide.md"
    };

    std::vector<std::string> pdfSearchPaths = {
        "docs/BLUFX_User_Guide.pdf",
        "./Resources/plugins/" NAME_LOWERCASE "/docs/BLUFX_User_Guide.pdf",
        "Resources/plugins/" NAME_LOWERCASE "/docs/BLUFX_User_Guide.pdf"
    };

    // Also check plugin directory path:
    XPLMPluginID myId = XPLMGetMyID();
    if (myId != XPLM_NO_PLUGIN_ID) {
        char pluginFilePath[512] = {0};
        XPLMGetPluginInfo(myId, nullptr, pluginFilePath, nullptr, nullptr);
        std::string pPath(pluginFilePath);
        size_t slash = pPath.find_last_of("/\\");
        if (slash != std::string::npos) {
            std::string pluginDir = pPath.substr(0, slash);
            size_t prevSlash = pluginDir.find_last_of("/\\");
            std::string parentDir = (prevSlash != std::string::npos) ? pluginDir.substr(0, prevSlash) : pluginDir;
            searchPaths.insert(searchPaths.begin(), parentDir + "/docs/BLUFX_User_Guide.md");
            pdfSearchPaths.insert(pdfSearchPaths.begin(), parentDir + "/docs/BLUFX_User_Guide.pdf");
        }
    }

    struct stat st;
    mDocumentPath.clear();
    for (const auto& path : searchPaths) {
        if (stat(path.c_str(), &st) == 0) {
            mDocumentPath = path;
            mLastMtime = st.st_mtime;
            break;
        }
    }

    mPdfPath.clear();
    for (const auto& path : pdfSearchPaths) {
        if (stat(path.c_str(), &st) == 0) {
            mPdfPath = path;
            break;
        }
    }

    if (!mDocumentPath.empty()) {
        FILE* fp = fopen(mDocumentPath.c_str(), "rb");
        if (fp) {
            fseek(fp, 0, SEEK_END);
            long sz = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            if (sz > 0) {
                std::string buf;
                buf.resize(sz);
                size_t readBytes = fread(&buf[0], 1, sz, fp);
                buf.resize(readBytes);
                mContent = std::move(buf);
                mLoaded = true;
                fclose(fp);
                return;
            }
            fclose(fp);
        }
    }

    // Fallback:
    mContent = s_FallbackMarkdown;
    mLoaded = true;
}

DocViewerImgWindow::WindowRectSnapshot& DocViewerImgWindow::ActiveBaselineRect()
{
    if (IsInVR()) return _vrBaselineRect;
    if (IsPoppedOut()) return _osBaselineRect;
    return _simBaselineRect;
}

DocViewerImgWindow::WindowRectSnapshot& DocViewerImgWindow::ActivePriorZoomRect()
{
    if (IsInVR()) return _vrPriorZoomRect;
    if (IsPoppedOut()) return _osPriorZoomRect;
    return _simPriorZoomRect;
}

void DocViewerImgWindow::CaptureCurrentWindowRect(WindowRectSnapshot& rect) const
{
    GetCurrentWindowGeometry(rect.l, rect.t, rect.r, rect.b);
    if (IsInVR()) {
        rect.valid = (rect.r > 0 && rect.t > 0);
    } else if (IsPoppedOut()) {
        rect.valid = (rect.r > rect.l && rect.t != rect.b);
    } else {
        rect.valid = (rect.r > rect.l && rect.t > rect.b);
    }
}

void DocViewerImgWindow::RestoreWindowRect(const WindowRectSnapshot& rect)
{
    if (!rect.valid || !GetWindowId()) return;

    if (IsInVR()) {
        int w = std::abs(rect.r - rect.l);
        int h = std::abs(rect.t - rect.b);
        if (w > 0 && h > 0) {
            w = ImClamp(w, 480, 2560);
            h = ImClamp(h, 400, 1600);
            SetWindowGeometryVR(w, h);
        }
        return;
    }

    if (IsPoppedOut()) {
        if (rect.r > rect.l && rect.t != rect.b) {
            SetWindowGeometryOS(rect.l, rect.t, rect.r, rect.b);
        }
        return;
    }

    if (rect.r <= rect.l || rect.t <= rect.b) return;
    int l = rect.l;
    int t = rect.t;
    int r = rect.r;
    int b = rect.b;

    ClampGeometryToScreen(l, t, r, b, 480, 400, 2560, 1600, GetEffectiveTopMargin());
    SetWindowGeometry(l, t, r, b);
}

void DocViewerImgWindow::applyZoomTransition(float oldZoom, float newZoom)
{
    if (std::abs(newZoom - oldZoom) < 0.001f) return;
    ScaleWindowGeometry(oldZoom, newZoom);
}

void DocViewerImgWindow::applyTheme(int themeIdx)
{
    mCurrentTheme = themeIdx;
    ImGuiStyle& style = ImGui::GetStyle();
    switch (mCurrentTheme) {
        case 0: ImGui::ApplyDefaultTheme(&style); break;
        case 1: ImGui::StyleColorsDark(&style); break;
        case 2: ImGui::StyleColorsClassic(&style); break;
        case 3: ImGui::StyleColorsLight(&style); break;
        default: ImGui::ApplyDefaultTheme(&style); break;
    }
    style.Colors[ImGuiCol_WindowBg].w = 0.95f;
    style.Colors[ImGuiCol_Border].w = 0.8f;
}

void DocViewerImgWindow::drawToolbar()
{
    float avail_w = ImGui::GetWindowWidth();
    int center_local = int(avail_w * 0.5f);

    // Responsive title plain string
    const char* t_plain = (avail_w > 540.0f) ? NAME " User Guide"
                        : (avail_w > 400.0f) ? "User Guide"
                        : "Guide";
    const float t_w = ImGui::CalcTextSize(t_plain).x + 36.0f; // with icon
    const float m_offset = ImGui::GetStyle().WindowPadding.x + t_w + ImGui::GetStyle().ItemSpacing.x;
    const float fixed_w = 210.0f;
    const float r_margin = ImGui::GetStyle().WindowPadding.x + 8.0f;
    const float avail_suite = ImMax(fixed_w + 90.0f, avail_w - m_offset - r_margin);
    const float s_w = ImClamp(avail_suite - fixed_w, 75.0f, 120.0f);
    const float tgt_offset = avail_w - (fixed_w + s_w) - r_margin;
    const float suite_start = ImMax(m_offset, tgt_offset);

    // Modern Avionics Glass Top Grip Handle
    if (!IsPoppedOut()) {
        int drag_left = std::max(center_local - 60, int(m_offset + 2.0f));
        int drag_right = std::min(center_local + 60, int(suite_start - 2.0f));
        if (drag_right > drag_left) {
            SetWindowDragArea(drag_left, 0, drag_right, 16);
        } else {
            drag_left = std::max(0, center_local - 15);
            drag_right = center_local + 15;
            SetWindowDragArea(drag_left, 0, drag_right, 16);
        }

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 win_pos = ImGui::GetWindowPos();
        float center_x = win_pos.x + avail_w * 0.5f;
        float grip_w = 48.0f;
        float grip_y = win_pos.y + 6.0f;
        draw_list->AddRectFilled(ImVec2(center_x - grip_w * 0.5f, grip_y),
                                 ImVec2(center_x + grip_w * 0.5f, grip_y + 4.0f),
                                 IM_COL32(255, 255, 255, 100), 2.0f);

        float hover_min_x = win_pos.x + float(drag_left);
        float hover_max_x = win_pos.x + float(drag_right);
        ImVec2 grip_min(hover_min_x, win_pos.y);
        ImVec2 grip_max(hover_max_x, win_pos.y + 16.0f);
        bool is_hovered = ImGui::IsMouseHoveringRect(grip_min, grip_max) && !IsWindowMoving();
        if (is_hovered || IsWindowMoving()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
        }
        ImGui::TimedTooltip::TextMDProxy("##DocViewerGripDragHandle", is_hovered,
            "**Window Drag Handle** (Glass HUD)\n"
            "• Left-drag here to reposition window.\n"
            "• *ProTip:* You can **right-drag anywhere** inside the window to move it!"
        );
    } else {
        ClearWindowDragArea();
    }

    // 1. Container / Window Chrome: Baseline 1.0x scale (rock-solid, non-twitching controls)
    ImGui::SetWindowFontScale(1.0f);

    // Title / Icon
    if (IM_BOLD_FONT) ImGui::PushFont(IM_BOLD_FONT);
    ImGui::TextUnformatted(ICON_FA_BOOK "  ");
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::TextUnformatted(t_plain);
    if (IM_BOLD_FONT) ImGui::PopFont();

    // Right-aligned zoom and actions suite
    ImGui::SameLine(suite_start);

    // Zoom Stepper [-]
    ImGui::PushButtonRepeat(true);
    bool decClicked = ToolbarButton("-##zoom_dec", nullptr, ImVec2(22.0f, 0.0f));
    ImGui::PopButtonRepeat();
    bool decHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    if (!decClicked && decHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        decClicked = true;
    }
    if (decClicked) {
        bool isInitial = ImGui::IsItemActivated() || ImGui::IsMouseClicked(0);
        bool allowStep = false;
        if (isInitial) {
            mDecRepeatCount = 0;
            allowStep = true;
        } else {
            mDecRepeatCount++;
            if ((mDecRepeatCount % 4) == 0) {
                allowStep = true;
            }
        }
        if (allowStep) {
            float oldZ = mFontAdjust;
            mFontAdjust = std::max(0.50f, mFontAdjust - 0.05f);
            if (std::abs(mFontAdjust - oldZ) > 0.001f) {
                applyZoomTransition(oldZ, mFontAdjust);
            }
        }
    }
    if (decHovered) {
        ImGui::TimedTooltip::TextMDProxy(ImGui::GetItemID(), decHovered, "Zoom Out (-5%%)\n\n*Click or hold down to step smoothly*");
    }

    ImGui::SameLine(0.0f, 3.0f);

    // Zoom Slider: 50% - 200%
    float preSliderZoom = mFontAdjust;
    float pct = mFontAdjust * 100.0f;
    ImGui::SetNextItemWidth(s_w);
    if (ImGui::SliderFloat("##zoom_slider", &pct, 50.0f, 200.0f, "Zoom %.0f%%")) {
        mFontAdjust = pct / 100.0f;
    }
    if (ImGui::IsItemActivated()) {
        mSliderStartZoom = preSliderZoom;
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        if (std::abs(mFontAdjust - mSliderStartZoom) > 0.001f) {
            applyZoomTransition(mSliderStartZoom, mFontAdjust);
            mSliderStartZoom = mFontAdjust;
        }
    }
    // Sensitive mouse wheel (2% per click on macOS):
    float wheelOld = mFontAdjust;
    if (ImGui::AdjustItemOnMouseWheel(&mFontAdjust, 0.50f, 2.00f, 0.02f)) {
        if (std::abs(mFontAdjust - wheelOld) > 0.001f) {
            applyZoomTransition(wheelOld, mFontAdjust);
        }
    }
    ImGui::TimedTooltip::MD::Text("Interactive UI Zoom: roll mouse wheel (2%%) or drag thumb.\n\n*Window resizes proportionally to match.*");

    ImGui::SameLine(0.0f, 3.0f);

    // Zoom Stepper [+]
    ImGui::PushButtonRepeat(true);
    bool incClicked = ToolbarButton("+##zoom_inc", nullptr, ImVec2(22.0f, 0.0f));
    ImGui::PopButtonRepeat();
    bool incHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    if (!incClicked && incHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        incClicked = true;
    }
    if (incClicked) {
        bool isInitial = ImGui::IsItemActivated() || ImGui::IsMouseClicked(0);
        bool allowStep = false;
        if (isInitial) {
            mIncRepeatCount = 0;
            allowStep = true;
        } else {
            mIncRepeatCount++;
            if ((mIncRepeatCount % 4) == 0) {
                allowStep = true;
            }
        }
        if (allowStep) {
            float oldZ = mFontAdjust;
            mFontAdjust = std::min(2.00f, mFontAdjust + 0.05f);
            if (std::abs(mFontAdjust - oldZ) > 0.001f) {
                applyZoomTransition(oldZ, mFontAdjust);
            }
        }
    }
    if (incHovered) {
        ImGui::TimedTooltip::TextMDProxy(ImGui::GetItemID(), incHovered, "Zoom In (+5%%)\n\n*Click or hold down to step smoothly*");
    }

    ImGui::SameLine(0.0f, 3.0f);

    // Vector Reset button with 1-click A/B toggle memory to prior zoom and geometry
    bool isModified = (std::abs(mFontAdjust - 1.0f) > 0.001f);
    bool canRestore = !isModified && (std::abs(mLastZoom - 1.0f) > 0.001f);
    bool rstClicked = ImGui::ResetButton("##zoom_reset", isModified, 0.0f, false, nullptr, canRestore);
    bool rstHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    if (rstClicked) {
        float oldZ = mFontAdjust;
        if (isModified) {
            CaptureCurrentWindowRect(ActivePriorZoomRect());
            mLastZoom = mFontAdjust;
            mFontAdjust = 1.0f;
            const WindowRectSnapshot& base = ActiveBaselineRect();
            if (base.valid) {
                RestoreWindowRect(base);
            } else {
                applyZoomTransition(oldZ, 1.0f);
            }
        } else if (canRestore) {
            mFontAdjust = mLastZoom;
            const WindowRectSnapshot& prior = ActivePriorZoomRect();
            if (prior.valid) {
                RestoreWindowRect(prior);
            } else {
                applyZoomTransition(1.0f, mFontAdjust);
            }
        }
    }
    if (rstHovered) {
        if (isModified) {
            ImGui::TimedTooltip::TextMDProxy(ImGui::GetItemID(), rstHovered, "**Reset (↺ 100%%):** Reset zoom from **%d%%** back to **100%% baseline**.", (int)std::round(mFontAdjust * 100.0f));
        } else if (canRestore) {
            ImGui::TimedTooltip::TextMDProxy(ImGui::GetItemID(), rstHovered, "**Toggle (↺ prior):** 1-click toggle back to prior **%d%% zoom**.", (int)std::round(mLastZoom * 100.0f));
        } else {
            ImGui::TimedTooltip::TextMDProxy(ImGui::GetItemID(), rstHovered, "**Reset (↺ 100%%):** Zoom is at **100%% baseline**.");
        }
    }

    ImGui::SameLine(0.0f, 3.0f);

    // Proportional Window Scaling: Fit window dimensions to baseline at current zoom level
    if (ToolbarButton("Fit##fit_window", nullptr, ImVec2(32.0f, 0.0f))) {
        ResizeToScale(mFontAdjust);
    }
    bool fitHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    if (fitHovered) {
        ImGui::TimedTooltip::TextMDProxy(ImGui::GetItemID(), fitHovered, "**Fit Window to Baseline**\n\nResets window to default proportions scaled to current zoom (%d%%) anchored to top-left.", (int)std::round(mFontAdjust * 100.0f));
    }

    // External PDF Button
    if (!mPdfPath.empty()) {
        ImGui::SameLine(0.0f, 6.0f);
        if (ToolbarButton(ICON_FA_FILE_PDF " PDF##open_pdf", "Open External PDF:\nLaunches the companion manual in your system default PDF reader.")) {
            ImGui::OpenURL(mPdfPath.c_str());
        }
    }

    // Window Mode Buttons (Pop-out into OS window / Pop back into simulator)
    ImGui::SameLine(0.0f, 4.0f);
    if (IsPoppedOut() || IsInVR()) {
        if (ToolbarButton(ICON_FA_WINDOW_MAXIMIZE "##doc_popin", "Move back into X-Plane", ImVec2(24.0f, 0.0f))) {
            mNextWinPosMode = xplm_WindowPositionFree;
            if (s_DocViewerFlightLoop) XPLMScheduleFlightLoop(s_DocViewerFlightLoop, -1.0f, 1);
        }
    } else {
        if (ToolbarButton(ICON_FA_WINDOW_RESTORE "##doc_popout", "Pop out into separate OS desktop window", ImVec2(24.0f, 0.0f))) {
            GetWindowGeometry(mSavedSimLeft, mSavedSimTop, mSavedSimRight, mSavedSimBottom);
            mSavedSimZoom = mFontAdjust;
            mHasSavedSimGeometry = true;
            mNextWinPosMode = xplm_WindowPopOut;
            if (s_DocViewerFlightLoop) XPLMScheduleFlightLoop(s_DocViewerFlightLoop, -1.0f, 1);
        }
    }

    // Close Button [X]
    ImGui::SameLine(0.0f, 4.0f);
    if (ToolbarButton(ICON_FA_WINDOW_CLOSE "##doc_close", "Close User Guide", ImVec2(24.0f, 0.0f))) {
        RequestClose();
    }

    ImGui::Separator();
}

void DocViewerImgWindow::drawContent()
{
    // Check if file was modified on disk and hot-reload:
    if (!mDocumentPath.empty()) {
        struct stat st;
        if (stat(mDocumentPath.c_str(), &st) == 0 && st.st_mtime != mLastMtime) {
            loadDocument();
        }
    }

    float padH = std::round(25.0f * mFontAdjust);
    float padV = std::round(18.0f * mFontAdjust);
    float itemSpacingY = std::round(3.0f * mFontAdjust);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padH, padV));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, itemSpacingY));

    // Balanced bezel inset on left and right when inside the simulator (fixed boxels)
    // so the vertical scrollbar and content do not collide with X-Plane's native window resize grab zones.
    float bezelMargin = IsInsideSim() ? 7.0f : 0.0f;
    if (bezelMargin > 0.0f) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + bezelMargin);
    }
    ImGui::BeginChild("DocContentScroll", ImVec2(-bezelMargin, 0.0f), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    // Apply active font scale to the markdown canvas:
    ImGui::SetWindowFontScale(mFontAdjust);

    // Configure dedicated markdown image resolver with in-depth path discovery & typography:
    ImGui::MarkdownConfig mdConfig;
    const ImGui::MarkdownConfig* defCfg = ImGui::GetDefaultMarkdownConfig();
    if (defCfg) {
        mdConfig = *defCfg;
    }

    std::string docDir;
    if (!mDocumentPath.empty()) {
        size_t s = mDocumentPath.find_last_of("/\\");
        if (s != std::string::npos) {
            docDir = mDocumentPath.substr(0, s);
        }
    }
    if (!docDir.empty()) {
        mdConfig.baseDir = docDir.c_str();
    }

    mdConfig.imageCallback = &ImGui::MarkdownImageService::DefaultMarkdownImageCallback;
    mdConfig.enableImageLightbox = true;
    mdConfig.showImageCaptions = true;
    mdConfig.enableTableTooltips = true;

    // Use dynamic content-region wrap (0.0f) so Dear ImGui automatically accounts
    // for bullet indents, nested lists, and scrollbars without text overshooting the margin:
    mdConfig.wrapWidth = 0.0f;
    if (IMG_TITLE_FONT)      mdConfig.h1Font = IMG_TITLE_FONT;
    if (IM_BOLD_LARGER_FONT) mdConfig.h2Font = IM_BOLD_LARGER_FONT;
    if (IM_BOLD_FONT)        mdConfig.h3Font = IM_BOLD_FONT;
    if (IM_BOLD_FONT)        mdConfig.boldFont = IM_BOLD_FONT;
    if (IM_ITALIC_FONT)      mdConfig.italicFont = IM_ITALIC_FONT;
    if (IM_MONO_NORMAL_FONT) mdConfig.monoFont = IM_MONO_NORMAL_FONT;

    // Render markdown document with content-region wrapping:
    ImGui::Markdown::TextWrapped(mContent, &mdConfig);

    ImGui::EndChild();
    ImGui::PopStyleVar(2);
}

void DocViewerImgWindow::buildInterface()
{
    static int s_currentTheme = 0;
    static ImGuiContext* s_lastThemeContext = nullptr;
    if (s_lastThemeContext != ImGui::GetCurrentContext()) {
        s_lastThemeContext = ImGui::GetCurrentContext();
        ImGui::ApplyDefaultTheme();
        ImGuiStyle& st = ImGui::GetStyle();
        st.Colors[ImGuiCol_WindowBg].w = 0.95f;
        st.Colors[ImGuiCol_Border].w = 0.8f;
    }

    drawToolbar();
    drawContent();

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
            sDocViewerZoom = mFontAdjust;

            mSavedSimLeft = l;
            mSavedSimTop = t;
            mSavedSimRight = r;
            mSavedSimBottom = b;
            mSavedSimZoom = mFontAdjust;
            mHasSavedSimGeometry = true;
        }
    }
}
