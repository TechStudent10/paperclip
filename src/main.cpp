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

#include <imgui/misc/cpp/imgui_stdlib.h>

#include <cereal/types/polymorphic.hpp>

static constexpr float PI_DIV_180 = std::numbers::pi / 180.f;

void ClipProperty::drawProperty() {
    auto& state = State::get();
    auto setData = [&](std::string data) {
        if (keyframes.size() == 1) {
            keyframes[0] = data;
            state.lastRenderedFrame = -1;
            for (auto audTrack : state.video->audioTracks) {
                audTrack->processTime();
            }
            return;
        }

        int keyframe = state.currentFrame - state.draggingClip->startFrame;
        state.draggingClip->m_properties.setKeyframe(id, keyframe, data);
        state.lastRenderedFrame = -1;
        for (auto audTrack : state.video->audioTracks) {
            audTrack->processTime();
        }
    };

    auto drawDimensions = [&]() {
        Dimensions dimensions = Dimensions::fromString(data);

        bool x = ImGui::DragInt("X", &dimensions.pos.x);
        bool y = ImGui::DragInt("Y", &dimensions.pos.y);
        bool w = ImGui::DragInt("Width", &dimensions.size.x);
        bool h = ImGui::DragInt("Height", &dimensions.size.y);
        
        if (x || y || w || h) {
            setData(dimensions.toString());
        }
    };

    auto drawPosition = [&]() {
        Vector2D position = Vector2D::fromString(data);

        bool x = ImGui::DragInt("X", &position.x);
        bool y = ImGui::DragInt("Y", &position.y);

        if (x || y) {
            setData(position.toString());
        }
    };

    auto drawInt = [&]() {
        Vector1D number = Vector1D::fromString(data);
        if (ImGui::DragInt(
            std::format("##{}", name).c_str(),
            &number.number,
            1.0f,
            0,
            type == PropertyType::Percent ? 100 : 0
        )) {
            setData(number.toString());
        }
    };

    auto drawColorPicker = [&]() {
        RGBAColor color = RGBAColor::fromString(data);

        float colorBuf[4] = { (float)color.r / 255, (float)color.g / 255, (float)color.b / 255, (float)color.a / 255 };
        if (ImGui::ColorPicker4("Color", colorBuf)) {
            color.r = std::round(colorBuf[0] * 255);
            color.g = std::round(colorBuf[1] * 255);
            color.b = std::round(colorBuf[2] * 255);
            color.a = std::round(colorBuf[3] * 255);

            setData(color.toString());
        }
    };

    auto drawText = [&]() {
        std::string text = data;
        if (ImGui::InputText(std::format("##{}", id).c_str(), &text)) {
            setData(text);
        }
    };

    auto drawDropdown = [this, setData]() {
        DropdownOptions dropdownOptions = DropdownOptions::fromString(options);

        if (ImGui::BeginCombo(name.c_str(), data.c_str())) {
            for (auto option : dropdownOptions.options) {
                bool selected = option == data;
                auto optionStr = std::string(option);
                if (ImGui::Selectable(optionStr.c_str(), selected)) {
                    setData(optionStr);
                }

                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }
    };

    std::map<PropertyType, std::function<void()>> drawFuncs = {
        { PropertyType::Dimensions, drawDimensions },
        { PropertyType::Color, drawColorPicker },
        { PropertyType::Number, drawInt },
        { PropertyType::Percent, drawInt },
        { PropertyType::Position, drawPosition },
        { PropertyType::Text, drawText },
        { PropertyType::Dropdown, drawDropdown }
    };
    drawFuncs[type]();

    int keyframe = state.currentFrame - state.draggingClip->startFrame;
    bool isKeyframed = keyframes.contains(keyframe);

    if (ImGui::Button(std::format("{}Keyframe {}", isKeyframed ? "Remove " : "", name).c_str())) {
        if (isKeyframed) {
            keyframes.erase(keyframe);

            if (keyframeInfo.contains(keyframe)) {
                keyframeInfo.erase(keyframe);
            }
        } else if (!isKeyframed && keyframe >= 0) {
            state.draggingClip->m_properties.setKeyframe(id, keyframe, data);
        }
    }

    if (keyframes.size() > 1) {
        int previousKeyframe = 0;
        int nextKeyframe = 0;

        for (auto keyframeObj : keyframes) {
            if (keyframeObj.first > keyframe) {
                nextKeyframe = keyframeObj.first;
                break;
            }

            if (keyframeObj.first > previousKeyframe) {
                previousKeyframe = keyframeObj.first;
            }
        }

        if (ImGui::Button(std::format("Go to next##{}", id).c_str())) {
            state.currentFrame = state.draggingClip->startFrame + nextKeyframe;
        }

        ImGui::SameLine();

        if (ImGui::Button(std::format("Go to previous##{}", id).c_str())) {
            state.currentFrame = state.draggingClip->startFrame + previousKeyframe;
        }

        auto currentEasing = animation::EASING_NAMES[(int)keyframeInfo[nextKeyframe].easing];
        if (ImGui::BeginCombo(std::format("Easing##{}", id).c_str(), currentEasing)) {
            for (int i = 0; i < animation::EASING_NAMES.size(); i++) {
                auto easingName = animation::EASING_NAMES[i];
                bool selected = easingName == currentEasing;
                if (ImGui::Selectable(easingName, selected)) {
                    // this is so stupid i love it
                    keyframeInfo[nextKeyframe].easing = (animation::Easing)i;
                }

                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }

        auto currentMode = animation::EASING_MODE_NAMES[(int)keyframeInfo[nextKeyframe].mode];
        if (ImGui::BeginCombo(std::format("Mode##{}", id).c_str(), currentMode)) {
            for (int i = 0; i < animation::EASING_MODE_NAMES.size(); i++) {
                auto modeName = animation::EASING_MODE_NAMES[i];
                bool selected = modeName == currentMode;
                if (ImGui::Selectable(modeName, selected)) {
                    // this is so stupid i love it x2
                    keyframeInfo[nextKeyframe].mode = (animation::EasingMode)i;
                }

                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }
    }
}

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

    m_properties.addProperty(
        ClipProperty::dropdown()
            ->setId("font")
            ->setName("Font")
            ->setOptions(DropdownOptions{
                .options = std::vector<std::string>({
                    "Inter",
                    "JetBrains Mono",
                    "Noto Sans",
                    "Noto Serif",
                    "Open Sans",
                    "Oswald",
                    "Raleway",
                    "Raleway Dots",
                    "Roboto",
                    "Source Code Pro",
                    "Source Serif 4"
                })
            }.toString())
            ->setDefaultKeyframe("Inter")
    );

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
    auto font = std::format("resources/fonts/{}.ttf", m_properties.getProperty("font")->data);
    auto fontSize = Vector1D::fromString(m_properties.getProperty("size")->data).number;
    auto pos = Vector2D::fromString(m_properties.getProperty("position")->data);
    auto color = RGBAColor::fromString(m_properties.getProperty("color")->data);

    state.textRenderer->drawText(frame, text, font, pos, color, fontSize);
}

void drawImage(Frame* frame, unsigned char* data, Vector2D size, Vector2D position, float rotation = 0) {
    // TODO: account for scaling

    // x'' = (x' * cos(θ)) - (y' * sin(θ))
    // y'' = (x' * sin(θ)) + (y' * cos(θ))
    // x'' = rotated x
    // y'' = rotated y
    // x' = x - center x
    // y' = y - center y

    // rotation = rotation % 360.f;
    float radians = rotation * PI_DIV_180;
    // float radians = 1.5707961;
    // float radians = 0;
    float centerX = (float)position.x + (float)size.x / 2.f;
    float centerY = (float)position.y + (float)size.y / 2.f;

    float cosR = cos(radians);
    float sinR = sin(radians);

    for (int dstY = 0; dstY < size.y + 10; dstY++) {
        for (int dstX = 0; dstX < size.x + 10; dstX++) {
            int frameX = dstX + position.x;
            if (frameX >= frame->width || frameX < 0) continue;

            int frameY = dstY + position.y;
            if (frameY >= frame->height || frameY < 0) continue;

            int srcX = dstX;
            int srcY = dstY;

            if (rotation != 0) {
                float xPrime = dstX - centerX;
                float yPrime = dstY - centerY;
                // std::println("b: {}, {}, {}", yPrime, _dstY, size.y);
                // inverse rotation
                float rotatedSrcX = (xPrime * cos(radians)) + (yPrime * sin(radians)) + centerX / 4.f;
                float rotatedSrcY = -(xPrime * sin(radians)) + (yPrime * cos(radians)) + centerY / 4.f;

                srcX = static_cast<int>(std::round(rotatedSrcX));
                srcY = static_cast<int>(std::round(rotatedSrcY));

                if (srcX >= size.x || srcX < 0) continue; 
                if (srcY >= size.y || srcY < 0) continue;
            }

            int srcLoc = (srcY * size.x + srcX) * 3;
            int dstLoc = (frameY * frame->width + frameX) * 4;
            uint8_t* dst = &frame->imageData[dstLoc];
            dst[0] = data[srcLoc];
            dst[1] = data[srcLoc + 1];
            dst[2] = data[srcLoc + 2];
            dst[3] = 255;
        }
    }
}

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
        std::println("no profile!");
        return false;
    }
    producer = mlt_factory_producer(profile, "avformat", path.c_str());
    if (producer == NULL) {
        std::println("no video!");
        return false;
    }

    fps = mlt_producer_get_fps(producer);
    mlt_producer_get_out(producer);

    initialized = true;

    return true;
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
    drawImage(frame, vidFrame.data(), { width, height }, position);
}

ImageClip::ImageClip(const std::string& path): Clip(0, 120), path(path) {
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
            ->setName("Rotation (deg)")
            ->setDefaultKeyframe(Vector1D{ .number = 0 }.toString())
    );

    m_metadata.name = path;

    if (path.empty()) return;

    initialize();
}

bool ImageClip::initialize() {
    if (initialized) {
        return true;
    }

    imageData = stbi_load(path.c_str(), &width, &height, NULL, 3);
    if (imageData == NULL) {
        std::println("could not load image {}", path);
        return false;
    }

    initialized = true;

    return true;
}

ImageClip::ImageClip(): ImageClip("") {
    std::println("construct: {} ({})", path, sizeof(imageData));
}

void ImageClip::render(Frame* frame) {
    initialize();

    int frameW = frame->width;
    int frameH = frame->height;
    int clipW  = width;
    int clipH  = height;

    float scaleX = (float)Vector1D::fromString(m_properties.getProperty("scale-x")->data).number / 100.f;
    float scaleY = (float)Vector1D::fromString(m_properties.getProperty("scale-y")->data).number / 100.f;
    int rotation = Vector1D::fromString(m_properties.getProperty("rotation")->data).number;
    if (scaleX <= 0 || scaleY <= 0) {
        return;
    }
    auto position = Vector2D::fromString(m_properties.getProperty("position")->data);

    drawImage(frame, imageData, { width, height }, position, rotation);
}

ImageClip::~ImageClip() {
    onDelete();
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
                        auto nextText = property.second->keyframes[previousKeyframe];
                        property.second->data = nextText;
                    } },
                    { PropertyType::Dropdown, [&]() {
                        // same deal as text
                        auto nextOption = property.second->keyframes[previousKeyframe];
                        property.second->data = nextOption;
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

AudioClip::AudioClip(const std::string& path): Clip(0, 7680), path(path) {
    m_metadata.name = path;
    m_properties.addProperty(
        ClipProperty::number()
            ->setId("volume")
            ->setName("Volume")
            ->setDefaultKeyframe(Vector1D{ .number = 100 }.toString())
    );

    if (!path.empty()) {
        initalize();
    }
}

AudioClip::AudioClip(): AudioClip("") {}

AudioClip::~AudioClip() {
    ma_sound_uninit(&sound);
}

bool AudioClip::initalize() {
    if (initialized) {
        return true;
    }

    auto& state = State::get();
    
    if (ma_sound_init_from_file(&state.soundEngine, path.c_str(), 0, NULL, NULL, &sound) != MA_SUCCESS) {
        std::println("could not init file");
        return false;
    }

    initialized = true;

    return true;
}

void AudioClip::play() {
    initalize();
    if (this->playing) return;
    auto volume = Vector1D::fromString(m_properties.getProperty("volume")->data).number;
    ma_sound_set_volume(&sound, (float)volume / 255);
    if (ma_sound_start(&sound) != MA_SUCCESS) {
        std::println("no success :(");
    }
    this->playing = true;
}

void AudioClip::stop() {
    initalize();
    if (!this->playing) return;
    ma_sound_stop(&sound);
    this->playing = false;
}

void AudioClip::seekToSec(float seconds) {
    initalize();
    ma_sound_seek_to_second(&sound, seconds);
}

float AudioClip::getCursor() {
    initalize();
    float cursor = 0;
    ma_sound_get_cursor_in_seconds(&sound, &cursor);
    return cursor;
}

void AudioClip::setVolume(float volume) {
    initalize();
    ma_sound_set_volume(&sound, volume);
}

void AudioTrack::processTime() {
    auto& state = State::get();
    auto currentFrame = state.currentFrame;
    for (auto clip : clips) {
        if (currentFrame >= clip->startFrame && currentFrame < clip->startFrame + clip->duration && state.isPlaying) {
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
    for (auto clip : clips) {
        if (state.currentFrame >= clip->startFrame && state.currentFrame <= clip->startFrame + clip->duration) {
            clip->seekToSec(state.video->timeForFrame(state.currentFrame - clip->startFrame));
        }
    }
}

void AudioTrack::onStop() {
    for (auto clip : clips) {
        clip->stop();
    }
}

CEREAL_REGISTER_DYNAMIC_INIT(AudioClip)
CEREAL_REGISTER_DYNAMIC_INIT(ImageClip)
CEREAL_REGISTER_DYNAMIC_INIT(VideoClip)

int main() {
    if (mlt_factory_init("resources/mlt") == 0) {
        std::println("unable to init mlt factory");
    }
    std::cout << "hello!" << std::endl;


    int width = 1920;
    int height = 1080;
    int fps = 60;

    auto video = std::make_shared<Video>();
    video->framerate = 60;
    video->resolution = {width, height};
    video->addTrack(std::make_shared<VideoTrack>());
    video->addTrack(std::make_shared<VideoTrack>());

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

    video->audioTracks.push_back(std::make_shared<AudioTrack>());
    video->audioTracks.push_back(std::make_shared<AudioTrack>());

    auto& state = State::get();
    if (ma_engine_init(NULL, &state.soundEngine) != MA_SUCCESS) {
        std::println("could not init engine");
    }
    
    state.video = video;
    state.textRenderer = std::make_shared<TextRenderer>();

    Application app;
    app.setup();
    app.run();

    ma_engine_uninit(&state.soundEngine);
    mlt_factory_close();

    return 0;
}
