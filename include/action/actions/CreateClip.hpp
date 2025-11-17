#pragma once

#include "../action.hpp"
#include <memory>
#include <clips/clip.hpp>

class CreateClip : public Action {
protected:
    std::shared_ptr<Clip> clip;
    ClipType type;
    int trackIdx;
public:
    CreateClip(std::shared_ptr<Clip> clip, ClipType type, int trackIdx): clip(clip), type(type), trackIdx(trackIdx) {}

    void perform() override;
    void undo() override;
};
