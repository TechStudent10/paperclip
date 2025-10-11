#pragma once

#include "../clip.hpp"
#include <common.hpp>
#include <frame.hpp>

namespace clips {
    class Circle : public Clip {
    public:
        Circle();
        void render(Frame* frame) override;

        ClipType getType() override { return ClipType::Circle; }
        Vector2D getSize() override;
        Vector2D getPos() override;
    };
} // namespace clips
