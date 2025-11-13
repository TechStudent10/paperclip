#include "widgets.hpp"
#include <clips/clip.hpp>
#include <state.hpp>
#include <functional>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

void ClipProperty::drawProperty() {
    auto& state = State::get();
    if (!clip) {
        fmt::println("no clip!");
        return;
    }

    auto setData = [&](std::string data) {
        if (keyframes.size() == 1) {
            keyframes[0] = data;
            state.lastRenderedFrame = -1;
            for (auto audTrack : state.video->audioTracks) {
                audTrack->processTime();
            }
            return;
        }

        int keyframe = state.currentFrame - clip->startFrame;
        clip->m_properties.setKeyframe(id, keyframe, data);
        state.lastRenderedFrame = -1;
        for (auto audTrack : state.video->audioTracks) {
            audTrack->processTime();
        }
    };

    auto drawDimensions = [&]() {
        Dimensions dimensions = Dimensions::fromString(data);

        bool w = ImGui::DragInt("Width", &dimensions.size.x);
        bool h = ImGui::DragInt("Height", &dimensions.size.y);

        bool x = ImGui::DragInt("X", &dimensions.transform.position.x);
        bool y = ImGui::DragInt("Y", &dimensions.transform.position.y);
        bool rotation = ImGui::DragFloat("Rotation", &dimensions.transform.rotation);
        bool pitch = ImGui::DragFloat("Pitch", &dimensions.transform.pitch);
        bool roll = ImGui::DragFloat("Roll", &dimensions.transform.roll);
        bool anchorX = ImGui::DragFloat("Anchor X", &dimensions.transform.anchorPoint.x);
        bool anchorY = ImGui::DragFloat("Anchor Y", &dimensions.transform.anchorPoint.y);
        
        if (x || y || w || h || rotation || pitch || roll || anchorX || anchorY) {
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
            fmt::format("##{}", name).c_str(),
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
        if (ImGui::InputTextMultiline(fmt::format("##{}", id).c_str(), &text, ImVec2(0, 0), ImGuiInputTextFlags_WordWrap)) {
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

    auto drawTransform = [&]() {
        Transform transform = Transform::fromString(data);

        bool x = ImGui::DragInt("X", &transform.position.x);
        bool y = ImGui::DragInt("Y", &transform.position.y);
        bool rotation = ImGui::DragFloat("Rotation", &transform.rotation);
        bool pitch = ImGui::DragFloat("Pitch", &transform.pitch);
        bool roll = ImGui::DragFloat("Roll", &transform.roll);
        bool anchorX = ImGui::DragFloat("Anchor X", &transform.anchorPoint.x);
        bool anchorY = ImGui::DragFloat("Anchor Y", &transform.anchorPoint.y);

        if (x || y || rotation || pitch || roll || anchorX || anchorY) {
            setData(transform.toString());
        }
    };

    std::map<PropertyType, std::function<void()>> drawFuncs = {
        { PropertyType::Dimensions, drawDimensions },
        { PropertyType::Color, drawColorPicker },
        { PropertyType::Number, drawInt },
        { PropertyType::Percent, drawInt },
        { PropertyType::Position, drawPosition },
        { PropertyType::Text, drawText },
        { PropertyType::Transform, drawTransform },
        { PropertyType::Dropdown, drawDropdown }
    };
    drawFuncs[type]();

    int keyframe = state.currentFrame - clip->startFrame;
    bool isKeyframed = keyframes.contains(keyframe);

    if (ImGui::Button(fmt::format("{}Keyframe {}", isKeyframed ? "Remove " : "", name).c_str())) {
        if (isKeyframed) {
            keyframes.erase(keyframe);

            if (keyframeInfo.contains(keyframe)) {
                keyframeInfo.erase(keyframe);
            }
        } else if (!isKeyframed && keyframe >= 0) {
            clip->m_properties.setKeyframe(id, keyframe, data);
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

void ClipProperties::addProperty(ClipProperty* property) {
    property->clip = clip;
    property->data = property->keyframes[0];
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
