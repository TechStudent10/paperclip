#include <video.hpp>
#include <renderer/text.hpp>
#include <Application.hpp>
#include <state.hpp>

#include <iostream>

#include <fmt/base.h>
#include <miniaudio.h>
#include <nfd.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <clips/properties/color.hpp>
#include <clips/properties/number.hpp>
#include <clips/properties/transform.hpp>

int main() {
    if (mlt_factory_init("resources/mlt") == 0) {
        fmt::println("unable to init mlt factory");
    }
    std::cout << "hello!" << std::endl;
    NFD_Init();

    Application app;
    app.setup();

    int width = 1920;
    int height = 1080;
    int fps = 60;

    auto& state = State::get();
    
    auto video = std::make_shared<Video>();
    video->framerate = 60;
    video->resolution = {width, height};
    video->addTrack(std::make_shared<VideoTrack>());
    video->addTrack(std::make_shared<VideoTrack>());
    state.video = video;

    auto rectClip = std::make_shared<clips::Circle>();

    rectClip->getProperty<TransformProperty>("transform").unwrap()->keyframes[120] = Transform{ .position = { .x = 500, .y = 1000 }, .anchorPoint = { 0, 0 } };
    rectClip->getProperty<TransformProperty>("transform").unwrap()->keyframes[180] = Transform{ .position = { .x = 200, .y = 800 }, .anchorPoint = { 0, 0 } };
    rectClip->getProperty<TransformProperty>("transform").unwrap()->keyframes[240] = Transform{ .position = { .x = 0, .y = 500 }, .anchorPoint = { 0, 0 } };

    rectClip->m_properties["transform"]->keyframeInfo[120] = {
        .easing = animation::Easing::Exponential,
        .mode = animation::EasingMode::EaseOut
    };

    rectClip->m_properties["transform"]->keyframeInfo[180] = {
        .easing = animation::Easing::Exponential,
        .mode = animation::EasingMode::EaseIn
    };

    rectClip->m_properties["transform"]->keyframeInfo[240] = {
        .easing = animation::Easing::Cubic,
        .mode = animation::EasingMode::EaseInOut
    };

    rectClip->getProperty<ColorProperty>("color").unwrap()->keyframes[90] = RGBAColor{ .r = 255, .g = 0, .b = 0 };
    rectClip->getProperty<ColorProperty>("color").unwrap()->keyframes[150] = RGBAColor{ .r = 0, .g = 255, .b = 0 };
    rectClip->getProperty<ColorProperty>("color").unwrap()->keyframes[210] = RGBAColor{ .r = 0, .g = 0, .b = 255 };

    rectClip->m_properties["color"]->keyframeInfo[90] = {
        .easing = animation::Easing::Backwards,
        .mode = animation::EasingMode::EaseInOut
    };

    rectClip->m_properties["color"]->keyframeInfo[150] = {
        .easing = animation::Easing::Circular,
        .mode = animation::EasingMode::EaseOut
    };

    rectClip->m_properties["color"]->keyframeInfo[210] = {
        .easing = animation::Easing::Exponential,
        .mode = animation::EasingMode::EaseOut
    };

    rectClip->getProperty<NumberProperty>("radius").unwrap()->keyframes[60] = 250;
    rectClip->getProperty<NumberProperty>("radius").unwrap()->keyframes[120] = 600;
    rectClip->getProperty<NumberProperty>("radius").unwrap()->keyframes[180] = 150;

    rectClip->m_properties["radius"]->keyframeInfo[60] = {
        .easing = animation::Easing::Quadratic,
        .mode = animation::EasingMode::EaseInOut
    };

    rectClip->m_properties["radius"]->keyframeInfo[120] = {
        .easing = animation::Easing::Elastic,
        .mode = animation::EasingMode::EaseInOut
    };

    rectClip->m_properties["radius"]->keyframeInfo[180] = {
        .easing = animation::Easing::Sine,
        .mode = animation::EasingMode::EaseInOut
    };

    video->addClip(0, rectClip);

    video->audioTracks.push_back(std::make_shared<AudioTrack>());
    video->audioTracks.push_back(std::make_shared<AudioTrack>());

    if (ma_engine_init(NULL, &state.soundEngine) != MA_SUCCESS) {
        fmt::println("could not init engine");
    }
    
    state.textRenderer = std::make_shared<TextRenderer>();

    app.run();

    // cleanup

    ma_engine_uninit(&state.soundEngine);
    mlt_factory_close();

    return 0;
}
