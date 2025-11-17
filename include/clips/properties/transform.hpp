#pragma once

#include "../clip.hpp"
#include <common.hpp>

class TransformProperty : public ClipProperty<Transform> {
public:
    TransformProperty() {
        keyframes[0] = {
            .position = {
                0, 0
            },
            .rotation = 0,
            .pitch = 0,
            .roll = 0,
        };
        data = keyframes[0];
        id = "transform";
        name = "Transform";
        type = PropertyType::Transform;
    }
    void drawProperty() override;
    void updateData(float progress, int previous, int next) override;
    void writeData(qn::HeapByteWriter& writer) override;
    void readData(qn::ByteReader& reader) override;
};
