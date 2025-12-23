#include <Application.hpp>
#include <state.hpp>

void Application::drawPlayerWindow(ImVec2 imageSize, ImVec2 imagePos, float scale) {
    auto& state = State::get();

    ImGui::SetCursorPos(imagePos);
    ImGui::Image((ImTextureID)(uintptr_t)frame->textureID, imageSize);

    ImGuiIO& io = ImGui::GetIO();

    ImGui::SetCursorPos(imagePos);

    auto absMousePos = io.MousePos;
    auto windowPos = ImGui::GetWindowPos();
    auto mousePos = ImVec2(
        absMousePos.x - imagePos.x - windowPos.x,
        absMousePos.y - imagePos.y - windowPos.y
    );

    if (ImGui::InvisibleButton("btn", imageSize)) {
        // fmt::println("btn");
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    for (auto [clipID, selectedClip] : state.getSelectedClips()) {
        if (ImGui::IsItemHovered() && ImGui::IsMouseDown(0)) {
            // mouse position in the canvas
            // in terms of the video resolution
            auto canvasX = static_cast<int>(std::round(mousePos.x / scale));
            auto canvasY = static_cast<int>(std::round(mousePos.y / scale));
            
            bool deselected = false;

            if (isDraggingClip && io.MouseDownDuration[0] == 0) {
                // check if canvasX, canvasY are
                // outside of the clip bounding box
                // if so, deselect

                Vector2D position = selectedClip->getPos();
                Vector2D size = selectedClip->getSize();

                // top 10 laziest developer moments
                if (!(canvasX >= position.x && canvasX <= position.x + size.x &&
                    canvasY >= position.y && canvasY <= position.y + size.y)
                ) {
                    isDraggingClip = false;
                    state.deselect();    
                    deselected = true;   
                }
            }

            if (!deselected) {
                if (isDraggingClip) {
                    int dx = canvasX - initialPos.x;
                    int dy = canvasY - initialPos.y;

                    initialPos = { canvasX, canvasY };

                    if (selectedClip->m_properties.contains("position")) {
                        // TODO: REWORK THIS FOR TRANSFORM

                        // auto property = selectedClip->m_properties["position"];
                        // auto oldPos = Vector2D::fromString(property->data);
                        // Vector2D newPos = {
                        //     .x = std::clamp(oldPos.x + dx, 0, state.video->resolution.x),
                        //     .y = std::clamp(oldPos.y + dy, 0, state.video->resolution.y)
                        // };

                        // auto data = newPos.toString();
                        // if (property->keyframes.size() == 1) {
                        //     property->keyframes[0] = data;
                        //     state.lastRenderedFrame = -1;
                        // } else {
                        //     int keyframe = state.currentFrame - selectedClip->startFrame;
                        //     selectedClip->m_properties.setKeyframe(property->id, keyframe, data);
                        //     state.lastRenderedFrame = -1;
                        // }
                    }
                }

                if (!isDraggingClip) {
                    // find the clip currently being clicked on
                    // set state.draggingClip
                    // and initialPos

                    for (auto track : state.video->videoTracks) {
                        for (auto _clip : track->getClips()) {
                            auto clip = _clip.second;
                            Vector2D position = clip->getPos();
                            Vector2D size = clip->getSize();
                            // fmt::println("-------------------------");
                            // fmt::println("{}, {}", canvasX >= position.x, canvasX <= position.x + size.x);
                            // fmt::println("{}, {}", canvasY >= position.y, canvasY <= position.y + size.y);
                            // fmt::println("-------------------------");
                            // fmt::println("{}, {}", position.x, position.y);
                            // fmt::println("{}, {}", size.x, size.y);
                            // fmt::println("{}, {}", canvasX, canvasY);
                            // fmt::println("-------------------------");
                            if (canvasX >= position.x && canvasX <= position.x + size.x &&
                                canvasY >= position.y && canvasY <= position.y + size.y
                            ) {
                                fmt::println("found clip!");
                                selectedClip = clip;
                                initialPos = { canvasX, canvasY };
                                isDraggingClip = true;
                                break;
                            }
                        }

                        if (isDraggingClip) {
                            break;
                        }
                    }
                }
            }
        }

        Vector2D position = selectedClip->getPos();
        Vector2D size = selectedClip->getSize();
        Vector2D resolution = state.video->getResolution();
        drawList->AddRect({
            imagePos.x + windowPos.x + std::clamp(position.x, 0, resolution.x) * scale,
            imagePos.y + windowPos.y + std::clamp(position.y, 0, resolution.y) * scale
        }, {
            imagePos.x + windowPos.x + std::clamp(position.x + size.x, 0, resolution.x) * scale,
            imagePos.y + windowPos.y + std::clamp(position.y + size.y, 0, resolution.y) * scale
        }, ImColor(255, 0, 0, 127), 0.f, 0, 5.f);
    }
}
