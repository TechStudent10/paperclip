#include <video.hpp>
#include <renderer/text.hpp>
#include <Application.hpp>
#include <state.hpp>

#include <iostream>

#include <fmt/base.h>
#include <miniaudio.h>
#include <nfd.h>

int main() {
    if (mlt_factory_init("resources/mlt") == 0) {
        fmt::println("unable to init mlt factory");
    }
    std::cout << "hello!" << std::endl;
    NFD_Init();

    int width = 1920;
    int height = 1080;
    int fps = 60;

    auto video = std::make_shared<Video>();
    video->framerate = 60;
    video->resolution = {width, height};
    video->addTrack(std::make_shared<VideoTrack>());
    video->addTrack(std::make_shared<VideoTrack>());

    auto rectClip = std::make_shared<clips::Circle>();
    rectClip->m_properties.setKeyframe("position", 120, Vector2D{ .x = 500, .y = 1000}.toString());
    rectClip->m_properties.setKeyframe("position", 180, Vector2D{ .x = 200, .y = 800}.toString());
    rectClip->m_properties.setKeyframe("position", 240, Vector2D{ .x = 0, .y = 500}.toString());

    rectClip->m_properties.setKeyframeMeta("position", 120, {
        .easing = animation::Easing::Exponential,
        .mode = animation::EasingMode::EaseOut
    });

    rectClip->m_properties.setKeyframeMeta("position", 180, {
        .easing = animation::Easing::Exponential,
        .mode = animation::EasingMode::EaseIn
    });

    rectClip->m_properties.setKeyframeMeta("position", 240, {
        .easing = animation::Easing::Cubic,
        .mode = animation::EasingMode::EaseInOut
    });


    rectClip->m_properties.setKeyframe("color", 90, RGBAColor{ .r = 255, .g = 0, .b = 0 }.toString());
    rectClip->m_properties.setKeyframe("color", 150, RGBAColor{ .r = 0, .g = 255, .b = 0 }.toString());
    rectClip->m_properties.setKeyframe("color", 210, RGBAColor{ .r = 0, .g = 0, .b = 255 }.toString());

    rectClip->m_properties.setKeyframeMeta("color", 90, {
        .easing = animation::Easing::Backwards,
        .mode = animation::EasingMode::EaseInOut
    });

    rectClip->m_properties.setKeyframeMeta("color", 150, {
        .easing = animation::Easing::Circular,
        .mode = animation::EasingMode::EaseOut
    });

    rectClip->m_properties.setKeyframeMeta("color", 210, {
        .easing = animation::Easing::Exponential,
        .mode = animation::EasingMode::EaseOut
    });

    rectClip->m_properties.setKeyframe("radius", 60, Vector1D{ .number = 250 }.toString());
    rectClip->m_properties.setKeyframe("radius", 120, Vector1D{ .number = 600 }.toString());
    rectClip->m_properties.setKeyframe("radius", 180, Vector1D{ .number = 150 }.toString());

    rectClip->m_properties.setKeyframeMeta("radius", 60, {
        .easing = animation::Easing::Quadratic,
        .mode = animation::EasingMode::EaseInOut
    });

    rectClip->m_properties.setKeyframeMeta("radius", 120, {
        .easing = animation::Easing::Elastic,
        .mode = animation::EasingMode::EaseInOut
    });

    rectClip->m_properties.setKeyframeMeta("radius", 180, {
        .easing = animation::Easing::Sine,
        .mode = animation::EasingMode::EaseInOut
    });


    video->addClip(0, rectClip);

    video->audioTracks.push_back(std::make_shared<AudioTrack>());
    video->audioTracks.push_back(std::make_shared<AudioTrack>());

    auto& state = State::get();
    if (ma_engine_init(NULL, &state.soundEngine) != MA_SUCCESS) {
        fmt::println("could not init engine");
    }
    
    state.video = video;
    state.textRenderer = std::make_shared<TextRenderer>();

    Application app;
    app.setup();
    app.run();

    // cleanup

    ma_engine_uninit(&state.soundEngine);
    mlt_factory_close();

    return 0;
}
