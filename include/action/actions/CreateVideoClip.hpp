#pragma once

#include "../action.hpp"
#include <clips/clip.hpp>
#include <video.hpp>

class CreateVideoClip : public Action {
protected:
    ExtClipMetadata metadata;
    int frame;
    int trackIdx;

    std::string videoUID;
    std::string audioUID;
public:
    CreateVideoClip(ExtClipMetadata metadata, int frame, int trackIdx): metadata(metadata), frame(frame), trackIdx(trackIdx) {}

    void perform() override;
    void undo() override;
};
