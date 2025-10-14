#include <track/audio.hpp>

#include <state.hpp>

AudioClip::AudioClip(const std::string& path): Clip(0, 7680), path(path) {
    m_metadata.name = path;
    m_properties.addProperty(
        ClipProperty::number()
            ->setId("volume")
            ->setName("Volume")
            ->setDefaultKeyframe(Vector1D{ .number = 100 }.toString())
    );

    m_properties.addProperty(
        ClipProperty::number()
            ->setId("start-time")
            ->setName("Start Time")
            ->setDefaultKeyframe(Vector1D{ .number = 0 }.toString())
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

    initialized = true;

    return true;
}

void AudioClip::play() {
    initalize();
    if (this->playing) return;
    auto volume = Vector1D::fromString(m_properties.getProperty("volume")->data).number;
    ma_sound_set_volume(&sound, (float)volume / 255);
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
    int startTime = Vector1D::fromString(m_properties.getProperty("start-time")->data).number;
    ma_sound_seek_to_second(&sound, seconds + startTime);
}

float AudioClip::getCursor() {
    initalize();
    float cursor = 0;
    ma_sound_get_cursor_in_seconds(&sound, &cursor);
    int startTime = Vector1D::fromString(m_properties.getProperty("start-time")->data).number;
    return cursor - startTime;
}

void AudioClip::setVolume(float volume) {
    initalize();
    ma_sound_set_volume(&sound, volume);
}

void AudioTrack::processTime() {
    auto& state = State::get();
    auto currentFrame = state.currentFrame;
    for (auto clip : clips) {
        if (currentFrame >= clip->startFrame && currentFrame < clip->startFrame + clip->duration && state.isPlaying) {
            float seconds = state.video->timeForFrame(currentFrame - clip->startFrame);
            if (std::abs(clip->getCursor() - seconds) >= 0.25f) {
                clip->seekToSec(seconds);
            }
            clip->play();
        }
        if (clip->startFrame + clip->duration < currentFrame && state.isPlaying) {
            clip->stop();
        }
        for (auto property : clip->m_properties.getProperties()) {
            // only one keyframe? use that
            if (property.second->keyframes.size() == 1) {
                property.second->data = property.second->keyframes[0];
                continue;
            }

            // beyond the last keyframe? use that
            if (std::prev(property.second->keyframes.end())->first <= currentFrame - clip->startFrame) {
                property.second->data = property.second->keyframes.rbegin()->second;
                continue;
            }

            int previousKeyframe = 0;
            int nextKeyframe = 0;

            for (auto keyframe : property.second->keyframes) {
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
                property.second->data = property.second->keyframes[previousKeyframe];
                continue;
            }

            float progress = (float)(currentFrame - previousKeyframe) / (float)(nextKeyframe - previousKeyframe);
            if (property.second->keyframeInfo.contains(nextKeyframe)) {
                progress = animation::getEasingFunction(property.second->keyframeInfo[nextKeyframe].easing, property.second->keyframeInfo[nextKeyframe].mode)(progress);
            }

            auto oldNumber = Vector1D::fromString(property.second->keyframes[previousKeyframe]);
            auto nextNumber = Vector1D::fromString(property.second->keyframes[nextKeyframe]);

#define ANIMATE_NUM(old, new, arg) .arg = static_cast<int>(std::rint(old.arg + (float)(new.arg - old.arg) * progress))

            property.second->data = Vector1D{
                ANIMATE_NUM(oldNumber, nextNumber, number),
            }.toString();

#undef ANIMATE_NUM
        }

        clip->setVolume(Vector1D::fromString(clip->m_properties.getProperty("volume")->data).number / 100.f);
    }
}

void AudioTrack::onPlay() {
    auto& state = State::get();
    // filters clips that should be playing now
    for (auto clip : clips) {
        if (state.currentFrame >= clip->startFrame && state.currentFrame <= clip->startFrame + clip->duration) {
            clip->seekToSec(state.video->timeForFrame(state.currentFrame - clip->startFrame));
        }
    }
}

void AudioTrack::onStop() {
    for (auto clip : clips) {
        clip->stop();
    }
}
