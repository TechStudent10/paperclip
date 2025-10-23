#pragma once
#include <memory>
#include <map>
#include <unordered_map>
#include <vector>
#include <string>

#include <binary/reader.hpp>
#include <binary/writer.hpp>

#include <common.hpp>
#include <track/audio.hpp>
#include <track/video.hpp>

#include <renderer/video.hpp>

struct ExtClipMetadata {
    std::string filePath;
    int frameCount;

    void write(qn::HeapByteWriter& writer) {
        writer.writeStringU32(filePath);
        writer.writeI16(frameCount);
    }

    void read(qn::ByteReader& reader) {
        filePath = reader.readStringU32().unwrapOr("");
        frameCount = reader.readI16().unwrapOr(0);
    }
};

class Video {
protected:
    // map of all clip IDs to their respective track indexes
    // positive idx = video track
    // negative idx = audio track
    std::unordered_map<std::string, int> clipMap;
public:
    int framerate;

    Vector2D resolution;
    
    std::vector<std::shared_ptr<VideoTrack>> videoTracks;
    std::vector<std::shared_ptr<AudioTrack>> audioTracks;

    std::vector<ExtClipMetadata> clipPool;
    std::vector<ExtClipMetadata> imagePool;
    std::vector<ExtClipMetadata> audioClipPool;

    Video(int framerate, Vector2D resolution): framerate(framerate), resolution(resolution) {}
    Video(): Video(30, {1920, 1080}) {}

    int frameCount = 0;
    void recalculateFrameCount();

    int addTrack(std::shared_ptr<VideoTrack> track) {
        videoTracks.push_back(track);
        return videoTracks.size() - 1;
    }

    void addClip(int trackIdx, std::shared_ptr<Clip> clip);
    void addAudioClip(int trackIdx, std::shared_ptr<AudioClip> clip);

    void removeClip(int trackIdx, std::shared_ptr<Clip> clip);
    void removeAudioClip(int trackIdx, std::shared_ptr<AudioClip> clip);

    const std::vector<std::shared_ptr<VideoTrack>>& getTracks() const { return videoTracks; }
    std::unordered_map<std::string, int> getClipMap() { return clipMap; }

    void render(VideoRenderer* renderer);
    Frame* renderAtFrame(int frame);
    void renderIntoFrame(int frameNum, std::shared_ptr<Frame> frame);

    int getFPS() { return framerate; }
    Vector2D getResolution() { return resolution; }

    int frameForTime(float time);
    float timeForFrame(int time);

    void write(qn::HeapByteWriter& writer) {
        writer.writeI16(framerate);
        resolution.write(writer);
        
        writer.writeI16(videoTracks.size());
        for (auto track : videoTracks) {
            track->write(writer);
        }

        writer.writeI16(audioTracks.size());
        for (auto audTrack : audioTracks) {
            audTrack->write(writer);
        }

        writer.writeI16(clipPool.size());
        for (auto clip : clipPool) {
            clip.write(writer);
        }

        writer.writeI16(audioClipPool.size());
        for (auto audClip : audioClipPool) {
            audClip.write(writer);
        }

        writer.writeI16(imagePool.size());
        for (auto imgClip : imagePool) {
            imgClip.write(writer);
        }
    }

    void read(qn::ByteReader& reader) {
        framerate = reader.readI16().unwrapOr(0);
        resolution.read(reader);

        auto vidSize = reader.readI16().unwrapOr(0);
        for (int i = 0; i < vidSize; i++) {
            auto track = std::make_shared<VideoTrack>();
            track->read(reader);
            videoTracks.push_back(track);
        }

        auto audSize = reader.readI16().unwrapOr(0);
        for (int i = 0; i < audSize; i++) {
            auto track = std::make_shared<AudioTrack>();
            track->read(reader);
            audioTracks.push_back(track);
        }

        auto poolSize = reader.readI16().unwrapOr(0);
        for (int i = 0; i < poolSize; i++) {
            ExtClipMetadata meta;
            meta.read(reader);
            clipPool.push_back(meta);
        }

        auto audPoolSize = reader.readI16().unwrapOr(0);
        for (int i = 0; i < audPoolSize; i++) {
            ExtClipMetadata meta;
            meta.read(reader);
            audioClipPool.push_back(meta);
        }

        auto imgPoolSize = reader.readI16().unwrapOr(0);
        for (int i = 0; i < imgPoolSize; i++) {
            ExtClipMetadata meta;
            meta.read(reader);
            imagePool.push_back(meta);
        }
    }
};
