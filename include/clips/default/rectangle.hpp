#pragma once

#include "../clip.hpp"
#include <common.hpp>
#include <frame.hpp>

namespace clips {
    class Rectangle : public Clip {
    protected:
        std::shared_ptr<Frame> previewFrame;
    public:
        Rectangle();
        void render(Frame* frame) override;

        ClipType getType() override { return ClipType::Rectangle; }
        Vector2D getSize() override;
        Vector2D getPos() override;

        GLuint getPreviewTexture(int frame) override;
        Vector2D getPreviewSize() override;
    };
} // namespace clips
