#include <track/video.hpp>

#include <functional>

void VideoTrack::render(Frame* frame, int targetFrame) {
    for (auto _clip : clips) {
        auto clip = _clip.second;
        if (targetFrame >= clip->startFrame && targetFrame <= clip->startFrame + clip->duration) {
            // process keyframes
            for (auto [id, property] : clip->m_properties) {
                property->processKeyframe(targetFrame);
            }
            clip->render(frame);
        }
    }
}
