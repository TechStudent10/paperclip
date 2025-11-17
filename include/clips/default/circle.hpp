#pragma once

#include "../clip.hpp"
#include <common.hpp>
#include <frame.hpp>

namespace clips {
    class Circle : public Clip {
    protected:
        std::shared_ptr<Frame> frame;
    public:
        Circle();
        void render(Frame* frame) override;

        ClipType getType() override { return ClipType::Circle; }
        Vector2D getSize() override;
        Vector2D getPos() override;

        GLuint getPreviewTexture(int frame) override;
        Vector2D getPreviewSize() override;
    };
} // namespace clips
