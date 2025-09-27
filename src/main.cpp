#include "libyuv/convert_from_argb.h"
#include <iostream>
#include <video.hpp>

#include <Application.hpp>
#include <state.hpp>

#include <print>

#include <format/vpf.hpp>

#include <miniaudio.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <libyuv/convert.h>
#include <libyuv/convert_argb.h>

void ClipProperties::addProperty(ClipProperty* property) {
    properties[property->id] = std::make_shared<ClipProperty>(*property);
}

std::shared_ptr<ClipProperty> ClipProperties::getProperty(std::string id) {
    return properties[id];
}

void ClipProperties::setKeyframe(std::string id, int frame, std::string data) {
    properties[id]->keyframes[frame] = data;
}

void ClipProperties::setKeyframeMeta(std::string id, int frame, PropertyKeyframeMeta data) {
    properties[id]->keyframeInfo[frame] = data;
}

Rectangle::Rectangle(): Clip(60, 120) {
    m_properties.addProperty(
        ClipProperty::dimensions()
    );

    m_properties.addProperty(
        ClipProperty::color()
    );

    m_metadata.name = "Rectangle";
}

void Rectangle::render(Frame* frame) {
    Dimensions dimensions = Dimensions::fromString(m_properties.getProperty("dimensions")->data);
    RGBAColor color = RGBAColor::fromString(m_properties.getProperty("color")->data);
    frame->drawRect(dimensions, color);
}


Circle::Circle(): Clip(60, 120) {
    m_properties.addProperty(
        ClipProperty::position()
    );

    m_properties.addProperty(
        ClipProperty::number()
            ->setName("Radius")
            ->setId("radius")
            ->setDefaultKeyframe(Vector1D{ .number = 500 }.toString())
    );

    m_properties.addProperty(
        ClipProperty::color()
    );

    m_metadata.name = "Circle";
}

void Circle::render(Frame* frame) {
    Vector2D position = Vector2D::fromString(m_properties.getProperty("position")->data);
    Vector1D radius = Vector1D::fromString(m_properties.getProperty("radius")->data);
    RGBAColor color = RGBAColor::fromString(m_properties.getProperty("color")->data);
    frame->drawCircle(position, radius.number, color);
}

Text::Text(): Clip(60, 120) {
    m_properties.addProperty(
        ClipProperty::position()
    );

    m_properties.addProperty(
        ClipProperty::color()
    );

    // default text property is fine here
    m_properties.addProperty(
        ClipProperty::text()
    );

    // TODO: change this to dropdown
    m_properties.addProperty(
        ClipProperty::text()
            ->setId("font")
            ->setName("Font")
            ->setDefaultKeyframe("inter.ttf")
    );

    // m_properties.addProperty(std::make_shared<ClipProperty>(ClipProperty({
    //     .id = "size",
    //     .name = "Font size",
    //     .type = PropertyType::Number,
    //     .keyframes = {
    //         { 0, Vector1D{ .number = 90 }.toString() }
    //     }
    // })));

    m_properties.addProperty(
        ClipProperty::number()
            ->setId("size")
            ->setName("Font Size")
            ->setDefaultKeyframe(Vector1D{ .number = 90 }.toString())
    );

    m_metadata.name = "Text";
}

void Text::render(Frame* frame) {
    auto& state = State::get();
    auto text = m_properties.getProperty("text")->data;
    auto font = m_properties.getProperty("font")->data;
    auto fontSize = Vector1D::fromString(m_properties.getProperty("size")->data).number;
    auto pos = Vector2D::fromString(m_properties.getProperty("position")->data);
    auto color = RGBAColor::fromString(m_properties.getProperty("color")->data);

    state.textRenderer->drawText(frame, text, font, pos, color, fontSize);
}

void drawImage(Frame* frame, unsigned char* data, Vector2D size, Vector2D position) {
    // TODO: account for scaling and rotation

    for (int srcY = 0; srcY < size.y; srcY++) {
        int dstY = srcY + position.y;
        if (dstY >= frame->height || dstY < 0) continue;
        for (int srcX = 0; srcX < size.x; srcX++) {
            int dstX = srcX + position.x;
            if (dstX >= frame->width || dstX < 0) continue;

            int srcLoc = (srcY * size.x + srcX) * 3;
            int dstLoc = (dstY * frame->width + dstX) * 4;
            uint8_t* dst = &frame->imageData[dstLoc];
            dst[0] = data[srcLoc];
            dst[1] = data[srcLoc + 1];
            dst[2] = data[srcLoc + 2];
            dst[3] = 255;
        }
    }
}

VideoClip::VideoClip(const std::string& path): Clip(10, 60) {
    profile = mlt_profile_init("hdv_720_30p");
    if (profile == NULL) {
        std::println("no profile!");
    }
    producer = mlt_factory_producer(profile, "avformat", path.c_str());
    if (producer == NULL) {
        std::println("no video!");
    }

    fps = mlt_producer_get_fps(producer);
    mlt_producer_get_out(producer);

    m_metadata.name = path;

    m_properties.addProperty(
        ClipProperty::position()
    );

    // m_properties.addProperty(std::make_shared<ClipProperty>(ClipProperty{
    //     .id = "scale",
    //     .name = "Scale",
    //     .type = PropertyType::Number,
    //     .keyframes = {
    //         { 0, Vector1D{ .number = 1 }.toString() }
    //     }
    // }));

    m_properties.addProperty(
        ClipProperty::number()
            ->setId("scale")
            ->setName("Scale")
            ->setDefaultKeyframe(Vector1D{ .number = 1 }.toString())
    );
}

VideoClip::~VideoClip() {
    mlt_producer_close(producer);
    mlt_profile_close(profile);
}

bool VideoClip::decodeFrame(int frameNumber) {
    mlt_producer_seek(producer, frameNumber);
    
    mlt_frame frame = nullptr;
    if (mlt_service_get_frame(MLT_PRODUCER_SERVICE(producer), &frame, 0) != 0) {
        std::println("Failed to get frame");
        return false;
    }
    
    if (!frame) {
        std::println("Frame is null");
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
    auto& state = State::get();
    int targetFrame = floor((float)(state.currentFrame - startFrame) / ((float)state.video->getFPS() / (float)fps));
    if (targetFrame < 0) return;

    if (!decodeFrame(targetFrame) || vidFrame.empty()) return;

    // int frameW = frame->width;
    // int frameH = frame->height;
    // int clipW  = width;
    // int clipH  = height;

    float scale = Vector1D::fromString(m_properties.getProperty("scale")->data).number;
    if (scale <= 0) {
        return;
    }

    auto position = Vector2D::fromString(m_properties.getProperty("position")->data);
    drawImage(frame, vidFrame.data(), { width, height }, position);

    // int scaledW = static_cast<int>(clipW * scale);
    // int scaledH = static_cast<int>(clipH * scale);

    // std::vector<unsigned char> scaledRow(scaledW * 4); // temp row buffer


    // for (int y = 0; y < scaledH; ++y) {
    //     int destY = position.y + y;
    //     if (destY < 0 || destY >= frameH) continue;

    //     float srcYf = y / scale;
    //     int srcY = static_cast<int>(srcYf);
    //     float yWeight = srcYf - srcY;
    //     int srcY1 = std::min(srcY + 1, clipH - 1);

    //     for (int x = 0; x < scaledW; ++x) {
    //         float srcXf = x / scale;
    //         int srcX = static_cast<int>(srcXf);
    //         float xWeight = srcXf - srcX;
    //         int srcX1 = std::min(srcX + 1, clipW - 1);

    //         int idx00 = (srcY  * clipW + srcX ) * 3;
    //         int idx01 = (srcY  * clipW + srcX1) * 3;
    //         int idx10 = (srcY1 * clipW + srcX ) * 3;
    //         int idx11 = (srcY1 * clipW + srcX1) * 3;

    //         for (int c = 0; c < 3; ++c) {
    //             float top    = vidFrame[idx00 + c] * (1 - xWeight) + vidFrame[idx01 + c] * xWeight;
    //             float bottom = vidFrame[idx10 + c] * (1 - xWeight) + vidFrame[idx11 + c] * xWeight;
    //             scaledRow[x * 4 + c] = static_cast<unsigned char>(top * (1 - yWeight) + bottom * yWeight);
    //         }

    //         scaledRow[x * 4] = scaledRow[x * 4 + 1] = scaledRow[x * 4 + 2] = (float)(
    //             scaledRow[x * 4] + scaledRow[x * 4 + 1] + scaledRow[x * 4 + 2]
    //         ) / 3.f;

    //         scaledRow[x * 4 + 3] = 255;
    //     }

    //     // copy into imageData directly
    //     // speed go brrrrrr
    //     unsigned char* dstRow = &frame->imageData[destY * frameW * 4];
    //     int copyWidth = std::min(scaledW, frameW - position.x);
    //     if (copyWidth > 0 && position.x < frameW) {
    //         std::memcpy(dstRow + position.x * 4, scaledRow.data(), copyWidth * 4);
    //     }
    // }
}

ImageClip::ImageClip(const std::string& path): Clip(0, 120) {
    imageData = stbi_load(path.c_str(), &width, &height, NULL, 3);
    if (imageData == NULL) {
        std::println("could not load image {}", path);
    }

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

    m_metadata.name = path;
}

void ImageClip::render(Frame* frame) {
    int frameW = frame->width;
    int frameH = frame->height;
    int clipW  = width;
    int clipH  = height;

    float scaleX = (float)Vector1D::fromString(m_properties.getProperty("scale-x")->data).number / 100.f;
    float scaleY = (float)Vector1D::fromString(m_properties.getProperty("scale-y")->data).number / 100.f;
    if (scaleX <= 0 || scaleY <= 0) {
        return;
    }
    auto position = Vector2D::fromString(m_properties.getProperty("position")->data);

    drawImage(frame, imageData, { width, height }, position);

    // int scaledW = static_cast<int>(clipW * scaleX);
    // int scaledH = static_cast<int>(clipH * scaleY);


    // std::vector<int> srcX(scaledW), srcX1(scaledW);
    // std::vector<float> xWeight(scaledW), invX(scaledW);
    // for (int x = 0; x < scaledW; ++x) {
    //     float srcXf = x / scaleX;
    //     srcX[x] = static_cast<int>(srcXf);
    //     xWeight[x] = srcXf - srcX[x];
    //     invX[x] = 1.0f - xWeight[x];
    //     srcX1[x] = std::min(srcX[x] + 1, clipW - 1);
    // }

    // for (int y = 0; y < scaledH; ++y) {
    //     int destY = position.y + y;
    //     if (destY < 0 || destY >= frameH) continue;

    //     float srcYf = y / scaleY;
    //     int srcY = static_cast<int>(srcYf);
    //     float yWeight = srcYf - srcY;
    //     float invY = 1.0f - yWeight;
    //     int srcY1 = std::min(srcY + 1, clipH - 1);

    //     // copy directly into frameData
    //     // speed go brrrrrr
    //     unsigned char* dstRow = &frame->imageData[destY * frameW * 4];
    //     int copyWidth = std::min(scaledW, frameW - position.x);
    //     if (copyWidth <= 0 || position.x >= frameW) continue;

    //     for (int x = 0; x < copyWidth; ++x) {
    //         int dstIdx = (position.x + x) * 4;

    //         int idx00 = (srcY  * clipW + srcX[x] ) * 3;
    //         int idx01 = (srcY  * clipW + srcX1[x]) * 3;
    //         int idx10 = (srcY1 * clipW + srcX[x] ) * 3;
    //         int idx11 = (srcY1 * clipW + srcX1[x]) * 3;

    //         for (int c = 0; c < 3; ++c) {
    //             float top    = imageData[idx00 + c] * invX[x] + imageData[idx01 + c] * xWeight[x];
    //             float bottom = imageData[idx10 + c] * invX[x] + imageData[idx11 + c] * xWeight[x];
    //             dstRow[dstIdx + c] = static_cast<unsigned char>(top * invY + bottom * yWeight);
    //         }
    //         dstRow[dstIdx + 3] = 255; // alpha
    //     }
    // }
}


ImageClip::~ImageClip() {
    if (imageData) {
        // stbi_image_free(&imageData);
        std::println("destructor sucess!");
    }
}

void ImageClip::onDelete() {
    if (imageData) {
        std::println("deletion!");
        stbi_image_free(imageData);
    }
}

void VideoTrack::render(Frame* frame, int currentFrame) {
    for (auto clip : clips) {
        if (currentFrame >= clip->startFrame && currentFrame <= clip->startFrame + clip->duration) {
            currentFrame = currentFrame - clip->startFrame;
            // process keyframes
            for (auto property : clip->m_properties.getProperties()) {
                // only one keyframe? use that
                if (property.second->keyframes.size() == 1) {
                    // std::println("one keyframe {}", clip->metadata.name);
                    property.second->data = property.second->keyframes[0];
                    continue;
                }

                // beyond the last keyframe? use that
                if (std::prev(property.second->keyframes.end())->first <= currentFrame - clip->startFrame) {
                    // std::println("one lasta keyframe {}", clip->metadata.name);
                    property.second->data = property.second->keyframes.rbegin()->second;
                    continue;
                }

                int previousKeyframe = 0;
                int nextKeyframe = 0;

                for (auto keyframe : property.second->keyframes) {
                    if (keyframe.first > currentFrame) {
                        nextKeyframe = keyframe.first;
                        break;
                    }

                    if (keyframe.first > previousKeyframe) {
                        previousKeyframe = keyframe.first;
                    }
                }

                if (nextKeyframe == 0) {
                    // somehow we did not catch the last keyframe, so we just set it here
                    property.second->data = property.second->keyframes[previousKeyframe];
                    continue;
                }

                // std::println("prev: {}, next: {}", previousKeyframe, nextKeyframe);

                float progress = (float)(currentFrame - previousKeyframe) / (float)(nextKeyframe - previousKeyframe);
                if (property.second->keyframeInfo.contains(nextKeyframe)) {
                    progress = animation::getEasingFunction(property.second->keyframeInfo[nextKeyframe].easing, property.second->keyframeInfo[nextKeyframe].mode)(progress);
                }

#define ANIMATE_NUM(old, new, arg) .arg = static_cast<int>(std::rint(old.arg + (float)(new.arg - old.arg) * progress))

                std::map<PropertyType, std::function<void()>> handlers = {
                    { PropertyType::Position, [&]() {
                        auto oldPos = Vector2D::fromString(property.second->keyframes[previousKeyframe]);
                        auto nextPos = Vector2D::fromString(property.second->keyframes[nextKeyframe]);

                        property.second->data = Vector2D{
                            ANIMATE_NUM(oldPos, nextPos, x),
                            ANIMATE_NUM(oldPos, nextPos, y),
                        }.toString();
                    } },
                    { PropertyType::Color, [&]() {
                        auto oldColor = RGBAColor::fromString(property.second->keyframes[previousKeyframe]);
                        auto nextColor = RGBAColor::fromString(property.second->keyframes[nextKeyframe]);

                        property.second->data = RGBAColor{
                            ANIMATE_NUM(oldColor, nextColor, r),
                            ANIMATE_NUM(oldColor, nextColor, g),
                            ANIMATE_NUM(oldColor, nextColor, b),
                            ANIMATE_NUM(oldColor, nextColor, a),
                        }.toString();
                        std::println("color: {}", property.second->data);
                    } },
                    { PropertyType::Number, [&]() {
                        auto oldNumber = Vector1D::fromString(property.second->keyframes[previousKeyframe]);
                        auto nextNumber = Vector1D::fromString(property.second->keyframes[nextKeyframe]);

                        property.second->data = Vector1D{
                            ANIMATE_NUM(oldNumber, nextNumber, number),
                        }.toString();
                    } },
                    { PropertyType::Dimensions, [&]() {
                        auto oldDimensions = Dimensions::fromString(property.second->keyframes[previousKeyframe]);
                        auto nextDimensions = Dimensions::fromString(property.second->keyframes[nextKeyframe]);

                        property.second->data = Dimensions{
                            .pos = {
                                ANIMATE_NUM(oldDimensions.pos, nextDimensions.pos, x),
                                ANIMATE_NUM(oldDimensions.pos, nextDimensions.pos, y),
                            },
                            .size = {
                                ANIMATE_NUM(oldDimensions.pos, nextDimensions.size, x),
                                ANIMATE_NUM(oldDimensions.pos, nextDimensions.size, y),
                            },
                        }.toString();
                    } },
                    { PropertyType::Text, [&]() {
                        // i do not want to bother animating text
                        // so this is the best we get
                        auto nextText = property.second->keyframes[nextKeyframe];
                        property.second->data = nextText;
                    } },
                    { PropertyType::Percent, [&]() {
                        handlers[PropertyType::Number]();
                    } },
                };

#undef ANIMATE_NUM

                if (handlers.contains(property.second->type)) {
                    handlers[property.second->type]();
                }
            }
            clip->render(frame);
        }
    }
}

void Video::addClip(int trackIdx, std::shared_ptr<Clip> clip) {
    getTracks()[trackIdx]->addClip(clip);
    recalculateFrameCount();
}

Frame* Video::renderAtFrame(int frameNum) {
    auto frame = std::make_shared<Frame>(resolution.x, resolution.y);
    renderIntoFrame(frameNum, frame);
    return frame.get();
}

void Video::renderIntoFrame(int frameNum, std::shared_ptr<Frame> frame) {
    for (auto track : videoTracks) {
        track->render(frame.get(), frameNum);
    }
}

void Video::render(VideoRenderer* renderer) {
    recalculateFrameCount();
    auto frame = std::make_shared<Frame>(resolution.x, resolution.y);
    auto& state = State::get();
    for (int currentFrame = 0; currentFrame < frameCount; currentFrame++) {
        state.currentFrame = currentFrame;
        frame->clearFrame();
        renderIntoFrame(currentFrame, frame);
        renderer->addFrame(frame);
    }
}

void Video::recalculateFrameCount() {
    int currentFrameCount = 0;
    for (auto track : videoTracks) {
        auto clips = track->getClips();
        for (auto clip : clips) {
            auto lastFrame = clip->startFrame + clip->duration;
            if (lastFrame > currentFrameCount) {
                currentFrameCount = lastFrame;
            }
        }
    }
    frameCount = currentFrameCount;
}

int Video::frameForTime(float time) {
    return std::floor(time * (float)getFPS());
}

float Video::timeForFrame(int frame) {
    return (float)frame / (float)getFPS();
}

AudioClip::AudioClip(const std::string& path): Clip(0, 7680) {
    auto& state = State::get();
    
    if (ma_sound_init_from_file(&state.soundEngine, path.c_str(), 0, NULL, NULL, &sound) != MA_SUCCESS) {
        std::println("could not init file");
    }
    m_metadata.name = path;
    m_properties.addProperty(
        ClipProperty::number()
            ->setId("volume")
            ->setName("Volume")
            ->setDefaultKeyframe(Vector1D{ .number = 100 }.toString())
    );
}

AudioClip::~AudioClip() {
    ma_sound_uninit(&sound);
}

void AudioClip::play() {
    if (this->playing) return;
    auto volume = Vector1D::fromString(m_properties.getProperty("volume")->data).number;
    ma_sound_set_volume(&sound, (float)volume / 255);
    if (ma_sound_start(&sound) != MA_SUCCESS) {
        std::println("no success :(");
    }
    this->playing = true;
}

void AudioClip::stop() {
    if (!this->playing) return;
    ma_sound_stop(&sound);
    // std::println("stop by justice");
    this->playing = false;
}

void AudioClip::seekToSec(float seconds) {
    // std::println("seekin to {}s", seconds);
    ma_sound_seek_to_second(&sound, seconds);
}

float AudioClip::getCursor() {
    float cursor = 0;
    ma_sound_get_cursor_in_seconds(&sound, &cursor);
    return cursor;
}

void AudioClip::setVolume(float volume) {
    ma_sound_set_volume(&sound, volume);
}

void AudioTrack::processTime() {
    auto& state = State::get();
    auto currentFrame = state.currentFrame;
    for (auto clip : clips) {
        if (currentFrame >= clip->startFrame && currentFrame < clip->startFrame + clip->duration && state.isPlaying) {
            // std::println("start");
            float seconds = state.video->timeForFrame(currentFrame - clip->startFrame);
            if (std::abs(clip->getCursor() - seconds) >= 0.5f) {
                clip->seekToSec(seconds);
            }
            clip->play();
        }
        if (clip->startFrame + clip->duration < currentFrame && state.isPlaying) {
            clip->stop();
        }
        for (auto property : clip->m_properties.getProperties()) {
            // only one keyframe? use that
            if (property.second->keyframes.size() == 1) {
                property.second->data = property.second->keyframes[0];
                continue;
            }

            // beyond the last keyframe? use that
            if (std::prev(property.second->keyframes.end())->first <= currentFrame - clip->startFrame) {
                property.second->data = property.second->keyframes.rbegin()->second;
                continue;
            }

            int previousKeyframe = 0;
            int nextKeyframe = 0;

            for (auto keyframe : property.second->keyframes) {
                if (keyframe.first > currentFrame) {
                    nextKeyframe = keyframe.first;
                    break;
                }

                if (keyframe.first > previousKeyframe) {
                    previousKeyframe = keyframe.first;
                }
            }

            if (nextKeyframe == 0) {
                // somehow we did not catch the last keyframe, so we just set it here
                property.second->data = property.second->keyframes[previousKeyframe];
                continue;
            }

            float progress = (float)(currentFrame - previousKeyframe) / (float)(nextKeyframe - previousKeyframe);
            if (property.second->keyframeInfo.contains(nextKeyframe)) {
                progress = animation::getEasingFunction(property.second->keyframeInfo[nextKeyframe].easing, property.second->keyframeInfo[nextKeyframe].mode)(progress);
            }

            auto oldNumber = Vector1D::fromString(property.second->keyframes[previousKeyframe]);
            auto nextNumber = Vector1D::fromString(property.second->keyframes[nextKeyframe]);

#define ANIMATE_NUM(old, new, arg) .arg = static_cast<int>(std::rint(old.arg + (float)(new.arg - old.arg) * progress))

            property.second->data = Vector1D{
                ANIMATE_NUM(oldNumber, nextNumber, number),
            }.toString();

#undef ANIMATE_NUM
        }

        clip->setVolume(Vector1D::fromString(clip->m_properties.getProperty("volume")->data).number / 100.f);
    }
}

void AudioTrack::onPlay() {
    auto& state = State::get();
    // filters clips that should be playing now
    // auto clips_view = clips | std::views::filter([state](std::shared_ptr<AudioClip> clip) {
    //     return clip->startFrame >= state.currentFrame && state.currentFrame <= clip->startFrame + clip->duration;
    // });
    for (auto clip : clips) {
        if (state.currentFrame >= clip->startFrame && state.currentFrame <= clip->startFrame + clip->duration) {
            std::println("hjjge");
            clip->seekToSec(state.video->timeForFrame(state.currentFrame - clip->startFrame));
        }
    }
}

void AudioTrack::onStop() {
    for (auto clip : clips) {
        clip->stop();
    }
}

int main() {
    // Mlt::Factory::init();
    if (mlt_factory_init("resources/mlt") == 0) {
        std::println("unable to init mlt factory");
    }

    // AudioRenderer renderer("test.mp3", 60.f);
    // renderer.addClip("/home/dee/Music/1191779_-PARTY-SIRENS-.mp3", 0.f, 40.f, std::make_shared<ClipProperty>(ClipProperty{
    //     .keyframes = {
    //         { 0, Vector1D{ .number = 0 }.toString() },
    //         { 360, Vector1D{ .number = 100 }.toString() }
    //     },
    //     .keyframeInfo = {
    //         { 0, {} }
    //     }
    // }));
    // renderer.addClip("/home/dee/Music/1191781_The-91s-Conundrum.mp3", 20.f, 55.f, std::make_shared<ClipProperty>(ClipProperty{
    //     .keyframes = {
    //         { 0, Vector1D{ .number = 35 }.toString() }
    //     },
    //     .keyframeInfo = {
    //         { 0, {} }
    //     }
    // }));
    // renderer.addClip("/home/dee/Music/Justice - Phantom Pt II (Official Audio) [5QCBkwmsOk0].mp3", 15.f, 55.f, std::make_shared<ClipProperty>(ClipProperty{
    //     .keyframes = {
    //         { 0, Vector1D{ .number = 35 }.toString() }
    //     },
    //     .keyframeInfo = {
    //         { 0, {} }
    //     }
    // }));
    // renderer.render(30);

    std::cout << "hello!" << std::endl;


    int width = 1920;
    int height = 1080;
    int fps = 60;

    // VPFFile::convertToVPF("/home/dee/Videos/Video_2025-09-06_10-36-39.mp4");

    // auto textRenderer = std::make_unique<TextRenderer>("inter.ttf");
    // auto videoRenderer = std::make_unique<VideoRenderer>("output.mp4", width, height, fps);

    auto video = new Video(fps, { width, height });
    auto vidTrack1 = new VideoTrack();
    video->addTrack(vidTrack1);

    auto vidTrack2 = new VideoTrack();
    video->addTrack(vidTrack2);

    // auto imgClip = std::make_shared<ImageClip>("/home/dee/Pictures/underscored v2 bg darkened.png");
    // video->addClip(0, imgClip);

    auto rectClip = std::make_shared<Circle>();
    rectClip->m_properties.setKeyframe("position", 120, Vector2D{ .x = 500, .y = 1000}.toString());
    rectClip->m_properties.setKeyframe("position", 180, Vector2D{ .x = 200, .y = 800}.toString());
    rectClip->m_properties.setKeyframe("position", 240, Vector2D{ .x = 0, .y = 500}.toString());

    rectClip->m_properties.setKeyframeMeta("position", 120, {
        .easing = animation::Easing::Exponential,
        .mode = animation::EasingMode::EaseOut
    });

    rectClip->m_properties.setKeyframeMeta("position", 180, {
        .easing = animation::Easing::Exponential,
        .mode = animation::EasingMode::EaseIn
    });

    rectClip->m_properties.setKeyframeMeta("position", 240, {
        .easing = animation::Easing::Cubic,
        .mode = animation::EasingMode::EaseInOut
    });


    rectClip->m_properties.setKeyframe("color", 90, RGBAColor{ .r = 255, .g = 0, .b = 0 }.toString());
    rectClip->m_properties.setKeyframe("color", 150, RGBAColor{ .r = 0, .g = 255, .b = 0 }.toString());
    rectClip->m_properties.setKeyframe("color", 210, RGBAColor{ .r = 0, .g = 0, .b = 255 }.toString());

    rectClip->m_properties.setKeyframeMeta("color", 90, {
        .easing = animation::Easing::Backwards,
        .mode = animation::EasingMode::EaseInOut
    });

    rectClip->m_properties.setKeyframeMeta("color", 150, {
        .easing = animation::Easing::Circular,
        .mode = animation::EasingMode::EaseOut
    });

    rectClip->m_properties.setKeyframeMeta("color", 210, {
        .easing = animation::Easing::Exponential,
        .mode = animation::EasingMode::EaseOut
    });

    rectClip->m_properties.setKeyframe("radius", 60, Vector1D{ .number = 250 }.toString());
    rectClip->m_properties.setKeyframe("radius", 120, Vector1D{ .number = 600 }.toString());
    rectClip->m_properties.setKeyframe("radius", 180, Vector1D{ .number = 150 }.toString());

    rectClip->m_properties.setKeyframeMeta("radius", 60, {
        .easing = animation::Easing::Quadratic,
        .mode = animation::EasingMode::EaseInOut
    });

    rectClip->m_properties.setKeyframeMeta("radius", 120, {
        .easing = animation::Easing::Elastic,
        .mode = animation::EasingMode::EaseInOut
    });

    rectClip->m_properties.setKeyframeMeta("radius", 180, {
        .easing = animation::Easing::Sine,
        .mode = animation::EasingMode::EaseInOut
    });


    video->addClip(0, rectClip);

    // auto vidTrack3 = new VideoTrack();
    // video->addTrack(vidTrack3);

    // auto vidTrack4 = new VideoTrack();
    // video->addTrack(vidTrack4);

    // auto vidTrack5 = new VideoTrack();
    // video->addTrack(vidTrack5);

    auto audTrack1 = new AudioTrack();
    video->audioTracks.push_back(audTrack1);

    auto audTrack2 = new AudioTrack();
    video->audioTracks.push_back(audTrack2);

    auto& state = State::get();
    if (ma_engine_init(NULL, &state.soundEngine) != MA_SUCCESS) {
        std::println("could not init engine");
    }

    // auto clip = std::make_shared<AudioClip>("/home/dee/Music/1191781_The-91s-Conundrum.mp3");
    // audTrack1->addClip(clip);

    // video->addClip(1, std::make_shared<VideoClipV2>("/home/dee/Videos/Video_2025-08-26_22-14-33.mp4"));
    // video->render(videoRenderer.get());

    // videoRenderer->finish();
    
    state.video = video;
    state.textRenderer = std::make_shared<TextRenderer>();
    
    // ma_result result;
    // ma_engine engine;

    Application app;
    app.setup();
    app.run();

    ma_engine_uninit(&state.soundEngine);
    mlt_factory_close();

    return 0;
}
