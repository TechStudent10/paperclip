#pragma once

#include <common.hpp>
#include <frame.hpp>

// i roll my OWN pi
#define PI 3.14159265358927

static constexpr double PI_DIV_180 = PI / 180.f;

namespace utils {
    void drawImage(Frame* frame, unsigned char* data, Vector2D size, Vector2D position, double rotation = 0);
} // namespace utils
