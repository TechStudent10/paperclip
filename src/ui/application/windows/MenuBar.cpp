#include <Application.hpp>
#include <state.hpp>
#include <filesystem>
#include <renderer/audio.hpp>

#include <fstream>
#include <nfd.h>

#include <misc/cpp/imgui_stdlib.h>

void Application::drawMenuBar() {
    auto& state = State::get();

    ImGuiID exportId = ImGui::GetID("Export Menu");

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save As")) {
#ifdef WIN32
                nfdnchar_t *outPath = NULL;
#else
                nfdchar_t *outPath = NULL;
#endif
                
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
#ifdef WIN32
                nfdnchar_t *outPath = NULL;
#else
                nfdchar_t *outPath = NULL;
#endif
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
#ifdef WIN32
                nfdnchar_t *outPath = NULL;
#else
                nfdchar_t *outPath = NULL;
#endif
            nfdresult_t result = NFD_SaveDialogN(
                &outPath,
                VIDEO_FILES_FILTER,
                std::size(VIDEO_FILES_FILTER),
                nullptr,
                nullptr
            );

            if (result == NFD_OKAY) {
                state.exportPath = std::move(ensureCStr(outPath));
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
                        clip->getPath(),
                        state.video->timeForFrame(clip->startFrame),
                        state.video->timeForFrame(clip->startFrame + clip->duration),
                        clip->getProperty<NumberProperty>("volume").unwrap()
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
}
