#include <video.hpp>
#include <fmt/base.h>
#include <fmt/format.h>

#define SAMPLE_RATE 48000
#define CHANNELS 2
#define FRAME_COUNT 4096

AudioRenderer::AudioRenderer(std::string_view outputPath, float length): outputPath(outputPath), length(length) {
    ma_encoder_config config = ma_encoder_config_init(ma_encoding_format_wav, ma_format_f32, CHANNELS, SAMPLE_RATE);
    if (ma_encoder_init_file(this->outputPath.c_str(), &config, &encoder) != MA_SUCCESS) {
        fmt::println("could not init encoder");
        return;
    }

    decoderConfig = ma_decoder_config_init(ma_format_f32, CHANNELS, SAMPLE_RATE);
}

void AudioRenderer::addClip(std::string path, float start, float end, std::shared_ptr<ClipProperty> volume) {
    AudioRenderFile file;
    ma_decoder_init_file(path.c_str(), &decoderConfig, &file.decoder);
    file.path = path;
    file.startTime = start;
    file.endTime = end;
    file.keyframes = volume->keyframes;
    file.keyframeInfo = volume->keyframeInfo;
    clips.push_back(file);
}

void AudioRenderer::render(float fps) {
    float outBuffer[FRAME_COUNT * CHANNELS];
    double currentTime = 0.0;
    double step = (double)FRAME_COUNT / SAMPLE_RATE;

    while (currentTime < length) {
        // fmt::println("current time: {}", currentTime);
        for (int i = 0; i < FRAME_COUNT * CHANNELS; i++) outBuffer[i] = 0.0f;

        for (auto clip : clips) {
            if (currentTime + step < clip.startTime) continue;
            if (currentTime >= clip.endTime) continue;
            
            float temp[FRAME_COUNT * CHANNELS];
            ma_uint64 framesRead;
            ma_decoder_read_pcm_frames(&clip.decoder, temp, FRAME_COUNT, &framesRead);
            
            double localTime = currentTime - clip.startTime;
            for (ma_uint64 i = 0; i < framesRead * CHANNELS; i++) {
                double sampleTime = localTime + (i / (double)CHANNELS / SAMPLE_RATE);
                int currentFrame = sampleTime * fps;

                // only one keyframe? use that
                if (clip.keyframes.size() == 1) {
                    // fmt::println("a {}", clip.keyframes[0]);
                    outBuffer[i] += temp[i] * ((float)Vector1D::fromString(clip.keyframes[0]).number / 100.f);
                    continue;
                }

                // beyond the last keyframe? use that
                if (std::prev(clip.keyframes.end())->first <= currentFrame - (clip.startTime / fps)) {
                    // fmt::println("b {}", clip.keyframes.rbegin()->second);
                    outBuffer[i] += temp[i] * ((float)Vector1D::fromString(clip.keyframes.rbegin()->second).number / 100.f);
                    continue;
                }

                int previousKeyframe = 0;
                int nextKeyframe = 0;

                for (auto keyframe : clip.keyframes) {
                    if (keyframe.first > currentFrame) {
                        nextKeyframe = keyframe.first;
                        break;
                    }

                    if (keyframe.first > previousKeyframe) {
                        previousKeyframe = keyframe.first;
                    }
                }

                if (nextKeyframe == 0) {
                    // somehow we did not catch the last keyframe, so we just set it here
                    // fmt::println("c {}", clip.keyframes[previousKeyframe]);
                    outBuffer[i] += temp[i] * ((float)Vector1D::fromString(clip.keyframes[previousKeyframe]).number / 100.f);
                    continue;
                }

                float progress = (float)(currentFrame - previousKeyframe) / (float)(nextKeyframe - previousKeyframe);
                if (clip.keyframeInfo.contains(nextKeyframe)) {
                    progress = animation::getEasingFunction(clip.keyframeInfo[nextKeyframe].easing, clip.keyframeInfo[nextKeyframe].mode)(progress);
                }

                auto oldNumber = Vector1D::fromString(clip.keyframes[previousKeyframe]);
                auto nextNumber = Vector1D::fromString(clip.keyframes[nextKeyframe]);

                float vol = std ::rint(oldNumber.number +
                                (float)(nextNumber.number - oldNumber.number) * progress) / 100.f;
                // fmt::println("d {}", vol);
                outBuffer[i] += temp[i] * vol;
                // fmt::println("{}", outBuffer[i]);
            }
        }

        for (int i = 0; i < FRAME_COUNT * CHANNELS; i++) {
            // clamp
            outBuffer[i] = std::min(std::max(outBuffer[i], -1.0f), 1.0f);
        }

        ma_encoder_write_pcm_frames(&encoder, outBuffer, FRAME_COUNT, NULL);
        currentTime += step;
    }

    for (auto clip : clips) ma_decoder_uninit(&clip.decoder);
    ma_encoder_uninit(&encoder);
}
