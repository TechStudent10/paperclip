#include <clips/default/rectangle.hpp>
#include <clips/properties/dimensions.hpp>
#include <clips/properties/color.hpp>


namespace clips {
    Rectangle::Rectangle(): Clip(60, 120) {
        addProperty(
            std::make_shared<DimensionsProperty>()
        );

        addProperty(
            std::make_shared<ColorProperty>()
                ->setId("color")
                ->setName("Color")
        );

        m_metadata.name = "Rectangle";

        previewFrame = std::make_shared<Frame>(
            500,
            500
        );
        previewFrame->clearFrame();
        int squareSize = 200;
        previewFrame->drawRect({
            .size {
                squareSize,
                squareSize
            },
            .transform = {
                .position = {
                    0, 0
                }
            },
        }, { 0, 0, 0, 255 });
    }

    Vector2D Rectangle::getPos() {
        Dimensions dimensions = getProperty<DimensionsProperty>("dimensions").unwrap()->data;
        return dimensions.transform.position;
    }

    Vector2D Rectangle::getSize() {
        Dimensions dimensions = getProperty<DimensionsProperty>("dimensions").unwrap()->data;
        return dimensions.size;
    }

    void Rectangle::render(Frame* frame) {
        Dimensions dimensions = getProperty<DimensionsProperty>("dimensions").unwrap()->data;
        RGBAColor color = getProperty<ColorProperty>("color").unwrap()->data;
        frame->drawRect(dimensions, color);
    }

    GLuint Rectangle::getPreviewTexture(int) {
        return previewFrame->textureID;
    }

    Vector2D Rectangle::getPreviewSize() { return { 500, 500 }; }
}
