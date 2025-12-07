#include <clips/default/circle.hpp>
#include <state.hpp>

#include <clips/properties/transform.hpp>
#include <clips/properties/number.hpp>
#include <clips/properties/color.hpp>

namespace clips {
    Circle::Circle(): Clip(60, 120) {
        addProperty(
            std::make_shared<TransformProperty>()
        );

        // radius (default = 500) 
        addProperty(
            std::make_shared<NumberProperty>()
                ->setId("radius")
                ->setName("Radius")
        );

        // color (self explanatory, default = black)
        addProperty(
            std::make_shared<ColorProperty>()
                ->setId("color")
                ->setName("Color")
        );

        m_metadata.name = "Circle";

        auto& state = State::get();

        Vector2D res = { 600, 600 };
        frame = std::make_shared<Frame>(
            res.x,
            res.y
        );

        this->frame->clearFrame({ 255, 255, 255, 255 });
        auto previewRad = 150;
        this->frame->drawCircle({ .position = {0 , 0 } }, previewRad, { 0, 0, 0, 255 });
    }

    Vector2D Circle::getSize() {
        int radius = getProperty<NumberProperty>("radius").unwrap()->data;
        int diameter = radius * 2;
        
        return { diameter, diameter };
    }

    Vector2D Circle::getPos() {
        Vector2D position = getProperty<TransformProperty>("transform").unwrap()->data.position;
        int radius = getProperty<NumberProperty>("radius").unwrap()->data;
        return position - radius;
    }

    void Circle::render(Frame* frame) {
        Transform transform = getProperty<TransformProperty>("transform").unwrap()->data;
        int radius = getProperty<NumberProperty>("radius").unwrap()->data;
        debug(radius);
        RGBAColor color = getProperty<ColorProperty>("color").unwrap()->data;
        frame->drawCircle(transform, radius, color.fade(opacity));
    }

    GLuint Circle::getPreviewTexture(int frameIdx) {
        return frame->textureID;
    }

    Vector2D Circle::getPreviewSize() { return { 600, 600 }; }
}
