#pragma once

#include "../action.hpp"
#include <memory>
#include <clips/clip.hpp>

class ResizeClip : public Action {
protected:
    std::shared_ptr<Clip> clip;

    int initialStartFrame = 0;
    int initialDuration = 0;

    int newStartFrame = 0;
    int newDuration = 0;
public:
    ResizeClip(std::shared_ptr<Clip> clip, int initialStartFrame, int initialDuration):
        clip(clip), initialStartFrame(initialStartFrame), initialDuration(initialDuration) {
        newStartFrame = clip->startFrame;
        newDuration = clip->duration;
    }

    void perform() override;
    void undo() override;
};
