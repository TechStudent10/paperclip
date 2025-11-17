#pragma once

#include "../clip.hpp"
#include <common.hpp>

class TextProperty : public ClipProperty<std::string> {
public:
    // this needs to take things in
    TextProperty(const std::string& defaultVal = "") {
        keyframes[0] = defaultVal;
        id = "text";
        name = "Text";
        data = keyframes[0];
        type = PropertyType::Text;
    }
    void drawProperty() override;
    void updateData(float progress, int previous, int next) override;
    void writeData(qn::HeapByteWriter& writer) override;
    void readData(qn::ByteReader& reader) override;
};
