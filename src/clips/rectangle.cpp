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
}
