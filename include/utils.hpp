#pragma once

#include <common.hpp>
#include <frame.hpp>

// i roll my OWN pi
#define PI 3.14159265358927

static constexpr double PI_DIV_180 = PI / 180.f;

namespace utils {
    void drawImage(Frame* frame, unsigned char* data, Vector2D size, Vector2D position, double rotation = 0);
} // namespace utils

namespace utils::video {
    // these arent the best argument names
    // but i like the album and the song
    // so we're sticking with it
    // (audio = audio input file, video = video input file, disco = overall output)
    void combineAV(std::string audio, std::string video, std::string disco);

    void extractAudio(std::string filename);
} // namespace video
