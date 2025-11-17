#pragma once

#include "../clip.hpp"
#include <common.hpp>

class DimensionsProperty : public ClipProperty<Dimensions> {
public:
    DimensionsProperty() {
        keyframes[0] = {
            .size = { 50, 50 },
            .transform = {
                .position = {
                    0, 0
                },
                .rotation = 0,
                .pitch = 0,
                .roll = 0,
            }
        };
        data = keyframes[0];
        id = "dimensions";
        name = "Dimensions";
        type = PropertyType::Dimensions;
    }
    void drawProperty() override;
    void updateData(float progress, int previous, int next) override;
    void writeData(qn::HeapByteWriter& writer) override;
    void readData(qn::ByteReader& reader) override;
};
