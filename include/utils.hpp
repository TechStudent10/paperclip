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
        fmt::println("{} failed to unwrap: {}", #expr, b.unwrapErr().message()); \
        return; \
    } \
})()

// cool little debug macro
#define debug(statement) ([&]() { auto res = statement; fmt::println("{}: {}", #statement, res); return res; })()

static constexpr double PI_DIV_180 = PI / 180.f;

namespace utils {
    std::string generateUUID();
} // namespace utils

namespace utils::video {
    // these arent the best argument names
    // but i like the album and the song
    // so we're sticking with it
    // (audio = audio input file, video = video input file, disco = overall output)
    void combineAV(std::string audio, std::string video, std::string disco);

    void extractAudio(std::string filename);
} // namespace video
