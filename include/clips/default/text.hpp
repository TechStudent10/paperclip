#pragma once

#include "../clip.hpp"
#include <common.hpp>
#include <frame.hpp>

namespace clips {
    class Text : public Clip {
    public:
        float width = 0.f;

        Text();
        void render(Frame* frame) override;

        ClipType getType() override { return ClipType::Text; }
        Vector2D getSize() override;
        Vector2D getPos() override;
    };
} // namespace clips
