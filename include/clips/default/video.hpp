#pragma once

#include "../clip.hpp"
#include <common.hpp>
#include <framework/mlt.h>

#include <vector>
#include <string>

#include <frame.hpp>

namespace clips {
    class VideoClip : public Clip {
    private:
        bool decodeFrame(int frameNumber);

        bool initialize();

        mlt_profile profile;
        mlt_producer producer;

        int width = 0, height = 0, fps = 0;
        std::vector<unsigned char> vidFrame;
        std::string path;
        bool initialized = false;
        bool hasUploaded = false;

        GLuint textureY, textureU, textureV;
    public:
        VideoClip(const std::string& path);
        VideoClip();
        ~VideoClip();

        ClipType getType() override { return ClipType::Video; }

        void render(Frame* frame) override;
        
        void write(qn::HeapByteWriter& writer) override {
            Clip::write(writer);
            writer.writeStringU32(path);
        }

        void read(qn::ByteReader& reader) override {
            Clip::read(reader);
            path = reader.readStringU32().unwrapOr("");
        }

        Vector2D getSize() override;
        Vector2D getPos() override;
    };
} // namespace clips
