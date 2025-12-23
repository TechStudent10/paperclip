#pragma once

#include <SDL3/SDL.h>
// #include <SDL3/SDL_main.h>
#include <imgui.h>

#include <thread>
#include <vector>

#include <video.hpp>
#include <SDL3/SDL_opengl.h>
#include <widgets.hpp>

#include <nfd.h>

struct IconInfo {
    GLuint texture;
    int w = 0, h = 0;
};

enum class IconType {
    PlayPause
};

inline nfdnfilteritem_t createFilter(const char* name, const char* spec) {
#ifndef WIN32
    return { name, spec };
#else
    return { GetWC(name), GetWC(spec) };
#endif
}
const nfdnfilteritem_t PROJECT_FILES_FILTER[] = { createFilter("Project Files", "pclp") };
const nfdnfilteritem_t VIDEO_FILES_FILTER [] = { createFilter("Video Files", "mp4,mov") };
const nfdnfilteritem_t IMAGE_FILES_FILTER [] = { createFilter("Image Files", "png,jpg,jpeg") };
const nfdnfilteritem_t AUDIO_FILES_FILTER [] = { createFilter("Audio Files", "mp3") };

// wow i hate windows
// https://stackoverflow.com/a/8032108
#ifdef WIN32
#include <winnls.h>
inline const wchar_t* GetWC(const char* c) {
    std::string str(c);
    int count = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), NULL, 0);
    std::wstring wstr(count, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), &wstr[0], count);
    return wstr.c_str();
}

inline const char* ensureCStr(const wchar_t* c) {
    std::wstring wstr(c);
    int count = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wstr.length(), NULL, 0, NULL, NULL);
    std::string str(count, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], count, NULL, NULL);
    return str.c_str();
}

#else

inline const char* ensureCStr(const char* c) {
    return c;
}

#endif

class Application {
public:
    Application() {}

    bool isRunning() { return running; }

    void setup();
    void run();
    void exit();
protected:
    // this gets loaded in at init time
    // (icon ID, icon info)
    std::unordered_map<IconType, IconInfo> icons;

    bool running = false;
    float convertProgress = 0.f;

    bool firstFrame;

    std::chrono::time_point<std::chrono::steady_clock> lastFrameTime;

    SDL_Window* window;
    SDL_GLContext gl_context;

    Timeline timeline;

    std::shared_ptr<Frame> frame;

    ImGuiWindowClass bareWindowClass;

    void setupStyle();

    void drawMenuBar();
    void drawViewport();

    void drawMediaWindow();
    void drawPropertiesWindow();
    void drawTrackWindow();
    void drawPlayerWindow(ImVec2 imageSize, ImVec2 imagePos, float scale);
    void drawPlayerControls(ImVec2 imageSize);

    void draw();

    void togglePlay();

    bool initSDL();
    bool initImGui();
    bool initIcons();

    bool ButtonCenteredOnLine(const char* label, float alignment = 0.5f);
    bool ImageButtonCenteredOnLine(const char* label, IconType iconType, float alignment = 0.5f);

    void setCurrentFrame(int frame);

    template<class T>
    void drawClipButton(std::string name, int defaultDuration);

    std::thread convertThread;

    std::vector<unsigned char> lastRenderedFrameData;

    bool isDraggingClip = false;
    Vector2D initialPos;

    ImFont* progressFont;
};
