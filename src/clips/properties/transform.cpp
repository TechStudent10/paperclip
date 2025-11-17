#include <clips/properties/transform.hpp>
#include <imgui.h>

#include <state.hpp>
#include <utils.hpp>

void TransformProperty::drawProperty() {
    Transform transform = data;
    bool x = ImGui::DragInt("X", &transform.position.x);
    bool y = ImGui::DragInt("Y", &transform.position.y);
    bool rotation = ImGui::DragFloat("Rotation", &transform.rotation);
    bool pitch = ImGui::DragFloat("Pitch", &transform.pitch);
    bool roll = ImGui::DragFloat("Roll", &transform.roll);
    bool anchorX = ImGui::DragFloat("Anchor X", &transform.anchorPoint.x);
    bool anchorY = ImGui::DragFloat("Anchor Y", &transform.anchorPoint.y);

    if (x || y || rotation || pitch || roll || anchorX || anchorY) {
        setData(transform, State::get().currentFrame - clip->startFrame);
    }
}

void TransformProperty::updateData(float progress, int previousKeyframe, int nextKeyframe) {
    Transform previous = keyframes[previousKeyframe];
    Transform next = keyframes[nextKeyframe];
    data = Transform{
        .position = {
            .x = static_cast<int>(utils::interpolate(progress, previous.position.x, next.position.x)),
            .y = static_cast<int>(utils::interpolate(progress, previous.position.y, next.position.y))
        },
        .anchorPoint = {
            .x = utils::interpolate(progress, previous.anchorPoint.x, next.anchorPoint.x),
            .y = utils::interpolate(progress, previous.anchorPoint.y, next.anchorPoint.y)
        },
        .rotation = utils::interpolate(progress, previous.rotation, next.rotation),
        .pitch = utils::interpolate(progress, previous.pitch, next.pitch),
        .roll = utils::interpolate(progress, previous.roll, next.roll),
    };
}

void TransformProperty::writeData(qn::HeapByteWriter& writer) {
    for (auto [frame, value] : keyframes) {
        writer.writeI64(frame);
        writer.writeI64(value.position.x);
        writer.writeI64(value.position.y);
        writer.writeFloat(value.anchorPoint.x);
        writer.writeFloat(value.anchorPoint.y);
        writer.writeFloat(value.rotation);
        writer.writeFloat(value.pitch);
        writer.writeFloat(value.roll);
    }
}

void TransformProperty::readData(qn::ByteReader& reader) {
    int size = reader.readI64().unwrapOr(0);
    for (int i = 0; i < size; i++) {
        int frame = reader.readI64().unwrapOr(0);
        int posX = reader.readI64().unwrapOr(0);
        int posY = reader.readI64().unwrapOr(0);
        float ancX = reader.readFloat().unwrapOr(0.f);
        float ancY = reader.readFloat().unwrapOr(0.f);
        float rotation = reader.readFloat().unwrapOr(0.f);
        float pitch = reader.readFloat().unwrapOr(0.f);
        float roll = reader.readFloat().unwrapOr(0.f);
        keyframes[frame] = Transform{
            .position = {
                .x = posX,
                .y = posY
            },
            .anchorPoint = {
                .x = ancX,
                .y = ancY
            },
            .rotation = rotation,
            .pitch = pitch,
            .roll = roll
        };
    }
}
