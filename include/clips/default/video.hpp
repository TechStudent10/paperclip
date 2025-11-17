#pragma once

#include "../clip.hpp"
#include <common.hpp>
#include <condition_variable>
#include <framework/mlt.h>

#include <thread>
#include <vector>
#include <string>

#include <frame.hpp>
#include <utils.hpp>

namespace clips {
    class VideoClip : public Clip {
    protected:
        bool decodeFrame(int frameNumber);
        std::array<uint8_t*, 3> decodeFrameRaw(int frameNumber);

        bool initialize();

        mlt_profile profile;
        mlt_producer producer;

        int width = 0, height = 0, fps = 0;
        std::string path;
        bool initialized = false;
        bool hasUploaded = false;

        GLuint textureY, textureU, textureV;
        GLuint VAO;
        GLuint VBO;
        GLuint EBO;

        std::thread previewGenThread;
        std::mutex framesMutex;
        std::mutex producerMutex;
        std::condition_variable previewCv;
        std::atomic<bool> stopGen = false;
        
        std::vector<int> pendingFrames;
        std::unordered_map<int, std::array<uint8_t*, 3>> finishedFrames;
        std::unordered_map<int, std::shared_ptr<Frame>> previewFrames;
    public:
        VideoClip(const std::string& path);
        VideoClip();
        ~VideoClip();

        ClipType getType() override { return ClipType::Video; }

        void render(Frame* frame) override;
        
        void write(qn::HeapByteWriter& writer) override {
            Clip::write(writer);
            UNWRAP_WITH_ERR(writer.writeStringU32(path));
        }

        void read(qn::ByteReader& reader) override {
            Clip::read(reader);
            path = reader.readStringU32().unwrapOr("");
        }

        Vector2D getSize() override;
        Vector2D getPos() override;

        GLuint getPreviewTexture(int frame) override;
        Vector2D getPreviewSize() override;
    };
} // namespace clips
