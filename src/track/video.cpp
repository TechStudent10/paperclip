#include <track/video.hpp>

#include <utils.hpp>

void VideoTrack::render(Frame* frame, int targetFrame) {
    for (auto _clip : clips) {
        auto clip = _clip.second;
        if (targetFrame >= clip->startFrame && targetFrame <= clip->startFrame + clip->duration) {
            // process keyframes
            for (auto [id, property] : clip->m_properties) {
                property->processKeyframe(targetFrame);
            }

            int relativeFrame = targetFrame - clip->startFrame;
            clip->opacity = 1;
            if (relativeFrame < clip->fadeInFrame) {
                clip->opacity = utils::interpolate(relativeFrame * 1.f / clip->fadeInFrame, 0, 1);
            }

            int fadeOutStart = clip->duration - clip->fadeOutFrame;
            if (relativeFrame >= fadeOutStart) {
                clip->opacity = utils::interpolate((relativeFrame - fadeOutStart) * 1.f / clip->fadeOutFrame, 1, 0);
            }

            clip->render(frame);
        }
    }
}
