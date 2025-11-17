#pragma once

#include "../clip.hpp"
#include <common.hpp>

class PositionProperty : public ClipProperty<Vector2D> {
public:
    PositionProperty() {
        keyframes[0] = { .x = 0, .y = 0 };
        id = "position";
        name = "Position";
        data = keyframes[0];
        type = PropertyType::Position;
    }
    void drawProperty() override;
    void updateData(float progress, int previous, int next) override;
    void writeData(qn::HeapByteWriter& writer) override;
    void readData(qn::ByteReader& reader) override;
};
