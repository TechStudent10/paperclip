#pragma once

#include "../action.hpp"
#include <string>
#include <clips/clip.hpp>

enum class TrackType;

class ChangeClipTrack : public Action {
protected:
    std::vector<std::string> uIDs;
    int deltaTrack;
    bool isOnSameTracks;
    TrackType selectedType;

    std::vector<std::shared_ptr<Clip>> getClips();

    void moveByTrack(int delta);
public:
    ChangeClipTrack(std::vector<std::string> uIDs, int deltaTrack, TrackType selectedType, bool isOnSameTracks);

    void perform() override;
    void undo() override;
};
