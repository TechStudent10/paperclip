#pragma once

#include <unordered_map>
#include <memory>

#include <clips/all.hpp>

class VideoTrack {
private:
    std::unordered_map<std::string, std::shared_ptr<Clip>> clips;
public:
    VideoTrack() {
        clips = {};
    }

    void addClip(std::shared_ptr<Clip> clip) {
        clips[clip->uID] = (clip);
    }

    void removeClip(std::shared_ptr<Clip> clip) {
        std::erase_if(clips, [clip](const auto& _clip) {
            return _clip.second == clip;
        });
    }

    void removeClip(std::string clipID) {
        std::erase_if(clips, [clipID](const auto& _clip) {
            return _clip.second->uID == clipID;
        });
    }

    std::unordered_map<std::string, std::shared_ptr<Clip>> getClips() {
        return clips;
    }

    void render(Frame* frame, int currentFrame);

    void write(qn::HeapByteWriter& writer) {
        writer.writeI16(clips.size());
        for (auto [idx, clip] : clips) {
            clip->write(writer);
        }
    }

    void read(qn::ByteReader& reader) {
        auto size = reader.readI16().unwrapOr(0);
        for (int i = 0; i < size; i++) {
            auto clipType = (ClipType)(reader.readI16().unwrapOr(0));
            fmt::println("love me some {}", (int)clipType);
            std::shared_ptr<Clip> clip;

            switch (clipType) {
                case ClipType::Rectangle:
                    fmt::println("making rect");
                    clip = std::make_shared<clips::Rectangle>();
                    break;
                case ClipType::Circle:
                    fmt::println("making circ");
                    clip = std::make_shared<clips::Circle>();
                    break;
                case ClipType::Text:
                    fmt::println("making text");
                    clip = std::make_shared<clips::Text>();
                    break;
                case ClipType::Image:
                    fmt::println("making imag");
                    clip = std::make_shared<clips::ImageClip>();
                    break;
                case ClipType::Video:
                    fmt::println("making vide");
                    clip = std::make_shared<clips::VideoClip>();
                    break;
                default:
                    fmt::println("making god knows what");
                    clip = std::make_shared<Clip>();
                    break;
            }

            clip->read(reader);
            fmt::println("{}", clip->m_metadata.name);
            addClip(clip);
        }
    }
};
