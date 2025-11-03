#pragma once

#include "../action.hpp"
#include <string>
#include <clips/clip.hpp>

class MoveClip : public Action {
protected:
    std::vector<std::string> uIDs;
    int deltaFrame;

    std::vector<std::shared_ptr<Clip>> getClips();
public:
    MoveClip(std::vector<std::string> uIDs, int deltaFrame);

    void perform() override;
    void undo() override;
};
