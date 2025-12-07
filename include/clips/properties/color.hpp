#pragma once

#include "../clip.hpp"
#include <common.hpp>

class ColorProperty : public ClipProperty<RGBAColor> {
public:
    ColorProperty() {
        keyframes[0] = { .r = 255, .g = 255, .b = 255, .a = 255 };
        data = keyframes[0];
        id = "color";
        name = "Color";
        type = PropertyType::Color;
    }
    void drawProperty() override;
    void updateData(float progress, int previous, int next) override;
    void writeData(qn::HeapByteWriter& writer) override;
    void readData(qn::ByteReader& reader) override;
};
