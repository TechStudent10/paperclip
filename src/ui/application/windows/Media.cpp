#include <Application.hpp>
#include <state.hpp>

#include <action/actions/CreateClip.hpp>
#include <action/actions/CreateVideoClip.hpp>
#include <clips/properties/number.hpp>

void Application::drawMediaWindow() {
    auto& state = State::get();

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
#ifdef WIN32
                nfdnchar_t *outPath = NULL;
#else
                nfdchar_t *outPath = NULL;
#endif
                nfdresult_t result = NFD_OpenDialogN(
                    &outPath,
                    VIDEO_FILES_FILTER,
                    std::size(VIDEO_FILES_FILTER),
                    nullptr
                );

                if (result == NFD_OKAY) {
                    std::string outFile = std::string(ensureCStr(std::move(outPath)));
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
                        // state.video->audioClipPool.push_back({
                        //     .filePath = fmt::format("{}.mp3", outFile),
                        //     .frameCount = frameCount
                        // });
                    });
                    convertThread.detach();
                }
                else if (result == NFD_CANCEL) {}
            }

            if (ImGui::Button("Import Audio")) {
#ifdef WIN32
                nfdnchar_t *outPath = NULL;
#else
                nfdchar_t *outPath = NULL;
#endif
                nfdresult_t result = NFD_OpenDialogN(
                    &outPath,
                    AUDIO_FILES_FILTER,
                    std::size(AUDIO_FILES_FILTER),
                    nullptr
                );

                if (result == NFD_OKAY) {
                    std::string outFile = std::move(ensureCStr(outPath));
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
#ifdef WIN32
                nfdnchar_t *outPath = NULL;
#else
                nfdchar_t *outPath = NULL;
#endif
                nfdresult_t result = NFD_OpenDialogN(
                    &outPath,
                    IMAGE_FILES_FILTER,
                    std::size(IMAGE_FILES_FILTER),
                    nullptr
                );

                if (result == NFD_OKAY) {
                    std::string outFile = std::move(ensureCStr(outPath));
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
                    timeline.placeDuration = clipMeta.frameCount;
                    timeline.placeCb = [this, clipMeta](int frame, int trackIdx) {
                        // it is guaranteed that the video clip can be placed
                        // but not the audio clip
                        // so check that before adding both
                        if (timeline.willClipCollide(frame, clipMeta.frameCount, trackIdx, TrackType::Audio)) return;

                        auto& state = State::get();
                        auto action = std::make_shared<CreateVideoClip>(clipMeta, frame, trackIdx);
                        state.addAction(action);
                        action->perform();
                    };
                }
            }

            ImGui::SeparatorText("Audio");

            for (auto soundFile : state.video->audioClipPool) {
                if (ImGui::Button(soundFile.filePath.c_str(), ImVec2(-1.0f, 0.0f))) {
                    timeline.isPlacingClip = true;
                    timeline.placeType = TrackType::Audio;
                    timeline.placeDuration = soundFile.frameCount;
                    timeline.placeCb = [this, soundFile](int frame, int trackIdx) {
                        auto& state = State::get();
                        auto clip = std::make_shared<AudioClip>(soundFile.filePath);
                        clip->startFrame = frame;
                        clip->duration = soundFile.frameCount;
                        clip->getProperty<NumberProperty>("volume").unwrap()->data = 100;
                        
                        auto action = std::make_shared<CreateClip>(clip, clip->getType(), trackIdx);
                        action->perform();
                        state.addAction(action);                        
                        state.lastRenderedFrame = -1;
                    };
                }
            }

            ImGui::SeparatorText("Images");

            for (auto imageFile : state.video->imagePool) {
                if (ImGui::Button(imageFile.filePath.c_str(), ImVec2(-1.0f, 0.0f))) {
                    timeline.isPlacingClip = true;
                    timeline.placeType = TrackType::Video;
                    timeline.placeDuration = imageFile.frameCount;
                    timeline.placeCb = [this, imageFile](int frame, int trackIdx) {
                        auto& state = State::get();
                        auto clip = std::make_shared<clips::ImageClip>(imageFile.filePath);
                        clip->startFrame = frame;
                        clip->duration = imageFile.frameCount;
                        auto action = std::make_shared<CreateClip>(clip, clip->getType(), trackIdx);
                        action->perform();
                        state.addAction(action);
                        state.lastRenderedFrame = -1;
                    };
                }
            }

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}
