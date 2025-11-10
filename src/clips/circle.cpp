#include <clips/default/circle.hpp>
#include <state.hpp>

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

        auto& state = State::get();

        Vector2D res = { 600, 600 };
        frame = std::make_shared<Frame>(
            res.x,
            res.y
        );

        this->frame->clearFrame();
        auto previewRad = 150;
        this->frame->drawCircle({ res.x / 2, res.y / 2 }, previewRad, { 0, 0, 0, 255 });
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
        debug(position.x);
        debug(position.y);
        debug(radius.number);
        debug(color.r);
        debug(color.g);
        debug(color.b);
        debug(color.a);
        frame->drawCircle(position, radius.number, color);
        if (frame != this->frame.get()) {
            // this->frame->clearFrame();
            // render(this->frame.get());
        }
    }

    GLuint Circle::getPreviewTexture(int frameIdx) {
        // frame->clearFrame({ frameIdx * 15, frameIdx * 15, frameIdx * 15 });
        return frame->textureID;
    }

    Vector2D Circle::getPreviewSize() { return { 600, 600 }; }
}
