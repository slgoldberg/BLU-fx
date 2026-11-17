#pragma once

#include "ImgWindow.h"
#include "contrib.h"
#include "sample-code/fonts/IconsFontAwesome5.h"
#include "settings-img-window.h"

#include <string>
#include <memory>
#include <vector>

class DocViewerImgWindow : public ImgWindow {
public:
    static constexpr int DEFAULT_WIDTH  = 760;
    static constexpr int DEFAULT_HEIGHT = 680;

    DocViewerImgWindow();
    virtual ~DocViewerImgWindow();

    void RequestClose();
    void ReloadDocument();
    const std::string& GetDocumentPath() const { return mDocumentPath; }

    static DocViewerImgWindow* GetInstance();
    static void ShowInstance();
    static void ToggleInstance();
    static void DestroyInstance();
    static bool IsOpen();
    static float DeferredFlightLoopCB(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon);

    struct WindowRectSnapshot {
        int  l = 0;
        int  t = 0;
        int  r = 0;
        int  b = 0;
        bool valid = false;
    };

protected:
    void buildInterface() override;

private:
    void drawToolbar();
    void drawContent();
    void loadDocument();
    void applyTheme(int themeIdx);
    void applyZoomTransition(float oldZoom, float newZoom);

    WindowRectSnapshot& ActiveBaselineRect();
    WindowRectSnapshot& ActivePriorZoomRect();
    void CaptureCurrentWindowRect(WindowRectSnapshot& rect) const;
    void RestoreWindowRect(const WindowRectSnapshot& rect);

    std::string mDocumentPath;
    std::string mPdfPath;
    std::string mContent;
    bool mLoaded = false;
    time_t mLastMtime = 0;

    float mFontAdjust = 1.0f;
    float mLastZoom = 1.0f;
    float mSliderStartZoom = 1.0f;
    int mDecRepeatCount = 0;
    int mIncRepeatCount = 0;
    int mCurrentTheme = 0; // 0 = Charcoal, 1 = Dark, 2 = Classic, 3 = Light

    WindowRectSnapshot _simBaselineRect;
    WindowRectSnapshot _osBaselineRect;
    WindowRectSnapshot _vrBaselineRect;
    WindowRectSnapshot _simPriorZoomRect;
    WindowRectSnapshot _osPriorZoomRect;
    WindowRectSnapshot _vrPriorZoomRect;

    int mSavedSimLeft = 0;
    int mSavedSimTop = 0;
    int mSavedSimRight = 0;
    int mSavedSimBottom = 0;
    float mSavedSimZoom = 1.0f;
    bool mHasSavedSimGeometry = false;

    XPLMWindowPositioningMode mNextWinPosMode = (XPLMWindowPositioningMode)-1;

    static std::unique_ptr<DocViewerImgWindow> sInstance;
};
