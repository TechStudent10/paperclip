#include <glad/include/glad/gl.h>
#include <shaders/shader.hpp>
#include <shaders/quad.hpp>
#include <Application.hpp>
#include <fmt/base.h>
#include <fmt/format.h>
#include <fmt/chrono.h>

#include <state.hpp>
#include <widgets.hpp>

#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>
#include <misc/cpp/imgui_stdlib.h>
#include <nfd.h>
#include <miniaudio.h>

#include <filesystem>

#include <fstream>

#include <renderer/audio.hpp>

#include <utils.hpp>

#include <stb_image.h>

#include <icons/IconsFontAwesome6.h>

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

constexpr nfdnfilteritem_t PROJECT_FILES_FILTER[] = { {"Project Files", "pclp"} };
constexpr nfdnfilteritem_t MP4_FILES_FILTER[] = { {"MP4 Files", "mp4"} };
constexpr nfdnfilteritem_t VIDEO_FILES_FILTER [] = { {"Video Files", "mp4,mov"} };
constexpr nfdnfilteritem_t IMAGE_FILES_FILTER [] = { {"Image Files", "png,jpg,jpeg"} };
constexpr nfdnfilteritem_t AUDIO_FILES_FILTER [] = { {"Audio Files", "mp3"} };

void Application::draw() {
    auto& state = State::get();

    ImGuiID exportId = ImGui::GetID("Export Menu");

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save As")) {
                nfdchar_t *outPath = NULL;
                nfdresult_t result = NFD_SaveDialogN(
                    &outPath,
                    PROJECT_FILES_FILTER,
                    std::size(PROJECT_FILES_FILTER),
                    nullptr,
                    nullptr
                );

                if (result == NFD_OKAY) {
                    std::ofstream file(outPath, std::ios::binary);
                    qn::HeapByteWriter writer;
                    state.video->write(writer);
                    auto written = writer.written();
                    file.write(reinterpret_cast<const char*>(written.data()), written.size());
                    file.close();
                }
                else if (result == NFD_CANCEL) {}
            }

            if (ImGui::MenuItem("Open")) {
                nfdchar_t *outPath = NULL;
                nfdresult_t result = NFD_OpenDialogN(
                    &outPath,
                    PROJECT_FILES_FILTER,
                    std::size(PROJECT_FILES_FILTER),
                    nullptr
                );

                if (result == NFD_OKAY) {
                    std::ifstream file(outPath, std::ios::binary);
                    std::vector<unsigned char> fileBuffer(std::istreambuf_iterator<char>(file), {});
                    qn::ByteReader reader(fileBuffer);
                    std::shared_ptr<Video> video = std::make_shared<Video>();
                    video->read(reader);
                    fmt::println("{}", video->getTracks().size());

                    state.video = video;
                }
                else if (result == NFD_CANCEL) {}

                if (outPath) { NFD_FreePathN(outPath); }
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Tracks")) {
            if (ImGui::MenuItem("New Video Track")) {
                state.video->addTrack(std::make_shared<VideoTrack>());
            }

            if (ImGui::MenuItem("New Audio Track")) {
                state.video->audioTracks.push_back(std::make_shared<AudioTrack>());
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Rendering")) {
            if (ImGui::MenuItem("Export")) {
                ImGui::OpenPopup(exportId);
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
    if (ImGui::BeginPopupModal("Export Menu")) {
        ImGui::Text("Export your video here");

        ImGui::SeparatorText("Options");

        ImGui::InputText("Path", &state.exportPath);
        ImGui::SameLine();
        if (ImGui::Button("File Picker")) {
            nfdchar_t *outPath = NULL;
            nfdresult_t result = NFD_SaveDialogN(
                &outPath,
                MP4_FILES_FILTER,
                std::size(MP4_FILES_FILTER),
                nullptr,
                nullptr
            );

            if (result == NFD_OKAY) {
                state.exportPath = std::move(outPath);
                NFD_FreePathN(outPath);
            }
            else if (result == NFD_CANCEL) {}
        }

        ImGui::Separator();

        if (ImGui::Button("Export")) {
            // .mp4
            auto exportPath = std::filesystem::path(state.exportPath);
            auto videoFilename = std::filesystem::path(exportPath)
                .replace_extension(
                    fmt::format(".na.{}", exportPath.extension().string())
                ).string();

            auto audioFilename = fmt::format("{}.wav", state.exportPath);
            VideoRenderer renderer(videoFilename, state.video->getResolution().x, state.video->getResolution().y, state.video->getFPS());
            state.video->render(&renderer);

            AudioRenderer audio(audioFilename, state.video->timeForFrame(state.video->frameCount));
            for (auto track : state.video->audioTracks) {
                for (auto _clip : track->getClips()) {
                    auto clip = _clip.second;
                    audio.addClip(
                        clip->m_metadata.name,
                        state.video->timeForFrame(clip->startFrame),
                        state.video->timeForFrame(clip->startFrame + clip->duration),
                        clip->m_properties.getProperty("volume")
                    );
                }
            }
            audio.render(state.video->getFPS());

            // renderer.addAudio(pcmData);

            renderer.finish();

            utils::video::combineAV(audioFilename, videoFilename, state.exportPath);

            // ImGui::InsertNotification({
            //     ImGuiToastType::Success,
            //     6000,
            //     "Render finish!"
            // });
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Close")) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", nullptr,
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoBackground);
    ImGuiID dockspace_id = ImGui::GetID("dockspace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    ImGui::End();
    ImGui::PopStyleVar();

    if (firstFrame) {
        firstFrame = false;
        ImGuiID dock_main_id = dockspace_id;

        ImGui::DockBuilderRemoveNode(dock_main_id);
        ImGui::DockBuilderAddNode(dock_main_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dock_main_id, viewport->WorkSize);

        ImGuiID dock_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, nullptr, &dock_main_id);
        ImGuiID dock_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.4f, nullptr, &dock_main_id);
        ImGuiID dock_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.25f, nullptr, &dock_main_id);

        ImGui::DockBuilderDockWindow("Media", dock_left);
        ImGui::DockBuilderDockWindow("Properties", dock_right);
        ImGui::DockBuilderDockWindow("Track", dock_bottom);
        ImGui::DockBuilderDockWindow("Player", dock_main_id);

        ImGui::DockBuilderFinish(dock_main_id);
    }

    ImGuiWindowClass bareWindowClass;
    bareWindowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoWindowMenuButton;

    ImGui::SetNextWindowClass(&bareWindowClass);
    ImGui::Begin("Media");

    static int defaultTime = state.video->frameForTime(5);

    if (ImGui::BeginTabBar("MediaTabBar")) {
        if (ImGui::BeginTabItem("Built-in")) {
            drawClipButton<clips::Rectangle>("Rectangle", defaultTime);
            drawClipButton<clips::Circle>("Circle", defaultTime);
            drawClipButton<clips::Text>("Text", defaultTime);

            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Pool")) {
            if (ImGui::Button("Import Videos")) {
                nfdchar_t *outPath = NULL;
                nfdresult_t result = NFD_OpenDialogN(
                    &outPath,
                    VIDEO_FILES_FILTER,
                    std::size(VIDEO_FILES_FILTER),
                    nullptr
                );

                if (result == NFD_OKAY) {
                    std::string outFile = std::move(outPath);
                    convertThread = std::thread([outFile]() {
                        utils::video::extractAudio(outFile);

                        ma_decoder decoder;
                        ma_uint64 pcmLength;
                        ma_decoder_init_file(fmt::format("{}.mp3", outFile).c_str(), NULL, &decoder);
                        ma_decoder_get_length_in_pcm_frames(&decoder, &pcmLength);

                        auto& state = State::get();
                        int frameCount = state.video->frameForTime((float)pcmLength / (float)decoder.outputSampleRate);

                        state.video->clipPool.push_back({
                            .filePath = outFile,
                            .frameCount = frameCount
                        });
                        state.video->audioClipPool.push_back({
                            .filePath = fmt::format("{}.mp3", outFile),
                            .frameCount = frameCount
                        });
                    });
                    convertThread.detach();
                }
                else if (result == NFD_CANCEL) {}
            }

            if (ImGui::Button("Import Audio")) {
                nfdchar_t *outPath = NULL;
                nfdresult_t result = NFD_OpenDialogN(
                    &outPath,
                    AUDIO_FILES_FILTER,
                    std::size(AUDIO_FILES_FILTER),
                    nullptr
                );

                if (result == NFD_OKAY) {
                    std::string outFile = std::move(outPath);
                    free(outPath);

                    ma_decoder decoder;
                    ma_uint64 pcmLength;
                    ma_decoder_init_file(outFile.c_str(), NULL, &decoder);
                    ma_decoder_get_length_in_pcm_frames(&decoder, &pcmLength);

                    int frameCount = state.video->frameForTime((float)pcmLength / (float)decoder.outputSampleRate);

                    state.video->audioClipPool.push_back({
                        .filePath = outFile,
                        .frameCount = frameCount
                    });

                    ma_decoder_uninit(&decoder);
                }
                else if (result == NFD_CANCEL) {}
            }

            if (ImGui::Button("Import Image")) {
                nfdchar_t *outPath = NULL;
                nfdresult_t result = NFD_OpenDialogN(
                    &outPath,
                    IMAGE_FILES_FILTER,
                    std::size(IMAGE_FILES_FILTER),
                    nullptr
                );

                if (result == NFD_OKAY) {
                    std::string outFile = std::move(outPath);
                    state.video->imagePool.push_back({
                        .filePath = outFile,
                        .frameCount = 300
                    });
                }
                else if (result == NFD_CANCEL) {}
            }

            ImGui::SeparatorText("Videos");

            for (auto clipMeta : state.video->clipPool) {
                if (ImGui::Button(clipMeta.filePath.c_str(), ImVec2(-1.0f, 0.0f))) {
                    timeline.isPlacingClip = true;
                    timeline.placeType = TrackType::Video;
                    timeline.placeCb = [this, clipMeta](int frame, int trackIdx) {
                        auto& state = State::get();
                        auto clip = std::make_shared<clips::VideoClip>(clipMeta.filePath);
                        clip->startFrame = frame;
                        clip->duration = clipMeta.frameCount;
                        state.lastRenderedFrame = -1;
                        state.video->addClip(trackIdx, clip);
                    };
                }
            }

            ImGui::SeparatorText("Audio");

            for (auto soundFile : state.video->audioClipPool) {
                if (ImGui::Button(soundFile.filePath.c_str(), ImVec2(-1.0f, 0.0f))) {
                    timeline.isPlacingClip = true;
                    timeline.placeType = TrackType::Audio;
                    timeline.placeCb = [this, soundFile](int frame, int trackIdx) {
                        auto& state = State::get();
                        auto clip = std::make_shared<AudioClip>(soundFile.filePath);
                        clip->startFrame = frame;
                        clip->duration = soundFile.frameCount;
                        clip->m_properties.getProperties()["volume"]->data = Vector1D{ .number = 100 }.toString();
                        state.lastRenderedFrame = -1;
                        state.video->addAudioClip(trackIdx, clip);
                    };
                }
            }

            ImGui::SeparatorText("Images");

            for (auto imageFile : state.video->imagePool) {
                if (ImGui::Button(imageFile.filePath.c_str(), ImVec2(-1.0f, 0.0f))) {
                    timeline.isPlacingClip = true;
                    timeline.placeType = TrackType::Video;
                    timeline.placeCb = [this, imageFile](int frame, int trackIdx) {
                        auto& state = State::get();
                        auto clip = std::make_shared<clips::ImageClip>(imageFile.filePath);
                        clip->startFrame = frame;
                        clip->duration = imageFile.frameCount;
                        state.lastRenderedFrame = -1;
                        state.video->getTracks()[trackIdx]->addClip(clip);
                    };
                }
            }

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();

    ImGui::SetNextWindowClass(&bareWindowClass);
    ImGui::Begin("Properties");
    auto setData = [&](std::shared_ptr<ClipProperty> property, std::string data) {
        if (property->keyframes.size() == 1) {
            property->keyframes[0] = data;
            state.lastRenderedFrame = -1;
            for (auto audTrack : state.video->audioTracks) {
                audTrack->processTime();
            }
            return;
        }

        auto selectedClip = state.getSelectedClip();
        int keyframe = state.currentFrame - selectedClip->startFrame;
        selectedClip->m_properties.setKeyframe(property->id, keyframe, data);
        state.lastRenderedFrame = -1;
        for (auto audTrack : state.video->audioTracks) {
            audTrack->processTime();
        }
    };
    if (state.isClipSelected()) {
        auto drawDimensions = [&](std::shared_ptr<ClipProperty> property) {
            Dimensions dimensions = Dimensions::fromString(property->data);

            bool x = ImGui::DragInt("X", &dimensions.pos.x);
            bool y = ImGui::DragInt("Y", &dimensions.pos.y);
            bool w = ImGui::DragInt("Width", &dimensions.size.x);
            bool h = ImGui::DragInt("Height", &dimensions.size.y);

            if (x || y || w || h) {
                setData(property, dimensions.toString());
            }
        };

        auto drawPosition = [&](std::shared_ptr<ClipProperty> property) {
            Vector2D position = Vector2D::fromString(property->data);

            bool x = ImGui::DragInt("X", &position.x);
            bool y = ImGui::DragInt("Y", &position.y);

            if (x || y) {
                setData(property, position.toString());
            }
        };

        auto drawInt = [&](std::shared_ptr<ClipProperty> property) {
            Vector1D number = Vector1D::fromString(property->data);
            if (ImGui::DragInt(
                fmt::format("##{}", property->name).c_str(),
                &number.number,
                1.0f,
                0,
                property->type == PropertyType::Percent ? 100 : 0
            )) {
                setData(property, number.toString());
            }
        };

        auto drawColorPicker = [&](std::shared_ptr<ClipProperty> property) {
            RGBAColor color = RGBAColor::fromString(property->data);

            float colorBuf[4] = { (float)color.r / 255, (float)color.g / 255, (float)color.b / 255, (float)color.a / 255 };
            if (ImGui::ColorPicker4("Color", colorBuf)) {
                color.r = std::round(colorBuf[0] * 255);
                color.g = std::round(colorBuf[1] * 255);
                color.b = std::round(colorBuf[2] * 255);
                color.a = std::round(colorBuf[3] * 255);

                setData(property, color.toString());
            }
        };

        auto drawText = [&](std::shared_ptr<ClipProperty> property) {
            std::string text = property->data;
            if (ImGui::InputText(fmt::format("##{}", property->id).c_str(), &text)) {
                setData(property, text);
            }
        };

        auto selectedClip = state.getSelectedClip();

        TextCentered(selectedClip->m_metadata.name);

        ImGui::Separator();

        if (ImGui::Button("Open Keyframe Editor", ImVec2(-1.0f, 0.0f))) {
            ImGui::OpenPopup("Keyframe Editor");

        }
        if (ImGui::BeginPopup("Keyframe Editor")) {
            for (auto prop : selectedClip->m_properties.getProperties()) {
                ImGui::Text("%s", prop.second->name.c_str());
            }
            ImGui::EndPopup();
        }

        ImGui::SeparatorText("Properties");

        for (auto prop : selectedClip->m_properties.getProperties()) {
            ImGui::SeparatorText(prop.second->name.c_str());
            prop.second->drawProperty();
        }

        if (ImGui::Button("Delete")) {
            ImGui::OpenPopup("Delete confirmation");
        }
        if (ImGui::BeginPopupModal("Delete confirmation")) {
            ImGui::Text("Are you sure you want to delete this clip?");
            ImGui::Separator();
            if (ImGui::Button("Yes")) {
                int trackIdx = state.video->getClipMap()[selectedClip->uID];
                if (timeline.selectedTrackType == TrackType::Audio) {
                    state.video->removeAudioClip(trackIdx, std::dynamic_pointer_cast<AudioClip>(selectedClip));
                } else {
                    state.video->removeClip(trackIdx, selectedClip);
                }
                selectedClip->onDelete();
                state.deselect();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("No")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
    ImGui::End();

    // fmt::println("number of clips in vid track 0: {}", state.video->videoTracks[0]->getClips().size());

    ImGui::SetNextWindowClass(&bareWindowClass);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Track", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    timeline.render();
    ImGui::End();
    ImGui::PopStyleVar();

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
    ImGui::SetCursorPos(imagePos);
    ImGui::Image((ImTextureID)(uintptr_t)frame->textureID, imageSize);

    ImGuiIO& io = ImGui::GetIO();

    ImGui::SetCursorPos(imagePos);

    auto absMousePos = io.MousePos;
    auto windowPos = ImGui::GetWindowPos();
    auto mousePos = ImVec2(
        absMousePos.x - imagePos.x - windowPos.x,
        absMousePos.y - imagePos.y - windowPos.y
    );

    if (ImGui::InvisibleButton("btn", imageSize)) {
        // fmt::println("btn");
    }

    auto selectedClip = state.getSelectedClip();

    if (ImGui::IsItemHovered() && ImGui::IsMouseDown(0)) {
        // mouse position in the canvas
        // in terms of the video resolution
        auto canvasX = static_cast<int>(std::round(mousePos.x / scale));
        auto canvasY = static_cast<int>(std::round(mousePos.y / scale));
        
        bool deselected = false;

        if (isDraggingClip && io.MouseDownDuration[0] == 0 && state.isClipSelected()) {
            // check if canvasX, canvasY are
            // outside of the clip bounding box
            // if so, deselect

            Vector2D position = selectedClip->getPos();
            Vector2D size = selectedClip->getSize();

            // top 10 laziest developer moments
            if (!(canvasX >= position.x && canvasX <= position.x + size.x &&
                canvasY >= position.y && canvasY <= position.y + size.y)
            ) {
                isDraggingClip = false;
                state.deselect();    
                deselected = true;   
            }
        }

        if (!deselected) {
            if (isDraggingClip && state.isClipSelected()) {
                int dx = canvasX - initialPos.x;
                int dy = canvasY - initialPos.y;

                initialPos = { canvasX, canvasY };

                if (selectedClip->m_properties.getProperties().contains("position")) {
                    auto property = selectedClip->m_properties.getProperty("position");
                    auto oldPos = Vector2D::fromString(property->data);
                    Vector2D newPos = {
                        .x = std::clamp(oldPos.x + dx, 0, state.video->resolution.x),
                        .y = std::clamp(oldPos.y + dy, 0, state.video->resolution.y)
                    };
                    setData(property, newPos.toString());
                }
            }

            if (!isDraggingClip) {
                // find the clip currently being clicked on
                // set state.draggingClip
                // and initialPos

                for (auto track : state.video->videoTracks) {
                    for (auto _clip : track->getClips()) {
                        auto clip = _clip.second;
                        Vector2D position = clip->getPos();
                        Vector2D size = clip->getSize();
                        // fmt::println("-------------------------");
                        // fmt::println("{}, {}", canvasX >= position.x, canvasX <= position.x + size.x);
                        // fmt::println("{}, {}", canvasY >= position.y, canvasY <= position.y + size.y);
                        // fmt::println("-------------------------");
                        // fmt::println("{}, {}", position.x, position.y);
                        // fmt::println("{}, {}", size.x, size.y);
                        // fmt::println("{}, {}", canvasX, canvasY);
                        // fmt::println("-------------------------");
                        if (canvasX >= position.x && canvasX <= position.x + size.x &&
                            canvasY >= position.y && canvasY <= position.y + size.y
                        ) {
                            fmt::println("found clip!");
                            selectedClip = clip;
                            initialPos = { canvasX, canvasY };
                            isDraggingClip = true;
                            break;
                        }
                    }

                    if (isDraggingClip) {
                        break;
                    }
                }
            }
        }
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    if (state.isClipSelected() && state.getSelectedClip()) {
        Vector2D position = selectedClip->getPos();
        Vector2D size = selectedClip->getSize();
        Vector2D resolution = state.video->getResolution();
        drawList->AddRect({
            imagePos.x + windowPos.x + std::clamp(position.x, 0, resolution.x) * scale,
            imagePos.y + windowPos.y + std::clamp(position.y, 0, resolution.y) * scale
        }, {
            imagePos.x + windowPos.x + std::clamp(position.x + size.x, 0, resolution.x) * scale,
            imagePos.y + windowPos.y + std::clamp(position.y + size.y, 0, resolution.y) * scale
        }, ImColor(255, 0, 0, 127), 0.f, 0, 5.f);
    }

    // media control bar
    float mediaControlW = imageSize.x;
    float mediaControlH = 20.f;

    float progress = std::max(
        (state.currentFrame * 1.f) / (state.video->frameCount * 1.f),
        1.f / 30.f
    );

    progress = std::clamp(progress, 0.f, 1.f);

    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - mediaControlW) / 2.f);
    ImVec2 mediaControlPos = ImGui::GetCursorScreenPos();

    float rounding = 30.f;

    // background
    drawList->AddRectFilled(
        mediaControlPos,
        ImVec2(mediaControlPos.x + mediaControlW, mediaControlPos.y + mediaControlH),
        ImGui::GetColorU32(ImGuiCol_FrameBg),
        rounding
    );
    
    // fill
    drawList->AddRectFilled(
        mediaControlPos,
        ImVec2(mediaControlPos.x + mediaControlW * progress, mediaControlPos.y + mediaControlH),
        IM_COL32(255, 100, 100, 255),
        rounding
    );

    float circleRad = mediaControlH / 2.f;

    // circle drag thingy idk
    drawList->AddCircleFilled(
        ImVec2(
            mediaControlPos.x + mediaControlW * progress - circleRad,
            mediaControlPos.y + mediaControlH - circleRad
        ),
        circleRad,
        IM_COL32(255, 150, 150, 255)
    );

    ImGui::SetCursorScreenPos(mediaControlPos);
    ImGui::InvisibleButton("##media-control", ImVec2(mediaControlW, mediaControlH));

    if (ImGui::IsItemActive()) {
        float mouseX = ImGui::GetIO().MousePos.x;
        float newProgress = std::clamp((mouseX - mediaControlPos.x) / mediaControlW, 0.0f, 1.0f);
        setCurrentFrame(state.video->frameCount * newProgress);   
    }

    // playback buttons

    ImVec2 buttonSize = ImVec2(
        ImGui::GetFontSize() * 2.f,
        ImGui::GetFontSize() * 2.f
    );

    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float windowWidth = ImGui::GetContentRegionAvail().x;

    float totalPlaybackBtnWidth = buttonSize.x * 3 + spacing * 2;
    // UR = undo/redo
    float totalURBtnWidth = buttonSize.x * 2 + spacing;

    float playbackOffset = (windowWidth - totalPlaybackBtnWidth) * 0.5f;
    auto btnStart = ImGui::GetCursorPosX();
    float playbackBtnStart = btnStart + playbackOffset;
    if (playbackOffset > 0.0f) {
        ImGui::SetCursorPosX(playbackBtnStart);
    }

    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 150));

    auto playbackBtn = [&](const char* label, std::function<void()> cb, bool disabled = false) {
        auto min = ImGui::GetCursorScreenPos();
        bool hovered = ImGui::IsMouseHoveringRect(min, ImVec2(min.x + buttonSize.x, min.y + buttonSize.y));
        if (disabled) {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 75));
        }
        if (hovered && !disabled) {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
        }
        
        if (ImGui::Button(
            label,
            buttonSize
        ) && !disabled) {
            cb();
        }

        if (hovered && !disabled) {
            ImGui::PopStyleColor();
        }

        if (disabled) {
            ImGui::PopStyleColor();
        }
    };

    playbackBtn(ICON_FA_BACKWARD, [&]() {
        setCurrentFrame(state.currentFrame - state.video->frameForTime(5));
    });
    
    ImGui::SameLine();

    playbackBtn(state.isPlaying ? ICON_FA_PAUSE : ICON_FA_PLAY, [&]() {
        togglePlay();
    });

    ImGui::SameLine();

    playbackBtn(ICON_FA_FORWARD, [&]() {
        setCurrentFrame(state.currentFrame + state.video->frameForTime(5));
    });

    ImGui::SameLine();

    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - mediaControlW) / 2.f + mediaControlW - totalURBtnWidth);
        
    playbackBtn(ICON_FA_ROTATE_LEFT, [&]() {
        state.undo();
    });

    ImGui::SameLine();

    playbackBtn(ICON_FA_ROTATE_RIGHT, [&]() {
        state.redo();
    }, state.redoStack.size() <= 0);


    ImGui::PopStyleColor(4);

    ImGui::SameLine();

    int duration = state.video->timeForFrame(state.video->frameCount);
    int currentTime = std::clamp(state.video->timeForFrame(state.currentFrame), 0.f, duration * 1.f);

    ImGui::PushFont(progressFont, 22.f);

    auto currentTimeText =
        duration >= 3600 ?
            fmt::format("{:%H:%M:%S}", std::chrono::seconds(currentTime)) :
            fmt::format("{:%M:%S}", std::chrono::seconds(currentTime));
    
    auto durationText = duration >= 3600 ?
            fmt::format("/ {:%H:%M:%S}", std::chrono::seconds(duration)) :
            fmt::format("/ {:%M:%S}", std::chrono::seconds(duration));
    
    auto currentTimeTextSize = ImGui::CalcTextSize(currentTimeText.c_str()).x;
    auto durationTextSize = ImGui::CalcTextSize(durationText.c_str()).x;

    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - mediaControlW) / 2.f);
    
    ImGui::Text("%s", currentTimeText.c_str());

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 150, 255));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(-25, 0));
 
    ImGui::Text("%s", durationText.c_str());

    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    ImGui::PopFont();

    ImGui::End();
}

void Application::setCurrentFrame(int frame) {
    auto& state = State::get();
    frame = std::clamp(frame, 0, state.video->frameCount);
    state.currentFrame = frame;
    // timeline.setPlayheadTime(state.video->timeForFrame(frame));
}

template<class T>
void Application::drawClipButton(std::string name, int defaultDuration) {
    if (ImGui::Button(name.c_str(), ImVec2(-1.0f, 0.0f))) {
        timeline.isPlacingClip = true;
        timeline.placeType = TrackType::Video;
        timeline.placeCb = [this, defaultDuration](int frame, int trackIdx) {
            auto& state = State::get();
            std::string uID = utils::generateUUID();

            Action action = {
                .perform = [&state, trackIdx, frame, defaultDuration](std::string info) {
                    auto clip = std::make_shared<T>();
                    clip->startFrame = frame;
                    clip->duration = defaultDuration;
                    clip->uID = info;
                    state.video->addClip(trackIdx, clip);
                    state.selectClip(clip);
                },
                .undo = [&state, trackIdx, uID](std::string) {
                    int trackIdx = state.video->getClipMap()[uID];
                    if (state.selectedClipId == uID) {
                        state.deselect();
                    }
                    state.video->getTracks()[trackIdx]->removeClip(uID);
                },
                .info = uID
            };

            action.perform(uID);

            state.undoStack.push(action);
            state.redoStack = std::stack<Action>();

            state.lastRenderedFrame = -1;
        };
    }
}

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

bool Application::initSDL() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fmt::println("SDL Init error: {}", SDL_GetError());
        return false;
    }

    SDL_SetAppMetadata("paperclip", "1.0", "com.underscored.paperclip");

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    window = SDL_CreateWindow("Paperclip", 1200, 800, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        fmt::println("no window?");
        return false;
    }

    
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    // SDL_GL_LoadLibrary(nullptr);
    gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        fmt::println("no GL Context");
        return false;
    }
    SDL_GL_MakeCurrent(window, gl_context);
    
    SDL_GL_SetSwapInterval(1); // Enable vsync
    
    int version = gladLoadGL((GLADloadfunc)&SDL_GL_GetProcAddress);
    if (version == 0) {
        fmt::println("could NOT initalize GLAD");
        return false;
    }
    fmt::println("GLAD loaded OpenGL {}.{}", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));
    fmt::println("GL Version: {}", (const char*)glGetString(GL_VERSION));
    fmt::println("GLSL Version: {}", (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));

    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    glViewport(0, 0, w, h);
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    SDL_GL_SwapWindow(window);

    if (!glGenTextures) fmt::println("glGenTextures is null!");
    if (!glBindTexture) fmt::println("glBindTexture is null!");

    // wireframe mode
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    fmt::println("SUCCESS!!!!!!!!!!!!!!!!!!!! i think");

    return true;
}

bool Application::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch

    if (!ImGui_ImplSDL3_InitForOpenGL(window, gl_context)) {
        return false;
    }

    if (!ImGui_ImplOpenGL3_Init()) {
        return false;
    }

    setupStyle();

    return true;
}

bool Application::initIcons() {
    // icons relative to resources/imgs
    std::unordered_map<IconType, std::string> paths = {
        { IconType::PlayPause, "play.png" }
    };

    int w = 0, h = 0, ch = 0;
    for (auto img : paths) {
        unsigned char* data = stbi_load(fmt::format("resources/imgs/{}", img.second).c_str(), &w, &h, &ch, 3);

        if (!data) {
            return false;
        }

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

        icons[img.first] = {
            .texture = texture,
            .w = w,
            .h = h
        };

        stbi_image_free(data);
    }

    return true;
}

void Application::setup() {
    if (!initSDL()) {
        fmt::println("could not initialize SDL");
        return;
    }

    if (!initImGui()) {
        fmt::println("could not initalize ImGui");
        return;
    }

    if (!initIcons()) {
        fmt::println("could not initialize icons");
        return;
    }
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

void Application::setupStyle() {
    // Future Dark style by rewrking from ImThemes
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha = 1.0f;
    style.DisabledAlpha = 1.0f;
    style.WindowPadding = ImVec2(12.0f, 12.0f);
    style.WindowRounding = 0.0f;
    style.WindowBorderSize = 0.0f;
    style.WindowMinSize = ImVec2(20.0f, 20.0f);
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_None;
    style.ChildRounding = 0.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupRounding = 0.0f;
    style.PopupBorderSize = 1.0f;
    style.FramePadding = ImVec2(6.0f, 6.0f);
    style.FrameRounding = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.ItemSpacing = ImVec2(12.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 3.0f);
    style.CellPadding = ImVec2(12.0f, 6.0f);
    style.IndentSpacing = 20.0f;
    style.ColumnsMinSpacing = 6.0f;
    style.ScrollbarSize = 12.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabMinSize = 12.0f;
    style.GrabRounding = 0.0f;
    style.TabRounding = 0.0f;
    style.TabBorderSize = 0.0f;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

    style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.2745098173618317f, 0.3176470696926117f, 0.4509803950786591f, 1.0f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.2352941185235977f, 0.2156862765550613f, 0.5960784554481506f, 1.0f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.5372549295425415f, 0.5529412031173706f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.2352941185235977f, 0.2156862765550613f, 0.5960784554481506f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.2352941185235977f, 0.2156862765550613f, 0.5960784554481506f, 1.0f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.2352941185235977f, 0.2156862765550613f, 0.5960784554481506f, 1.0f);
    style.Colors[ImGuiCol_Tab] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.5215686559677124f, 0.6000000238418579f, 0.7019608020782471f, 1.0f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.03921568766236305f, 0.9803921580314636f, 0.9803921580314636f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(1.0f, 0.2901960909366608f, 0.5960784554481506f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.9960784316062927f, 0.4745098054409027f, 0.6980392336845398f, 1.0f);
    style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.2352941185235977f, 0.2156862765550613f, 0.5960784554481506f, 1.0f);
    style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 0.501960813999176f);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 0.501960813999176f);

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("resources/opensans.ttf");

    float baseFontSize = 13.0f; // 13.0f is the size of the default font. Change to the font size you use.
    float iconFontSize = baseFontSize * 2.0f / 3.0f; // FontAwesome fonts need to have their sizes reduced by 2.0f/3.0f in order to align correctly

    // merge in icons from Font Awesome
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
    ImFontConfig icons_config; 
    icons_config.MergeMode = true; 
    icons_config.PixelSnapH = true; 
    icons_config.GlyphMinAdvanceX = iconFontSize;
    io.Fonts->AddFontFromFileTTF(fmt::format("resources/icons/{}", FONT_ICON_FILE_NAME_FAS).c_str(), iconFontSize, &icons_config, icons_ranges);
    progressFont = io.Fonts->AddFontFromFileTTF("resources/boldonse.ttf");
}
