#include <action/actions/CreateClip.hpp>

#include <state.hpp>
#include <clips/clip.hpp>

void CreateClip::perform() {
    auto& state = State::get();
    state.deselect();
    switch (type) {
        case ClipType::Audio:
            state.video->addAudioClip(trackIdx, std::static_pointer_cast<AudioClip>(clip));
            break;
        default:
            state.video->addClip(trackIdx, clip);
            break;
    }

    state.selectClip(clip);
}

void CreateClip::undo() {
    auto& state = State::get();

    state.deselect(clip->uID);

    switch (type) {
        case ClipType::Audio:
            state.video->removeAudioClip(trackIdx, std::static_pointer_cast<AudioClip>(clip));
            break;
        default:
            state.video->removeClip(trackIdx, clip);
            fmt::println("removed!");
            break;
    }
}
