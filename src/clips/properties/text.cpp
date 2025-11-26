#include <clips/properties/text.hpp>
#include <imgui.h>

#include <state.hpp>
#include <utils.hpp>

#include <misc/cpp/imgui_stdlib.h>

void TextProperty::drawProperty() {
    std::string text = data;
    if (ImGui::InputTextMultiline(fmt::format("##{}", id).c_str(), &text, ImVec2(0, 0), ImGuiInputTextFlags_WordWrap)) {
        setData(text, State::get().currentFrame - clip->startFrame);
    }
}

// TODO: figure out interpolating this lol
void TextProperty::updateData(float progress, int previousKeyframe, int nextKeyframe) {
    data = keyframes[previousKeyframe];
}

void TextProperty::writeData(qn::HeapByteWriter& writer) {
    for (auto [frame, value] : keyframes) {
        writer.writeI64(frame);
        UNWRAP_WITH_ERR(writer.writeStringVar(value));
    }
}

void TextProperty::readData(qn::ByteReader& reader) {
    int size = reader.readI64().unwrapOr(0);
    for (int i = 0; i < size; i++) {
        int frame = reader.readI64().unwrapOr(0);
        std::string value = reader.readStringVar().unwrapOr("FAILED TO READ FROM SAVE; YOU SHOULD NOT SEE THIS!!");
        keyframes[frame] = debug(value);
    }
}
