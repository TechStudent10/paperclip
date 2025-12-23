#define SDL_MAIN_HANDLED
#include <glad/include/glad/gl.h>
#include <shaders/shader.hpp>
#include <Application.hpp>
#include <fmt/base.h>
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <fmt/color.h>

#include <state.hpp>
#include <widgets.hpp>

#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>
#include <misc/cpp/imgui_stdlib.h>

#include <utils.hpp>

#include <action/actions/CreateClip.hpp>

bool Application::ButtonCenteredOnLine(const char* label, float alignment) {
    ImGuiStyle& style = ImGui::GetStyle();

    float size = ImGui::CalcTextSize(label).x + style.FramePadding.x * 2.0f;
    float avail = ImGui::GetContentRegionAvail().x;

    float off = (avail - size) * alignment;
    if (off > 0.0f)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

    return ImGui::Button(label);
}

bool Application::ImageButtonCenteredOnLine(const char* label, IconType iconType, float alignment) {
    ImGuiStyle& style = ImGui::GetStyle();

    auto icon = icons[iconType];

    // float size = ImGui::CalcTextSize(label).x + style.FramePadding.x * 2.0f;
    float size = icon.w;
    float avail = ImGui::GetContentRegionAvail().x;

    float off = (avail - size) * alignment;
    if (off > 0.0f)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

    return ImGui::ImageButton(label, icon.texture, ImVec2(icon.w, icon.h));
}

void TextCentered(std::string text, float alignment = 0.5f) {
    auto windowWidth = ImGui::GetContentRegionAvail().x;
    auto textWidth   = ImGui::CalcTextSize(text.c_str()).x;

    ImGui::SetCursorPosX((windowWidth - textWidth) * alignment);
    ImGui::Text("%s", text.c_str());
}

void Application::draw() {
    auto& state = State::get();

    drawMenuBar();
    drawViewport();

    drawMediaWindow();
    drawPropertiesWindow();
    drawTrackWindow();

    using namespace std::chrono;

    if (state.isPlaying) {
        auto now = steady_clock::now();
        auto elapsed = duration_cast<milliseconds>(now - lastFrameTime).count();

        double msPerFrame = 1000.0 / state.video->getFPS();

        if (elapsed >= msPerFrame) {
            int framesToAdvance = static_cast<int>(elapsed / msPerFrame);

            setCurrentFrame(state.currentFrame + framesToAdvance);

            if (state.currentFrame >= state.video->frameCount) {
                state.isPlaying = false;
                for (auto audTrack : state.video->audioTracks) {
                    audTrack->onStop();
                }
            }

            if (state.isPlaying) {
                for (auto audTrack : state.video->audioTracks) {
                    audTrack->processTime();
                }
            }

            lastFrameTime += milliseconds(
                static_cast<int>(framesToAdvance * msPerFrame)
            );
        }
    }

    auto resolution = state.video->getResolution();
    if (state.lastRenderedFrame != state.currentFrame) {
        frame->clearFrame();
        state.video->renderIntoFrame(state.currentFrame, frame);
        state.lastRenderedFrame = state.currentFrame;
    }

    ImGui::SetNextWindowClass(&bareWindowClass);
    ImGui::Begin("Player");
    ImVec2 avail = ImGui::GetContentRegionAvail();
    auto scale = ImMin(avail.x / resolution.x, (avail.y - 75) / resolution.y);
    ImVec2 imageSize = ImVec2(resolution.x * scale, resolution.y * scale);
    ImVec2 imagePos = ImVec2((ImGui::GetWindowSize().x - imageSize.x) * 0.5f , ImGui::GetCursorPosY());

    drawPlayerWindow(imageSize, imagePos, scale);
    drawPlayerControls(imageSize);
}

void Application::setCurrentFrame(int frame) {
    auto& state = State::get();
    frame = std::clamp(frame, 0, state.video->frameCount);
    state.currentFrame = frame;
}

template<class T>
void Application::drawClipButton(std::string name, int defaultDuration) {
    if (ImGui::Button(name.c_str(), ImVec2(-1.0f, 0.0f))) {
        timeline.isPlacingClip = true;
        timeline.placeType = TrackType::Video;
        timeline.placeDuration = defaultDuration;
        timeline.placeCb = [this, defaultDuration](int frame, int trackIdx) {
            auto& state = State::get();

            auto clip = std::make_shared<T>();
            clip->startFrame = frame;
            clip->duration = defaultDuration;
            auto action = std::make_shared<CreateClip>(clip, clip->getType(), trackIdx);
            
            action->perform();
            state.selectClip(clip);
            state.addAction(action);

            state.lastRenderedFrame = -1;
        };
    }
}

template void Application::drawClipButton<clips::Rectangle>(std::string name, int defaultDuration);
template void Application::drawClipButton<clips::Circle>(std::string name, int defaultDuration);
template void Application::drawClipButton<clips::Text>(std::string name, int defaultDuration);

void Application::togglePlay() {
    auto& state = State::get();
    bool stopping = state.currentFrame >= state.video->frameCount;
    if (stopping) {
        state.currentFrame = 0;
    }
    state.isPlaying = !state.isPlaying;
    lastFrameTime = std::chrono::steady_clock::now();

    for (auto audTrack : state.video->audioTracks) {
        if (state.isPlaying) {
            audTrack->onPlay();
        } else if (stopping || !state.isPlaying) {
            audTrack->onStop();
        }
    }
}

void Application::exit() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Application::run() {
    if (isRunning()) {
        return;
    }

    running = true;
    firstFrame = true;
    lastFrameTime = std::chrono::steady_clock::now();

    auto& state = State::get();
    auto resolution = state.video->getResolution();
    frame = std::make_shared<Frame>(resolution.x, resolution.y);

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (ImGui::IsKeyPressed(ImGuiKey_LeftShift) && event.motion.xrel > 100 && event.motion.yrel > 100) {
                timeline.trackScrollY += std::min(event.wheel.y, 1.f);
                timeline.trackScrollY = std::max(std::min(timeline.trackScrollY, -2.2f), state.video->getTracks().size() - 9.2f);
            }
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
                break;
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                if (!ImGui::IsAnyItemActive() && !ImGui::IsAnyItemFocused()) {
                    switch (event.key.key) {
                        case SDLK_SPACE:
                            togglePlay();
                            break;
                        case SDLK_D:
                            if (event.key.mod & SDL_KMOD_ALT) {
                                state.deselect();
                            }
                            break;
                        case SDLK_Y:
                            if (event.key.mod & SDL_KMOD_CTRL && !(event.key.mod & SDL_KMOD_SHIFT)) {
                                state.redo();
                            }
                            break;
                        case SDLK_Z:
                            if (event.key.mod & SDL_KMOD_CTRL && !(event.key.mod & SDL_KMOD_SHIFT)) {
                                state.undo();
                            } else if (event.key.mod & SDL_KMOD_CTRL | SDL_KMOD_SHIFT) {
                                state.redo();
                            }
                            break;
                    }
                }
            }
        }
        
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        int win_w, win_h;
        SDL_GetWindowSize(window, &win_w, &win_h);

        glViewport(0, 0, win_w, win_h);
        // glClearColor(1.f, 0.1f, 1.f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        // actual drawing
        draw();

        // ImGui::ShowDemoWindow();
        
        /* clear the window to the draw color. */


        ImGui::Render();
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        /* put the newly-cleared rendering on the screen. */
        SDL_GL_SwapWindow(window);

        SDL_Delay(25);
    }

    exit();
}
