#pragma once

#include "../clip.hpp"
#include <common.hpp>

class NumberProperty : public ClipProperty<float> {
protected:
    float step = -1.f;
    float min = -1.f;
    float max = -1.f;
public:
    // this needs to take things in
    // but ill worry about that later
    NumberProperty(float defaultNum = 100.f, float step = -1.f, float min = -1.f, float max = -1.f):
        step(step), min(min), max(max) {
        keyframes[0] = defaultNum;
        data = keyframes[0];
        type = PropertyType::Number;
    }
    void drawProperty() override;
    void updateData(float progress, int previous, int next) override;
    void writeData(qn::HeapByteWriter& writer) override;
    void readData(qn::ByteReader& reader) override;
};
