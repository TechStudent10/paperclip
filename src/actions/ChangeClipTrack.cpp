#include "widgets.hpp"
#include <action/actions/ChangeClipTrack.hpp>

#include <state.hpp>
#include <clips/clip.hpp>

ChangeClipTrack::ChangeClipTrack(std::vector<std::string> uIDs, int deltaTrack, TrackType selectedType, bool isOnSameTracks):
        uIDs(uIDs), deltaTrack(deltaTrack), selectedType(selectedType), isOnSameTracks(isOnSameTracks) {
    fmt::println("added action");
}

std::vector<std::shared_ptr<Clip>> ChangeClipTrack::getClips() {
    auto& state = State::get();
    std::vector<std::shared_ptr<Clip>> clips;

    for (auto uID : uIDs) {
        int trackIdx = state.video->getClipMap()[uID];
        if (trackIdx < 0) {
            trackIdx = -(trackIdx + 1);
            clips.push_back(state.video->audioTracks[trackIdx]->getClip(uID));
            continue;
        }
        clips.push_back(state.video->videoTracks[trackIdx]->getClip(uID));
    }

    return clips;
}

void ChangeClipTrack::moveByTrack(int delta) {
    auto& state = State::get();
    
    for (auto selectedClip : getClips()) {
        int trackIdx = state.video->getClipMap()[selectedClip->uID];

        TrackType clipType = TrackType::Video;
        int targetTrack = trackIdx + (isOnSameTracks ? delta : (selectedType == TrackType::Video ? delta : -delta));
        if (trackIdx < 0) {
            targetTrack = -(trackIdx + 1) + (isOnSameTracks ? delta : (selectedType == TrackType::Audio ? delta : -delta));
            clipType = TrackType::Audio;
        }
        targetTrack = std::max(targetTrack, 0);
        if (targetTrack < 0) continue;
        if (clipType == TrackType::Audio) {
            state.video->removeAudioClip(trackIdx, std::static_pointer_cast<AudioClip>(selectedClip));
            state.video->addAudioClip(targetTrack, std::static_pointer_cast<AudioClip>(selectedClip));
        } else {
            state.video->removeClip(trackIdx, selectedClip);
            state.video->addClip(targetTrack, selectedClip);
        }
        // make sure the clip is still selected
        state.selectClip(selectedClip);
    }
}

void ChangeClipTrack::perform() {
    moveByTrack(deltaTrack);
}

void ChangeClipTrack::undo() {
    moveByTrack(-deltaTrack);
}
