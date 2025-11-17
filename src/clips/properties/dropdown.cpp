#include <clips/properties/dropdown.hpp>
#include <imgui.h>

#include <state.hpp>
#include <utils.hpp>

void DropdownProperty::drawProperty() {
    if (ImGui::BeginCombo(name.c_str(), data.c_str())) {
        for (auto option : options) {
            bool selected = option == data;
            auto optionStr = std::string(option);
            if (ImGui::Selectable(optionStr.c_str(), selected)) {
                setData(optionStr, State::get().currentFrame - clip->startFrame);
            }

            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }
}


// dropdowns are the one property where all data is discrete
// and not continuous, therefore we probably shouldn't try to interpolate this
// lol
void DropdownProperty::updateData(float progress, int previousKeyframe, int nextKeyframe) {
    std::string previous = keyframes[previousKeyframe];
    data = previous;
}

void DropdownProperty::writeData(qn::HeapByteWriter& writer) {
    for (auto [frame, value] : keyframes) {
        writer.writeI64(frame);
        UNWRAP_WITH_ERR(writer.writeStringVar(value));
    }

    writer.writeI64(debug(options.size()));
    for (auto option : options) {
        UNWRAP_WITH_ERR(writer.writeStringVar(option));
    }
}

void DropdownProperty::readData(qn::ByteReader& reader) {
    int size = reader.readI64().unwrapOr(0);
    for (int i = 0; i < size; i++) {
        int frame = reader.readI64().unwrapOr(0);
        std::string value = reader.readStringVar().unwrapOr("");
        keyframes[frame] = value;
    }

    int optionsSize = reader.readI64().unwrapOr(0);
    options.resize(optionsSize);
    for (int j = 0; j < debug(optionsSize); j++) {
        options[j] = debug(reader.readStringVar().unwrapOr(""));
    }
}
