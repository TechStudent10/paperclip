#pragma once

#include <SDL3/SDL.h>
// #include <SDL3/SDL_main.h>
#include <imgui.h>

#include <thread>
#include <vector>

#include <video.hpp>
#include <SDL3/SDL_opengl.h>
#include <widgets.hpp>

struct IconInfo {
    GLuint texture;
    int w = 0, h = 0;
};

enum class IconType {
    PlayPause
};

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

    VideoTimeline timeline;

    std::shared_ptr<Frame> frame;

    void setupStyle();
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
