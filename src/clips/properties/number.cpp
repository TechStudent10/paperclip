#include <clips/properties/number.hpp>
#include <imgui.h>

#include <state.hpp>
#include <utils.hpp>

void NumberProperty::drawProperty() {
    float number = data;
    if (ImGui::DragFloat(
        fmt::format("##{}", name).c_str(),
        &number,
        step == -1.f ? 1.0f : step,
        min == -1.f ? 0.f : min,
        max == -1.f ? 0.f : max
    )) {
        setData(number, State::get().currentFrame - clip->startFrame);
    }
}

void NumberProperty::updateData(float progress, int previousKeyframe, int nextKeyframe) {
    float previous = keyframes[previousKeyframe];
    float next = keyframes[nextKeyframe];
    data = utils::interpolate(progress, previous, next);
}

void NumberProperty::writeData(qn::HeapByteWriter& writer) {
    for (auto [frame, value] : keyframes) {
        writer.writeI64(frame);
        writer.writeFloat(value);
    }
}

void NumberProperty::readData(qn::ByteReader& reader) {
    int size = reader.readI64().unwrapOr(0);
    for (int i = 0; i < size; i++) {
        int frame = reader.readI64().unwrapOr(0);
        float value = reader.readFloat().unwrapOr(0.f);
        keyframes[frame] = value;
    }
}
