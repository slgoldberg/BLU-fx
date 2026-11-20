#pragma once

#include "contrib/ImgDocWindow.h"
#include "settings-img-window.h"

#include <string>
#include <memory>

class DocViewerImgWindow : public ImgDocWindow {
public:
    static constexpr int DEFAULT_WIDTH  = 760;
    static constexpr int DEFAULT_HEIGHT = 680;

    DocViewerImgWindow();
    ~DocViewerImgWindow() override;

    void RequestClose();

    static DocViewerImgWindow* GetInstance();
    static void ShowInstance();
    static void ToggleInstance();
    static void DestroyInstance();
    static bool IsOpen();
    static float DeferredFlightLoopCB(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon);

protected:
    void buildInterface() override;
    void onDocumentClosed() override;
    void onDocumentZoomChanged(float oldZoom, float newZoom) override;
    void onDocumentThemeChanged(int themeIndex, bool transparentHud, float opacity) override;

private:
    ImGui::MarkdownConfig mMdConfig{};

    static std::unique_ptr<DocViewerImgWindow> sInstance;
};
