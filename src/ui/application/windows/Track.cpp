#include <Application.hpp>

void Application::drawTrackWindow() {
    ImGui::SetNextWindowClass(&bareWindowClass);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Track", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    timeline.render();
    ImGui::End();
    ImGui::PopStyleVar();
}
