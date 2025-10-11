#include <clips/default/video.hpp>

#include <state.hpp>
#include <utils.hpp>

#include <libyuv/convert.h>
#include <libyuv/convert_argb.h>

namespace clips {
    VideoClip::VideoClip(const std::string& path): Clip(10, 60), path(path) {
        m_metadata.name = path;

        m_properties.addProperty(
            ClipProperty::position()
        );

        m_properties.addProperty(
            ClipProperty::number()
                ->setId("scale-x")
                ->setName("Scale X")
                ->setDefaultKeyframe(Vector1D{ .number = 100 }.toString())
        );

        m_properties.addProperty(
            ClipProperty::number()
                ->setId("scale-y")
                ->setName("Scale Y")
                ->setDefaultKeyframe(Vector1D{ .number = 100 }.toString())
        );

        m_properties.addProperty(
            ClipProperty::number()
                ->setId("rotation")
                ->setName("Rotation")
                ->setDefaultKeyframe(Vector1D{ .number = 0 }.toString())
        );

        if (!path.empty()) {
            initialize();
        }
    }

    VideoClip::VideoClip(): VideoClip("") {}

    bool VideoClip::initialize() {
        if (initialized) {
            return true;
        }

        auto genTexture = [](GLuint texture) {
            glBindTexture(GL_TEXTURE_2D, texture);
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        };
        
        glGenTextures(1, &textureY);
        glGenTextures(1, &textureU);
        glGenTextures(1, &textureV);

        genTexture(textureY);
        genTexture(textureU);
        genTexture(textureV);

        profile = mlt_profile_init("hdv_720_30p");
        if (profile == NULL) {
            fmt::println("no profile!");
            return false;
        }
        producer = mlt_factory_producer(profile, "avformat", path.c_str());
        if (producer == NULL) {
            fmt::println("no video!");
            return false;
        }

        fps = mlt_producer_get_fps(producer);
        mlt_producer_get_out(producer);

        initialized = true;

        return true;
    }

    Vector2D VideoClip::getSize() {    
        return { width, height };
    }

    Vector2D VideoClip::getPos() {
        Vector2D position = Vector2D::fromString(m_properties.getProperty("position")->data);
        return position;
    }

    VideoClip::~VideoClip() {
        mlt_producer_close(producer);
        mlt_profile_close(profile);
    }

    bool VideoClip::decodeFrame(int frameNumber) {
        if (!producer) {
            return false;
        }

        mlt_producer_seek(producer, frameNumber);

        mlt_frame frame = nullptr;
        if (mlt_service_get_frame(MLT_PRODUCER_SERVICE(producer), &frame, 0) != 0) {
            fmt::println("Failed to get frame");
            return false;
        }

        if (!frame) {
            fmt::println("Frame is null");
            return false;
        }

        mlt_image_format format = mlt_image_format::mlt_image_yuv420p;
        uint8_t* image = nullptr;

        if (mlt_frame_get_image(frame, &image, &format, &width, &height, 0) != 0 || !image) {
            mlt_frame_close(frame);
            return false;
        }

        if (vidFrame.empty()) {
            vidFrame.resize(width * height * 3);
        }

        uint8_t* y = image;
        uint8_t* u = y + width * height;
        uint8_t* v = u + static_cast<int>(std::rint((float)width / 2.f)) * static_cast<int>(std::rint((float)height / 2.f));

        // lots of code duplication i know...
        // this basically runs the more efficient version of
        // glTexImage2D if we have already uploaded initial data
        // or else it just doesn't work....
        if (hasUploaded) {
            glBindTexture(GL_TEXTURE_2D, textureY);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE, y);
            glBindTexture(GL_TEXTURE_2D, textureU);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width / 2, height / 2, GL_RED, GL_UNSIGNED_BYTE, u);
            glBindTexture(GL_TEXTURE_2D, textureV);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width / 2, height / 2, GL_RED, GL_UNSIGNED_BYTE, v);
        } else {
            glBindTexture(GL_TEXTURE_2D, textureY);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, y);
            glBindTexture(GL_TEXTURE_2D, textureU);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width / 2, height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, u);
            glBindTexture(GL_TEXTURE_2D, textureV);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width / 2, height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, v);
        }

        mlt_frame_close(frame);
        return true;
    }

    void VideoClip::render(Frame* frame) {
        initialize();

        auto& state = State::get();
        int targetFrame = floor((float)(state.currentFrame - startFrame) / ((float)state.video->getFPS() / (float)fps));
        if (targetFrame < 0) return;

        if (!decodeFrame(targetFrame)) return;

        float scaleX = Vector1D::fromString(m_properties.getProperty("scale-x")->data).number / 100.f;
        float scaleY = Vector1D::fromString(m_properties.getProperty("scale-y")->data).number / 100.f;
        if (scaleX <= 0 || scaleY <= 0) {
            return;
        }

        int rotation = Vector1D::fromString(m_properties.getProperty("rotation")->data).number;

        auto position = Vector2D::fromString(m_properties.getProperty("position")->data);

        int scaledW = static_cast<int>(std::floor(width * scaleX));
        int scaledH = static_cast<int>(std::floor(height * scaleY));

        frame->drawTextureYUV(textureY, textureU, textureV, position, { scaledW, scaledH }, rotation);
    }
}
