#include <clips/properties/dimensions.hpp>
#include <imgui.h>

#include <state.hpp>
#include <utils.hpp>

void DimensionsProperty::drawProperty() {
    Dimensions dimensions = data;
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
        setData(dimensions, State::get().currentFrame - clip->startFrame);
    }
}

void DimensionsProperty::updateData(float progress, int previousKeyframe, int nextKeyframe) {
    Dimensions previous = keyframes[previousKeyframe];
    Dimensions next = keyframes[nextKeyframe];
    data = Dimensions{
        .size = {
            .x = static_cast<int>(utils::interpolate(progress, previous.size.x, previous.size.x)),
            .y = static_cast<int>(utils::interpolate(progress, previous.size.y, previous.size.y))
        },
        .transform = {
            .position = {
                .x = static_cast<int>(utils::interpolate(progress, previous.transform.position.x, previous.transform.position.x)),
                .y = static_cast<int>(utils::interpolate(progress, previous.transform.position.y, previous.transform.position.y))
            },
            .anchorPoint = {
                .x = utils::interpolate(progress, previous.transform.anchorPoint.x, previous.transform.anchorPoint.x),
                .y = utils::interpolate(progress, previous.transform.anchorPoint.y, previous.transform.anchorPoint.y)
            },
            .rotation = utils::interpolate(progress, previous.transform.rotation, next.transform.rotation),
            .pitch = utils::interpolate(progress, previous.transform.pitch, next.transform.pitch),
            .roll = utils::interpolate(progress, previous.transform.roll, next.transform.roll),
        }
    };
}

void DimensionsProperty::writeData(qn::HeapByteWriter& writer) {
    for (auto [frame, value] : keyframes) {
        writer.writeI64(frame);
        writer.writeI64(value.size.x);
        writer.writeI64(value.size.y);
        writer.writeI64(value.transform.position.x);
        writer.writeI64(value.transform.position.y);
        writer.writeFloat(value.transform.anchorPoint.x);
        writer.writeFloat(value.transform.anchorPoint.y);
        writer.writeFloat(value.transform.rotation);
        writer.writeFloat(value.transform.pitch);
        writer.writeFloat(value.transform.roll);
    }
}

void DimensionsProperty::readData(qn::ByteReader& reader) {
    int size = reader.readI64().unwrapOr(0);
    for (int i = 0; i < size; i++) {
        int frame = reader.readI64().unwrapOr(0);
        int sizeX = reader.readI64().unwrapOr(0);
        int sizeY = reader.readI64().unwrapOr(0);
        int posX = reader.readI64().unwrapOr(0);
        int posY = reader.readI64().unwrapOr(0);
        float ancX = reader.readFloat().unwrapOr(0.f);
        float ancY = reader.readFloat().unwrapOr(0.f);
        float rotation = reader.readFloat().unwrapOr(0.f);
        float pitch = reader.readFloat().unwrapOr(0.f);
        float roll = reader.readFloat().unwrapOr(0.f);
        keyframes[frame] = Dimensions{
            .size = {
                .x = sizeX,
                .y = sizeY,
            },
            .transform = {
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
            }
        };
    }
}
