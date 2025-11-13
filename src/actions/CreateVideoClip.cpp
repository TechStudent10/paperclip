#include <action/actions/CreateVideoClip.hpp>

#include <state.hpp>
#include <clips/clip.hpp>

void CreateVideoClip::perform() {
    auto& state = State::get();
    state.deselect();
    auto videoClip = std::make_shared<clips::VideoClip>(metadata.filePath);
    
    videoClip->startFrame = frame;
    videoClip->duration = metadata.frameCount;

    auto audioClip = std::make_shared<AudioClip>(fmt::format("{}.mp3", metadata.filePath));

    audioClip->startFrame = frame;
    audioClip->duration = metadata.frameCount;
    audioClip->m_metadata.name = videoClip->m_metadata.name;
    audioClip->m_properties.getProperties()["volume"]->data = Vector1D{ .number = 100 }.toString();

    videoClip->linkedClips = { videoClip->uID, audioClip->uID };
    audioClip->linkedClips = { videoClip->uID, audioClip->uID };

    state.video->addClip(trackIdx, videoClip);
    state.video->addAudioClip(trackIdx, audioClip);

    videoUID = videoClip->uID;
    audioUID = audioClip->uID;

    state.selectClip(videoClip);

    state.lastRenderedFrame = -1;
}

void CreateVideoClip::undo() {
    auto& state = State::get();

    state.deselect(videoUID);
    state.deselect(audioUID);

    state.video->removeClip(trackIdx, state.video->videoTracks[trackIdx]->getClip(videoUID));
    state.video->removeAudioClip(trackIdx, state.video->audioTracks[trackIdx]->getClip(audioUID));
}
