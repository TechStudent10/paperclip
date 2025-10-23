#include <video.hpp>

#include <state.hpp>

void Video::addClip(int trackIdx, std::shared_ptr<Clip> clip) {
    getTracks()[trackIdx]->addClip(clip);
    clipMap[clip->uID] = trackIdx;
    recalculateFrameCount();
}

void Video::addAudioClip(int trackIdx, std::shared_ptr<AudioClip> clip) {
    audioTracks[trackIdx]->addClip(clip);
    clipMap[clip->uID] = -trackIdx;
    recalculateFrameCount();
}

void Video::removeClip(int trackIdx, std::shared_ptr<Clip> clip) {
    getTracks()[trackIdx]->removeClip(clip);
    clipMap.erase(clip->uID);
    recalculateFrameCount();
}

void Video::removeAudioClip(int trackIdx, std::shared_ptr<AudioClip> clip) {
    audioTracks[trackIdx]->removeClip(clip);
    clipMap.erase(clip->uID);
    recalculateFrameCount();
}

Frame* Video::renderAtFrame(int frameNum) {
    auto frame = std::make_shared<Frame>(resolution.x, resolution.y);
    renderIntoFrame(frameNum, frame);
    return frame.get();
}

void Video::renderIntoFrame(int frameNum, std::shared_ptr<Frame> frame) {
    for (auto track : videoTracks) {
        track->render(frame.get(), frameNum);
    }
}

void Video::render(VideoRenderer* renderer) {
    recalculateFrameCount();
    auto frame = std::make_shared<Frame>(resolution.x, resolution.y);
    auto& state = State::get();
    for (int currentFrame = 0; currentFrame < frameCount; currentFrame++) {
        state.currentFrame = currentFrame;
        frame->clearFrame();
        renderIntoFrame(currentFrame, frame);
        renderer->addFrame(frame);
    }
}

void Video::recalculateFrameCount() {
    int currentFrameCount = 0;
    for (auto track : videoTracks) {
        auto clips = track->getClips();
        for (auto _clip : clips) {
            auto clip = _clip.second;
            auto lastFrame = clip->startFrame + clip->duration;
            if (lastFrame > currentFrameCount) {
                currentFrameCount = lastFrame;
            }
        }
    }
    frameCount = currentFrameCount;
}

int Video::frameForTime(float time) {
    return std::floor(time * (float)getFPS());
}

float Video::timeForFrame(int frame) {
    return (float)frame / (float)getFPS();
}
