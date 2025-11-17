#include <clips/default/video.hpp>

#include <filesystem>

#include <mutex>
#include <state.hpp>
#include <utils.hpp>

#include <clips/properties/transform.hpp>
#include <clips/properties/number.hpp>

double roundToNearestN(double value, double n) {
    if (n == 0) {
        return 0; 
    }
    return std::floor(value / n) * n;
}


namespace clips {
    VideoClip::VideoClip(const std::string& path): Clip(10, 60), path(path) {
        m_metadata.name = std::filesystem::path(path).filename();

        addProperty(
            std::make_shared<TransformProperty>()
        );

        // scale x
        // default = 100
        addProperty(
            std::make_shared<NumberProperty>()
                ->setId("scale-x")
                ->setName("Scale X")
        );

        // scale y
        // default = 100
        addProperty(
            std::make_shared<NumberProperty>()
                ->setId("scale-y")
                ->setName("Scale Y")
        );

        // start time
        // default = 0
        addProperty(
            std::make_shared<NumberProperty>()
                ->setId("start-time")
                ->setName("Start Time")
        );

        glGenBuffers(1, &VBO);
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &EBO);

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
        std::lock_guard<std::mutex> guard(producerMutex);
        if (producer == NULL) {
            fmt::println("no video!");
            return false;
        }

        fps = mlt_producer_get_fps(producer);
        mlt_producer_get_out(producer);

        // preview gen thread
        previewGenThread = std::thread([this]() {
            while (true) {
                std::unique_lock<std::mutex> lk(this->framesMutex);
                this->previewCv.wait(lk, [&]() {
                    return (!this->pendingFrames.empty() && !State::get().isPlaying) || this->stopGen.load();
                });

                if (State::get().isPlaying) {
                    continue;
                }

                if (this->stopGen.load()) {
                    return; // we done here
                }

                std::vector<int> framesToDo;
                framesToDo.swap(this->pendingFrames);
                lk.unlock();
                for (int frameIdx : framesToDo) {
                    {
                        std::scoped_lock guard(this->framesMutex);
                        if (this->previewFrames.contains(frameIdx) || this->finishedFrames.contains(frameIdx)) continue;
                    }
                    auto res = this->decodeFrameRaw(frameIdx);
                    {
                        std::scoped_lock guard(this->framesMutex);
                        this->finishedFrames.emplace(frameIdx, std::move(res));
                    }
                    fmt::println("finished frame {}!", frameIdx);
                }
            }
        });

        initialized = true;

        return true;
    }

    Vector2D VideoClip::getSize() {    
        return { width, height };
    }

    Vector2D VideoClip::getPos() {
        Vector2D position = getProperty<TransformProperty>("transform").unwrap()->data.position;
        return position;
    }

    VideoClip::~VideoClip() {
        stopGen.store(true);
        mlt_producer_close(producer);
        mlt_profile_close(profile);
    }

    std::array<uint8_t*, 3> VideoClip::decodeFrameRaw(int frameNumber) {
        std::lock_guard<std::mutex> guard(producerMutex);
        if (!producer) {
            return {};
        }

        mlt_producer_seek(producer, frameNumber);

        mlt_frame frame = nullptr;
        if (mlt_service_get_frame(MLT_PRODUCER_SERVICE(producer), &frame, 0) != 0) {
            fmt::println("Failed to get frame");
            return {};
        }

        if (!frame) {
            fmt::println("Frame is null");
            return {};
        }

        mlt_image_format format = mlt_image_format::mlt_image_yuv420p;
        uint8_t* image = nullptr;

        if (mlt_frame_get_image(frame, &image, &format, &width, &height, 0) != 0 || !image) {
            mlt_frame_close(frame);
            return {};
        }

        uint8_t* y = image;
        uint8_t* u = y + width * height;
        uint8_t* v = u + static_cast<int>(std::rint((float)width / 2.f)) * static_cast<int>(std::rint((float)height / 2.f));

        mlt_frame_close(frame);
        return { y, u, v };
    }

    bool VideoClip::decodeFrame(int frameNumber) {
        auto yuv = decodeFrameRaw(frameNumber);
        if (yuv.size() != 3) {
            return false;
        }

        auto y = yuv[0];
        auto u = yuv[1];
        auto v = yuv[2];

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
        return true;
    }

    void VideoClip::render(Frame* frame) {
        initialize();

        auto& state = State::get();
        int startTime = getProperty<NumberProperty>("start-time").unwrap()->data;
        int offset = startTime * fps;
        int targetFrame = std::floor(state.video->timeForFrame(state.currentFrame - startFrame) * (float)fps) + offset;
        if (targetFrame < 0) return;

        if (!decodeFrame(targetFrame)) return;

        float scaleX = (float)getProperty<NumberProperty>("scale-x").unwrap()->data / 100.f;;
        float scaleY = (float)getProperty<NumberProperty>("scale-y").unwrap()->data / 100.f;;
        if (scaleX <= 0 || scaleY <= 0) {
            return;
        }

        auto transform = getProperty<TransformProperty>("transform").unwrap()->data;

        int scaledW = static_cast<int>(std::floor(width * scaleX));
        int scaledH = static_cast<int>(std::floor(height * scaleY));

        frame->drawTextureYUV(textureY, textureU, textureV, { scaledW, scaledH }, transform, VAO, VBO, EBO);
    }

    GLuint VideoClip::getPreviewTexture(int frameIdx) {
        auto& state = State::get();
        frameIdx = (int)roundToNearestN(std::floor(state.video->timeForFrame(frameIdx) * (float)fps), fps);
        {
            std::scoped_lock guard(framesMutex);
            if (previewFrames.contains(frameIdx)) {
                return previewFrames[frameIdx]->textureID;
            }
        }

        {
            std::scoped_lock guard(framesMutex);
            if (finishedFrames.contains(frameIdx)) {
                auto genTexture = [](GLuint texture) {
                    glBindTexture(GL_TEXTURE_2D, texture);
                    
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                };
                
                GLuint textureY, textureU, textureV;
    
                glGenTextures(1, &textureY);
                glGenTextures(1, &textureU);
                glGenTextures(1, &textureV);
    
                genTexture(textureY);
                genTexture(textureU);
                genTexture(textureV);
    
                fmt::println("before upload");
    
                auto y = finishedFrames[frameIdx][0];
                auto u = finishedFrames[frameIdx][1];
                auto v = finishedFrames[frameIdx][2];
    
                glBindTexture(GL_TEXTURE_2D, textureY);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, y);
                glBindTexture(GL_TEXTURE_2D, textureU);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width / 2, height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, u);
                glBindTexture(GL_TEXTURE_2D, textureV);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width / 2, height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, v);
    
                fmt::println("after upload");
    
                auto res = state.video->getResolution();
                previewFrames[frameIdx] = std::make_shared<Frame>(
                    res.x,
                    res.y
                );
                previewFrames[frameIdx]->clearFrame({ 0, 0, 0, 255 });
    
                previewFrames[frameIdx]->drawTextureYUV(
                    textureY,
                    textureU,
                    textureV,
                    { width, height },
                    { .position = { 0, 0 } },
                    VAO,
                    VBO,
                    EBO
                );
    
                finishedFrames.erase(frameIdx);
                utils::removeFromVector(pendingFrames, frameIdx);
                fmt::println("uploaded and erased :saluting_face:");
    
                // state.textRenderer->drawText(
                //     previewFrames[frameIdx].get(),
                //     fmt::format("frame {}", frameIdx),
                //     "resources/fonts/Inter.ttf",
                //     { 0, 0 },
                //     { 255, 255, 255, 255 },
                //     350
                // );
    
                return previewFrames[frameIdx]->textureID;
            }
        }

        {
            std::scoped_lock guard(framesMutex);
            if (!utils::vectorContains(pendingFrames, frameIdx)) {
                // generate it!
                pendingFrames.push_back(frameIdx);
                // return 0;
            }
        }

        previewCv.notify_one();
        
        return 0;
    }

    Vector2D VideoClip::getPreviewSize() { return State::get().video->getResolution(); }
}
