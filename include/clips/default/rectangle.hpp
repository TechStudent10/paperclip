#pragma once

#include "../clip.hpp"
#include <common.hpp>
#include <frame.hpp>

namespace clips {
    class Rectangle : public Clip {
    public:
        Rectangle();
        void render(Frame* frame) override;

        ClipType getType() override { return ClipType::Rectangle; }
        Vector2D getSize() override;
        Vector2D getPos() override;
    };
} // namespace clips
