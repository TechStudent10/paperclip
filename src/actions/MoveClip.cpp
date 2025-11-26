#include <action/actions/MoveClip.hpp>

#include <state.hpp>
#include <clips/clip.hpp>

#include <utils.hpp>

MoveClip::MoveClip(std::vector<std::string> uIDs, int deltaFrame):
        uIDs(uIDs), deltaFrame(deltaFrame) {
    fmt::println("added action");
}

std::vector<std::shared_ptr<Clip>> MoveClip::getClips() {
    auto& state = State::get();
    std::vector<std::shared_ptr<Clip>> clips;

    for (auto uID : uIDs) {
        int trackIdx = state.video->getClipMap()[uID];
        if (trackIdx < 0) {
            trackIdx = -(trackIdx + 1);
            auto clip = state.video->audioTracks[trackIdx]->getClip(uID);
            if (!utils::vectorContains(clips, std::static_pointer_cast<Clip>(clip))) {
                clips.push_back(clip);
            }
            continue;
        }
        auto clip = state.video->videoTracks[trackIdx]->getClip(uID);
        if (!utils::vectorContains(clips, clip)) {
            clips.push_back(clip);
        }
    }

    return clips;
}

void MoveClip::perform() {
    auto& state = State::get();
    
    for (auto clip : getClips()) {
        clip->startFrame += deltaFrame;
    }
}

void MoveClip::undo() {
    auto& state = State::get();
    
    for (auto clip : getClips()) {
        fmt::println("{} -> {}", clip->startFrame, deltaFrame);
        clip->startFrame -= deltaFrame;
    }
}
