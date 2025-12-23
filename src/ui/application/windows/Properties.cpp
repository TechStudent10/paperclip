#include <Application.hpp>
#include <state.hpp>

void Application::drawPropertiesWindow() {
    auto& state = State::get();

    ImGui::SetNextWindowClass(&bareWindowClass);
    ImGui::Begin("Properties");
    if (state.areClipsSelected()) {
        if (ImGui::Button("Open Keyframe Editor", ImVec2(-1.0f, 0.0f))) {
            ImGui::OpenPopup("Keyframe Editor");

        }
        if (ImGui::BeginPopup("Keyframe Editor")) {
            ImGui::Text("boop");
            // for (auto prop : selectedClip->m_properties) {
            //     ImGui::Text("%s", prop.second->name.c_str());
            // }
            ImGui::EndPopup();
        }
        ImGui::Separator();
        for (auto [clipID, selectedClip] : state.getSelectedClips()) {
            ImGui::SeparatorText(selectedClip->m_metadata.name.c_str());

            ImGui::SeparatorText("Properties");

            for (auto prop : selectedClip->m_properties) {
                ImGui::SeparatorText(prop.second->name.c_str());
                prop.second->_drawProperty();
            }

            if (ImGui::Button("Delete")) {
                ImGui::OpenPopup("Delete confirmation");
            }
            if (ImGui::BeginPopupModal("Delete confirmation")) {
                ImGui::Text("Are you sure you want to delete this clip?");
                ImGui::Separator();
                if (ImGui::Button("Yes")) {
                    int trackIdx = state.video->getClipMap()[selectedClip->uID];
                    if (timeline.selectedTrackType == TrackType::Audio) {
                        state.video->removeAudioClip(trackIdx, std::static_pointer_cast<AudioClip>(selectedClip));
                    } else {
                        state.video->removeClip(trackIdx, selectedClip);
                    }
                    selectedClip->onDelete();
                    state.deselect();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("No")) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
    }
    ImGui::End();
}
