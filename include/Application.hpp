#pragma once

#include <SDL3/SDL.h>
// #include <SDL3/SDL_main.h>
#include <SDL3/SDL_opengl.h>

#include <imgui.h>

#include <thread>
#include <vector>

#include <video.hpp>
#include <widgets.hpp>


class Application {
public:
    Application() {}

    bool isRunning() { return running; }

    void setup();
    void run();
    void exit();
protected:
    bool running = false;
    bool showUploadDialog = false;
    float convertProgress = 0.f;

    bool firstFrame;

    std::chrono::time_point<std::chrono::steady_clock> lastFrameTime;

    SDL_Window* window;
    SDL_GLContext gl_context;

    GLuint imageTexture;

    VideoTimeline timeline;

    std::shared_ptr<Frame> frame;

    void setupStyle();
    void draw();

    void togglePlay();

    bool initSDL();
    bool initImGui();

    template<class T>
    void drawClipButton(std::string name, int defaultDuration);

    std::thread convertThread;

    std::vector<unsigned char> lastRenderedFrameData;

    bool isDraggingClip = false;
    Vector2D initialPos;
};
