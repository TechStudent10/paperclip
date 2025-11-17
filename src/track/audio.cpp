#include <filesystem>
#include <track/audio.hpp>

#include <state.hpp>

#include <clips/properties/number.hpp>

AudioClip::AudioClip(const std::string& path): Clip(0, 7680), path(path) {
    m_metadata.name = std::filesystem::path(path).filename();
    // volume
    // default = 100
    addProperty(
        std::make_shared<NumberProperty>()
    );

    // start time
    // default = 0
    addProperty(
        std::make_shared<NumberProperty>()
    );

    if (!path.empty()) {
        initalize();
    }
}

AudioClip::AudioClip(): AudioClip("") {}

AudioClip::~AudioClip() {
    ma_sound_uninit(&sound);
}

bool AudioClip::initalize() {
    if (initialized) {
        return true;
    }

    auto& state = State::get();
    
    if (ma_sound_init_from_file(&state.soundEngine, path.c_str(), 0, NULL, NULL, &sound) != MA_SUCCESS) {
        fmt::println("could not init file");
        return false;
    }

    ma_decoder decoder;
    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 1, 48000);

    auto res = ma_decoder_init_file(path.c_str(), &config, &decoder);
    if (res != MA_SUCCESS) {
        fmt::println("could not initialize decoder: {}", (int)res);
        return false;
    }
    int framesPerChunk = decoder.outputSampleRate * 0.001f; // every 1ms
    float buffer[framesPerChunk];
    ma_uint64 framesRead;

    // do NOT switch this to a regular while loop
    // it breaks the res != MA_SUCCESS check up there
    // because of undefined behavior
    // i love c++
    do {
        ma_decoder_read_pcm_frames(&decoder, buffer, framesPerChunk, &framesRead);

        float sumSq = 0.f;
        for (ma_uint32 i = 0; i < framesRead; i++) {
            float s = buffer[i];
            sumSq += s * s;
        }

        float rms = sqrt(sumSq / framesRead);
        waveform.push_back(rms);
    } while (framesRead > 0);

    fmt::println("decoded audio waveform!");

    ma_decoder_uninit(&decoder);

    initialized = true;

    return true;
}

void AudioClip::play() {
    initalize();
    if (this->playing) return;
    int volume = getProperty<NumberProperty>("volume").unwrap()->data;
    ma_sound_set_volume(&sound, (float)volume / 100.f);
    if (ma_sound_start(&sound) != MA_SUCCESS) {
        fmt::println("no success :(");
    }
    this->playing = true;
}

void AudioClip::stop() {
    initalize();
    if (!this->playing) return;
    ma_sound_stop(&sound);
    this->playing = false;
}

void AudioClip::seekToSec(float seconds) {
    initalize();

    auto& state = State::get();
    int startTime = getProperty<NumberProperty>("start-time").unwrap()->data;
    ma_sound_seek_to_second(&sound, seconds + startTime);
}

float AudioClip::getCursor() {
    initalize();
    float cursor = 0;
    ma_sound_get_cursor_in_seconds(&sound, &cursor);
    int startTime = getProperty<NumberProperty>("start-time").unwrap()->data;
    return cursor - startTime;
}

void AudioClip::setVolume(float volume) {
    initalize();
    ma_sound_set_volume(&sound, volume);
}

void AudioTrack::processTime() {
    auto& state = State::get();
    auto currentFrame = state.currentFrame;
    for (auto _clip : clips) {
        auto clip = _clip.second;
        if (currentFrame >= clip->startFrame && currentFrame < clip->startFrame + clip->duration && state.isPlaying) {
            float seconds = state.video->timeForFrame(currentFrame - clip->startFrame);
            // the threshold is how far off the playback audio can be before we re-align it
            float threshold = 0.05f;
            if (std::abs(clip->getCursor() - seconds) >= threshold) {
                clip->seekToSec(seconds);
            }
            clip->play();
        }
        if (clip->startFrame + clip->duration < currentFrame && state.isPlaying) {
            clip->stop();
        }
        for (auto [id, property] : clip->m_properties) {
            property->processKeyframe(currentFrame);
        }

        int volume = clip->getProperty<NumberProperty>("volume").unwrap()->data;
        clip->setVolume(volume / 100.f);
    }
}

void AudioTrack::onPlay() {
    auto& state = State::get();
    // filters clips that should be playing now
    for (auto _clip : clips) {
        auto clip = _clip.second;
        if (state.currentFrame >= clip->startFrame && state.currentFrame <= clip->startFrame + clip->duration) {
            clip->seekToSec(state.video->timeForFrame(state.currentFrame - clip->startFrame));
        }
    }
}

void AudioTrack::onStop() {
    for (auto clip : clips) {
        clip.second->stop();
    }
}
