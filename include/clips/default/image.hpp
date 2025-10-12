#pragma once

#include "../clip.hpp"
#include <common.hpp>

#include <vector>
#include <string>
#include <frame.hpp>

namespace clips {
    class ImageClip : public Clip {
    private:
        GLuint texture;
        unsigned char* imageData;

        int width, height;
        int scaledW = 0, scaledH = 0;

        bool initialize();
    public:
        bool initialized = false;
        std::string path;
        ImageClip();
        ImageClip(const std::string& path);
        ~ImageClip();

        ImageClip& operator=( const ImageClip& ) = delete;

        void render(Frame* frame) override;
        void onDelete() override;

        ClipType getType() override { return ClipType::Image; }

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
