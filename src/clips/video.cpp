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
                ->setId("scale")
                ->setName("Scale")
                ->setDefaultKeyframe(Vector1D{ .number = 1 }.toString())
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

        libyuv::I420ToRGB24(
            y, width,
            v, std::rint((float)width / 2.f),
            u, std::rint((float)width / 2.f),
            vidFrame.data(), width * 3,
            width, height
        );

        mlt_frame_close(frame);
        return true;
    }

    void VideoClip::render(Frame* frame) {
        initialize();

        auto& state = State::get();
        int targetFrame = floor((float)(state.currentFrame - startFrame) / ((float)state.video->getFPS() / (float)fps));
        if (targetFrame < 0) return;

        if (!decodeFrame(targetFrame) || vidFrame.empty()) return;

        float scale = Vector1D::fromString(m_properties.getProperty("scale")->data).number;
        if (scale <= 0) {
            return;
        }

        auto position = Vector2D::fromString(m_properties.getProperty("position")->data);
        utils::drawImage(frame, vidFrame.data(), { width, height }, position);
    }
}
