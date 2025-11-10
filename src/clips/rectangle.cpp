#include <clips/default/rectangle.hpp>

namespace clips {
    Rectangle::Rectangle(): Clip(60, 120) {
        m_properties.addProperty(
            ClipProperty::dimensions()
        );

        m_properties.addProperty(
            ClipProperty::color()
        );

        m_metadata.name = "Rectangle";

        previewFrame = std::make_shared<Frame>(
            500,
            500
        );
        previewFrame->clearFrame();
        int squareSize = 200;
        previewFrame->drawRect({
            .pos = {
                (500 - squareSize) / 2,
                (500 - squareSize) / 2
            },
            .size {
                squareSize,
                squareSize
            }
        }, { 0, 0, 0, 255 });
    }

    Vector2D Rectangle::getPos() {
        Dimensions dimensions = Dimensions::fromString(m_properties.getProperty("dimensions")->data);
        return dimensions.pos;
    }

    Vector2D Rectangle::getSize() {
        Dimensions dimensions = Dimensions::fromString(m_properties.getProperty("dimensions")->data);
        return dimensions.size;
    }

    void Rectangle::render(Frame* frame) {
        Dimensions dimensions = Dimensions::fromString(m_properties.getProperty("dimensions")->data);
        RGBAColor color = RGBAColor::fromString(m_properties.getProperty("color")->data);
        frame->drawRect(dimensions, color);
    }

    GLuint Rectangle::getPreviewTexture(int) {
        return previewFrame->textureID;
    }

    Vector2D Rectangle::getPreviewSize() { return { 500, 500 }; }
}
