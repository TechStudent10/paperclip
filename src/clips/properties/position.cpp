#include <clips/properties/position.hpp>
#include <imgui.h>

#include <state.hpp>
#include <utils.hpp>

void PositionProperty::drawProperty() {
    Vector2D position = data;
    bool x = ImGui::DragInt("X", &position.x);
    bool y = ImGui::DragInt("Y", &position.y);

    if (x || y) {
        setData(position, State::get().currentFrame - clip->startFrame);
    }
}

void PositionProperty::updateData(float progress, int previousKeyframe, int nextKeyframe) {
    Vector2D previous = keyframes[previousKeyframe];
    Vector2D next = keyframes[nextKeyframe];
    data = Vector2D{
        .x = static_cast<int>(utils::interpolate(progress, previous.x, next.x)),
        .y = static_cast<int>(utils::interpolate(progress, previous.y, next.y)),
    };
}

void PositionProperty::writeData(qn::HeapByteWriter& writer) {
    for (auto [frame, value] : keyframes) {
        writer.writeI64(frame);
        writer.writeI64(value.x);
        writer.writeI64(value.y);
    }
}

void PositionProperty::readData(qn::ByteReader& reader) {
    int size = reader.readI64().unwrapOr(0);
    for (int i = 0; i < size; i++) {
        int frame = reader.readI64().unwrapOr(0);
        int x = reader.readI64().unwrapOr(0.f);
        int y = reader.readI64().unwrapOr(0.f);
        keyframes[frame] = Vector2D{
            x, y
        };
    }
}
