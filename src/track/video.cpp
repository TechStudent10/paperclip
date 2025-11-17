#include <track/video.hpp>

#include <functional>

void VideoTrack::render(Frame* frame, int targetFrame) {
    for (auto _clip : clips) {
        auto clip = _clip.second;
        if (clip->getType() == ClipType::Text) {
            // fmt::println("current: {}\nstart: {}\nduration: {}\n-----------", currentFrame, clip->startFrame, clip->duration);
        }
        if (targetFrame >= clip->startFrame && targetFrame <= clip->startFrame + clip->duration) {
            int currentFrame = targetFrame - clip->startFrame;
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
