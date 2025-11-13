#include <clips/default/circle.hpp>
#include <state.hpp>

namespace clips {
    Circle::Circle(): Clip(60, 120) {
        m_properties.addProperty(
            ClipProperty::transform()
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

        auto& state = State::get();

        Vector2D res = { 600, 600 };
        frame = std::make_shared<Frame>(
            res.x,
            res.y
        );

        this->frame->clearFrame();
        auto previewRad = 150;
        this->frame->drawCircle({ .position = {0 , 0 } }, previewRad, { 0, 0, 0, 255 });
    }

    Vector2D Circle::getSize() {
        int diameter = Vector1D::fromString(m_properties.getProperty("radius")->data).number * 2;
        
        return { diameter, diameter };
    }

    Vector2D Circle::getPos() {
        Vector2D position = Transform::fromString(m_properties.getProperty("transform")->data).position;
        int radius = Vector1D::fromString(m_properties.getProperty("radius")->data).number;
        return position - radius;
    }

    void Circle::render(Frame* frame) {
        Transform transform = Transform::fromString(m_properties.getProperty("transform")->data);
        Vector1D radius = Vector1D::fromString(m_properties.getProperty("radius")->data);
        RGBAColor color = RGBAColor::fromString(m_properties.getProperty("color")->data);
        frame->drawCircle(transform, radius.number, color);
    }

    GLuint Circle::getPreviewTexture(int frameIdx) {
        return frame->textureID;
    }

    Vector2D Circle::getPreviewSize() { return { 600, 600 }; }
}
