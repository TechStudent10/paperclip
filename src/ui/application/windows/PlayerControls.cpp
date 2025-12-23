#include <Application.hpp>
#include <state.hpp>

#include <icons/IconsFontAwesome6.h>
#include <fmt/format.h>
#include <fmt/chrono.h>

void Application::drawPlayerControls(ImVec2 imageSize) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    auto& state = State::get();

    // media control bar
    float mediaControlW = imageSize.x;
    float mediaControlH = 15.f;

    float progress = std::max(
        (state.currentFrame * 1.f) / (state.video->frameCount * 1.f),
        1.f / 30.f
    );

    progress = std::clamp(progress, 0.f, 1.f);

    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - mediaControlW) / 2.f);
    ImVec2 mediaControlPos = ImGui::GetCursorScreenPos();

    float rounding = 30.f;

    // background
    drawList->AddRectFilled(
        mediaControlPos,
        ImVec2(mediaControlPos.x + mediaControlW, mediaControlPos.y + mediaControlH),
        ImGui::GetColorU32(ImGuiCol_FrameBg),
        rounding
    );
    
    // fill
    drawList->AddRectFilled(
        mediaControlPos,
        ImVec2(mediaControlPos.x + mediaControlW * progress, mediaControlPos.y + mediaControlH),
        IM_COL32(255, 100, 100, 255),
        rounding
    );

    float circleRad = mediaControlH / 2.f;

    // circle drag thingy idk
    drawList->AddCircleFilled(
        ImVec2(
            mediaControlPos.x + mediaControlW * progress - circleRad,
            mediaControlPos.y + mediaControlH - circleRad
        ),
        circleRad,
        IM_COL32(255, 150, 150, 255)
    );

    ImGui::SetCursorScreenPos(mediaControlPos);
    ImGui::InvisibleButton("##media-control", ImVec2(mediaControlW, mediaControlH));

    if (ImGui::IsItemActive()) {
        float mouseX = ImGui::GetIO().MousePos.x;
        float newProgress = std::clamp((mouseX - mediaControlPos.x) / mediaControlW, 0.0f, 1.0f);
        setCurrentFrame(state.video->frameCount * newProgress);   
    }

    // playback buttons

    ImVec2 buttonSize = ImVec2(
        ImGui::GetFontSize() * 2.f,
        ImGui::GetFontSize() * 2.f
    );

    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float windowWidth = ImGui::GetContentRegionAvail().x;

    float totalPlaybackBtnWidth = buttonSize.x * 3 + spacing * 2;
    // UR = undo/redo
    float totalURBtnWidth = buttonSize.x * 3 + spacing * 2;

    float playbackOffset = (windowWidth - totalPlaybackBtnWidth) * 0.5f;
    auto btnStart = ImGui::GetCursorPosX();
    float playbackBtnStart = btnStart + playbackOffset;
    if (playbackOffset > 0.0f) {
        ImGui::SetCursorPosX(playbackBtnStart);
    }

    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 150));

    // ok so these param names are a bit misleading (aside from label thats obvious)
    // disabled = button does absolutely nothing; gets greyed out
    // enabled = on state, used for a toggle; button gets slightly more illuminated
    auto playbackBtn = [&](const char* label, bool disabled = false, bool enabled = false) {
        auto min = ImGui::GetCursorScreenPos();
        bool hovered = ImGui::IsMouseHoveringRect(min, ImVec2(min.x + buttonSize.x, min.y + buttonSize.y));
        int popCount = 0;
        
        if (disabled) {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 75));
            popCount++;
        }

        if (enabled) {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 225));
            popCount++;
        }

        if (hovered && !disabled) {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
            popCount++;
        }
        
        auto clicked = ImGui::Button(
            label,
            buttonSize
        ) && !disabled;

        ImGui::PopStyleColor(popCount);

        return clicked;
    };

    if (playbackBtn(ICON_FA_BACKWARD)) {
        setCurrentFrame(state.currentFrame - state.video->frameForTime(5));
    }
    
    ImGui::SameLine();

    if (playbackBtn(state.isPlaying ? ICON_FA_PAUSE : ICON_FA_PLAY)) {
        togglePlay();
    }

    ImGui::SameLine();

    if (playbackBtn(ICON_FA_FORWARD)) {
        setCurrentFrame(state.currentFrame + state.video->frameForTime(5));
    }

    ImGui::SameLine();

    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - mediaControlW) / 2.f + mediaControlW - totalURBtnWidth);
        
    if (playbackBtn(ICON_FA_ROTATE_LEFT, state.undoStack.size() <= 0)) {
        state.undo();
    }

    ImGui::SameLine();

    if (playbackBtn(ICON_FA_ROTATE_RIGHT, state.redoStack.size() <= 0)) {
        state.redo();
    }

    ImGui::SameLine();

    bool areClipsLinked = state.areClipsLinked();
    if (playbackBtn(areClipsLinked ? ICON_FA_LINK_SLASH : ICON_FA_LINK, state.selectedClips.size() <= 1, areClipsLinked)) {
        for (auto selectedClip : state.getSelectedClips()) {
            if (areClipsLinked) {
                selectedClip.second->linkedClips = {};
            } else {
                selectedClip.second->linkedClips = state.selectedClips;
            }
        }
    }

    ImGui::PopStyleColor(4);

    ImGui::SameLine();

    int duration = state.video->timeForFrame(state.video->frameCount);
    int currentTime = std::clamp(state.video->timeForFrame(state.currentFrame), 0.f, duration * 1.f);

    ImGui::PushFont(progressFont, 22.f);

    auto currentTimeS = std::chrono::seconds(currentTime);
    auto currentTimeText =
        duration >= 3600 ?
            fmt::format("{:%H:%M:%S}", currentTimeS) :
            fmt::format("{:%M:%S}", currentTimeS);
    
    auto durationS = std::chrono::seconds(duration);
    auto durationText =
        duration >= 3600 ?
            fmt::format("/ {:%H:%M:%S}", durationS) :
            fmt::format("/ {:%M:%S}", durationS);
    
    auto currentTimeTextSize = ImGui::CalcTextSize(currentTimeText.c_str()).x;
    auto durationTextSize = ImGui::CalcTextSize(durationText.c_str()).x;

    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - mediaControlW) / 2.f);
    
    ImGui::Text("%s", currentTimeText.c_str());

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 150, 255));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(-25, 0));
 
    ImGui::Text("%s", durationText.c_str());

    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    ImGui::PopFont();

    ImGui::End();
}
