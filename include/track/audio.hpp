#pragma once

#include <clips/clip.hpp>
#include <string>

#include <miniaudio.h>
#include <utils.hpp>

class AudioClip : public Clip {
private:
    ma_sound sound;
    std::string path;
    bool initialized = false;

    bool initalize();

    friend class AudioTrack;
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
        UNWRAP_WITH_ERR(writer.writeStringU32(path));
    }

    void read(qn::ByteReader& reader) override {
        UNWRAP_WITH_ERR(reader.readStringU32());
        UNWRAP_WITH_ERR(reader.readI16());
        Clip::read(reader);
        UNWRAP_WITH_ERR(reader.readStringVar());
        path = reader.readStringU32().unwrapOr("");
    }

    ClipType getType() override { return ClipType::Audio; }
};

class AudioTrack {
private:
    std::unordered_map<std::string, std::shared_ptr<AudioClip>> clips;
public:
    AudioTrack() {
        clips = {};
    }

    void addClip(std::shared_ptr<AudioClip> clip) {
        clips[clip->uID] = (clip);
    }

    std::shared_ptr<AudioClip> getClip(std::string id) {
        if (!clips.contains(id)) return nullptr;

        return clips[id];
    }

    void removeClip(std::shared_ptr<AudioClip> clip) {
        std::erase_if(clips, [clip](const auto& _clip) {
            return _clip.second == clip;
        });
    }

    void removeClip(std::string clipID) {
        if (!clips.at(clipID).get()) {
            fmt::println("invalid pointer!");
            return;
        }
        // std::erase_if(clips, [clipID](const auto& _clip) {
        //     return _clip.second->uID == clipID;
        // });
        clips.erase(clipID);
    }

    std::unordered_map<std::string, std::shared_ptr<AudioClip>> getClips() {
        return clips;
    }

    void processTime();
    void onPlay();
    void onStop();

    void write(qn::HeapByteWriter& writer) {
        writer.writeI16(clips.size());
        for (auto _clip : clips) {
            auto clip = _clip.second;
            clip->write(writer);
        }
    }

    void read(qn::ByteReader& reader) {
        auto size = reader.readI16().unwrapOr(0);
        for (int i = 0; i < size; i++) {
            auto clip = std::make_shared<AudioClip>();
            clip->read(reader);
            addClip(clip);
        }
    }
};
