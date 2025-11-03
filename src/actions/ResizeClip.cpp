#include <action/actions/ResizeClip.hpp>

#include <state.hpp>
#include <clips/clip.hpp>

void ResizeClip::perform() {
    auto& state = State::get();
    
    clip->startFrame = newStartFrame;
    clip->duration = newDuration;
}

void ResizeClip::undo() {
    auto& state = State::get();

    clip->startFrame = initialStartFrame;
    clip->duration = initialDuration;
}
