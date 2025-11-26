#include <clips/properties/color.hpp>
#include <imgui.h>

#include <state.hpp>
#include <utils.hpp>

void ColorProperty::drawProperty() {
    RGBAColor color = data;
    float colorBuf[4] = { (float)color.r / 255, (float)color.g / 255, (float)color.b / 255, (float)color.a / 255 };
    if (ImGui::ColorPicker4("Color", colorBuf)) {
        color.r = std::round(colorBuf[0] * 255);
        color.g = std::round(colorBuf[1] * 255);
        color.b = std::round(colorBuf[2] * 255);
        color.a = std::round(colorBuf[3] * 255);

        setData(color, State::get().currentFrame - clip->startFrame);
    }
}

void ColorProperty::updateData(float progress, int previousKeyframe, int nextKeyframe) {
    RGBAColor previous = keyframes[previousKeyframe];
    RGBAColor next = keyframes[nextKeyframe];
    data = RGBAColor{
        .r = static_cast<int>(utils::interpolate(progress, previous.r, next.r)),
        .g = static_cast<int>(utils::interpolate(progress, previous.g, next.g)),
        .b = static_cast<int>(utils::interpolate(progress, previous.b, next.b)),
    };
}

void ColorProperty::writeData(qn::HeapByteWriter& writer) {
    for (auto [frame, value] : keyframes) {
        writer.writeI64(frame);
        writer.writeI64(value.r);
        writer.writeI64(value.g);
        writer.writeI64(value.b);
    }
}

void ColorProperty::readData(qn::ByteReader& reader) {
    int size = reader.readI64().unwrapOr(0);
    for (int i = 0; i < size; i++) {
        int frame = reader.readI64().unwrapOr(0);
        int r = reader.readI64().unwrapOr(0.f);
        int g = reader.readI64().unwrapOr(0.f);
        int b = reader.readI64().unwrapOr(0.f);
        keyframes[frame] = RGBAColor{
            .r = r,
            .g = g,
            .b = b
        };
    }
}
