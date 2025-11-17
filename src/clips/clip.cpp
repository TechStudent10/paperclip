#include <clips/clip.hpp>
#include <state.hpp>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <clips/properties/color.hpp>
#include <clips/properties/dimensions.hpp>
#include <clips/properties/dropdown.hpp>
#include <clips/properties/number.hpp>
#include <clips/properties/position.hpp>
#include <clips/properties/text.hpp>
#include <clips/properties/transform.hpp>

void Clip::dispatchChange() {
    auto& state = State::get();
    state.lastRenderedFrame = -1;
    for (auto audTrack : state.video->audioTracks) {
        audTrack->processTime();
    }
}

void ClipPropertyBase::processKeyframe(int targetFrame) {
    auto keyframes = getKeyframes();
    fmt::println("currently processing property {}", id);
    if (debug(keyframes.size()) == 1) {
        fmt::println("only one keyframe; using it");
        updateData(1, keyframes[0], keyframes[0]);
        return;
    }

    // beyond the last keyframe? use that
    if (keyframes[keyframes.size() - 1] <= targetFrame - clip->startFrame) {
        fmt::println("beyond last keyframe; using it");
        updateData(1, keyframes[keyframes.size() - 1], keyframes[keyframes.size() - 1]);
        return;
    }

    int previousKeyframe = 0;
    int nextKeyframe = 0;

    for (auto keyframe : keyframes) {
        if (keyframe > targetFrame - clip->startFrame) {
            nextKeyframe = keyframe;
            break;
        }

        if (keyframe > previousKeyframe) {
            previousKeyframe = keyframe;
        }
    }

    if (nextKeyframe == 0) {
        // somehow we did not catch the last keyframe, so we just set it here
        fmt::println("did not catch that we are beyond the last keyframe; using it");
        updateData(1, previousKeyframe, previousKeyframe);
        return;
    }

    float progress = (float)(targetFrame - clip->startFrame - previousKeyframe) / (float)(nextKeyframe - previousKeyframe);
    if (keyframeInfo.contains(nextKeyframe)) {
        progress = animation::getEasingFunction(keyframeInfo[nextKeyframe].easing, keyframeInfo[nextKeyframe].mode)(progress);
    }

    debug(previousKeyframe);
    debug(nextKeyframe);

    updateData(progress, previousKeyframe, nextKeyframe);
}

void ClipPropertyBase::_drawProperty() {
    drawProperty();

    auto keyframes = getKeyframes();

    auto& state = State::get();

    int keyframe = state.currentFrame - clip->startFrame;
    bool isKeyframed = utils::vectorContains(keyframes, keyframe);

    if (ImGui::Button(fmt::format("{}Keyframe {}", isKeyframed ? "Remove " : "", name).c_str())) {
        if (isKeyframed) {
            removeKeyframe(keyframe);

            if (keyframeInfo.contains(keyframe)) {
                keyframeInfo.erase(keyframe);
            }
        } else if (!isKeyframed && keyframe >= 0) {
            addKeyframe(keyframe);
        }
    }

    if (keyframes.size() > 1) {
        int previousKeyframe = 0;
        int nextKeyframe = 0;

        for (auto keyframeObj : keyframes) {
            if (keyframeObj > keyframe) {
                nextKeyframe = keyframeObj;
                break;
            }

            if (keyframeObj > previousKeyframe) {
                previousKeyframe = keyframeObj;
            }
        }

        if (ImGui::Button(fmt::format("Go to next##{}", id).c_str())) {
            state.currentFrame = clip->startFrame + nextKeyframe;
        }

        ImGui::SameLine();

        if (ImGui::Button(fmt::format("Go to previous##{}", id).c_str())) {
            state.currentFrame = clip->startFrame + previousKeyframe;
        }

        auto currentEasing = animation::EASING_NAMES[(int)keyframeInfo[nextKeyframe].easing];
        if (ImGui::BeginCombo(fmt::format("Easing##{}", id).c_str(), currentEasing)) {
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
        if (ImGui::BeginCombo(fmt::format("Mode##{}", id).c_str(), currentMode)) {
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

void Clip::write(qn::HeapByteWriter& writer) {
    writer.writeI16((int)getType());
    fmt::println("wrote {} as type", (int)getType());
    writer.writeI64(m_properties.size());
    for (auto [_, property] : m_properties) {
        property->write(writer);
    }
    m_metadata.write(writer);
    writer.writeI16(startFrame);
    writer.writeI16(duration);
}

void Clip::read(qn::ByteReader& reader) {
    int size = reader.readI64().unwrapOr(0);
    debug(size);
    for (int i = 0; i < size; i++) {
        PropertyType type = (PropertyType)(debug(reader.readI16().unwrapOr(0)));
        std::shared_ptr<ClipPropertyBase> property;
        switch (type) {
            case PropertyType::Color:
                property = std::make_shared<ColorProperty>();
                break;
            case PropertyType::Text:
                property = std::make_shared<TextProperty>();
                break;
            case PropertyType::Percent: [[fallthrough]];
            case PropertyType::Number:
                property = std::make_shared<NumberProperty>();
                break;
            case PropertyType::Dimensions:
                property = std::make_shared<DimensionsProperty>();
                break;
            case PropertyType::Position:
                property = std::make_shared<PositionProperty>();
                break;
            case PropertyType::Transform:
                property = std::make_shared<TransformProperty>();
                break;
            case PropertyType::Dropdown:
                property = std::make_shared<DropdownProperty>();
                break;
        }
        debug((int)type);
        property->read(reader);
        addProperty(property);
    }
    m_metadata.read(reader);
    startFrame = reader.readI16().unwrapOr(0);
    duration = reader.readI16().unwrapOr(0);
}
