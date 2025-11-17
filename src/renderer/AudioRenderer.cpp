#include <fmt/base.h>
#include <fmt/format.h>

#include <renderer/audio.hpp>

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

void AudioRenderer::addClip(std::string path, float start, float end, std::shared_ptr<NumberProperty> volume) {
    AudioRenderFile file;
    ma_decoder_init_file(path.c_str(), &decoderConfig, &file.decoder);
    file.path = path;
    file.startTime = start;
    file.endTime = end;
    file.volume = volume;
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

                clip.volume->processKeyframe(currentFrame);

                // fmt::println("d {}", vol);
                outBuffer[i] += temp[i] * clip.volume->data;
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
