#include <clips/default/circle.hpp>

namespace clips {
    Circle::Circle(): Clip(60, 120) {
        m_properties.addProperty(
            ClipProperty::position()
        );

        m_properties.addProperty(
            ClipProperty::number()
                ->setName("Radius")
                ->setId("radius")
                ->setDefaultKeyframe(Vector1D{ .number = 500 }.toString())
        );

        m_properties.addProperty(
            ClipProperty::color()
        );

        m_metadata.name = "Circle";
    }

    Vector2D Circle::getSize() {
        int diameter = Vector1D::fromString(m_properties.getProperty("radius")->data).number * 2;
        
        return { diameter, diameter };
    }

    Vector2D Circle::getPos() {
        Vector2D position = Vector2D::fromString(m_properties.getProperty("position")->data);
        int radius = Vector1D::fromString(m_properties.getProperty("radius")->data).number;
        return { position.x - radius, position.y - radius };
    }

    void Circle::render(Frame* frame) {
        Vector2D position = Vector2D::fromString(m_properties.getProperty("position")->data);
        Vector1D radius = Vector1D::fromString(m_properties.getProperty("radius")->data);
        RGBAColor color = RGBAColor::fromString(m_properties.getProperty("color")->data);
        frame->drawCircle(position, radius.number, color);
    }
}
