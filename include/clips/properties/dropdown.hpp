#pragma once

#include "../clip.hpp"
#include <common.hpp>

class DropdownProperty : public ClipProperty<std::string> {
public:
    std::vector<std::string> options;

    // this needs to take things in
    // but i shall worry about that later
    DropdownProperty(std::vector<std::string> options = {}, const std::string& defaultStr = ""): options(options) {
        keyframes[0] = defaultStr;
        data = keyframes[0];
        type = PropertyType::Dropdown;
    }
    void drawProperty() override;
    void updateData(float progress, int previous, int next) override;
    void writeData(qn::HeapByteWriter& writer) override;
    void readData(qn::ByteReader& reader) override;
};
