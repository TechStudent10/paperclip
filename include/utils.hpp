#pragma once

#include <common.hpp>
#include <frame.hpp>

// i roll my OWN pi
#define PI 3.14159265358927

// use this to log errors from geode results
#define UNWRAP_WITH_ERR_DEFAULT(expr, default) ([&]() { \
    auto b = expr; \
    if (b.isErr()) { \
        fmt::println("##expr failed to unwrap: {}", b.unwrapErr().message()); \
        return default; \
    } \
    return b.unwrap(); \
})()

#define UNWRAP_WITH_ERR(expr) ([&]() { \
    auto b = expr; \
    if (b.isErr()) { \
        fmt::println("{} failed to unwrap at {}:{}: {}", #expr, __FILE__, __LINE__, b.unwrapErr().message()); \
        return; \
    } \
})()

// cool little debug macro
#define debug(...) ([&]() { auto res = __VA_ARGS__; fmt::println("{}:{} {}: {}", __FILE__, __LINE__, #__VA_ARGS__, res); return res; })()

static constexpr double PI_DIV_180 = PI / 180.f;

namespace utils {
    std::string generateUUID();

    template <typename T>
    bool vectorContains(std::vector<T> vec, T elem) {
        return std::find(vec.begin(), vec.end(), elem) != vec.end();
    }

    template <typename T>
    void removeFromVector(std::vector<T> vec, T elem) {
        vec.erase(
            std::remove(vec.begin(), vec.end(), elem),
            vec.end()
        );
    }

    // interpolates from a to b
    // based on progress
    inline float interpolate(float progress, float a, float b) {
        return a + (b - a) * progress;
    }
} // namespace utils

namespace utils::video {
    // these arent the best argument names
    // but i like the album and the song
    // so we're sticking with it
    // (audio = audio input file, video = video input file, disco = overall output)
    void combineAV(std::string audio, std::string video, std::string disco);

    void extractAudio(std::string filename);
} // namespace video
