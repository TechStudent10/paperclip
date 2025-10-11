#pragma once

#include <clips/clip.hpp>
#include <string>

#include <miniaudio.h>

class AudioClip : public Clip {
private:
    ma_sound sound;
    std::string path;
    bool initialized = false;

    bool initalize();
public:
    bool playing = false;
    AudioClip(const std::string& path);
    AudioClip();
    ~AudioClip();

    void play();
    void seekToSec(float seconds);
    void setVolume(float volume);
    void stop();

    float getCursor();
    
    void write(qn::HeapByteWriter& writer) override {
        Clip::write(writer);
        writer.writeStringU32(path);
    }

    void read(qn::ByteReader& reader) override {
        Clip::read(reader);
        path = reader.readStringU32().unwrapOr("");
    }
};

class AudioTrack {
private:
    std::vector<std::shared_ptr<AudioClip>> clips;
public:
    AudioTrack() {
        clips = {};
    }

    void addClip(std::shared_ptr<AudioClip> clip) {
        clips.push_back(clip);
    }

    void removeClip(std::shared_ptr<AudioClip> clip) {
        clips.erase(std::remove(clips.begin(), clips.end(), clip), clips.end());
    }

    std::vector<std::shared_ptr<AudioClip>> getClips() {
        return clips;
    }

    void processTime();
    void onPlay();
    void onStop();

    void write(qn::HeapByteWriter& writer) {
        writer.writeI16(clips.size());
        for (auto clip : clips) {
            clip->write(writer);
        }
    }

    void read(qn::ByteReader& reader) {
        auto size = reader.readI16().unwrapOr(0);
        for (int i = 0; i < size; i++) {
            auto clip = std::make_shared<AudioClip>();
            clip->read(reader);
            clips.push_back(clip);
        }
    }
};
