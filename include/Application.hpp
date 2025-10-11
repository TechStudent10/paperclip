#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <imgui.h>

#include <thread>
#include <vector>

#include <video.hpp>
#include <SDL3/SDL_opengl.h>
#include <widgets.hpp>

constexpr float quadVertices[] = {
    // X, Y, U, V
    -1.0f, -1.0f,  0.0f, 0.0f,  // bottom-left
     1.0f, -1.0f,  1.0f, 0.0f,  // bottom-right
     1.0f,  1.0f,  1.0f, 1.0f,  // top-right

    -1.0f, -1.0f,  0.0f, 0.0f,  // bottom-left
     1.0f,  1.0f,  1.0f, 1.0f,  // top-right
    -1.0f,  1.0f,  0.0f, 1.0f   // top-left
};

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

    // GLuint imageTexture;
    // GLuint quadVAO, quadVBO, quadShaderProgram;

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
