#include <clips/default/text.hpp>

#include <state.hpp>

namespace clips {
    Text::Text(): Clip(60, 120) {
        m_properties.addProperty(
            ClipProperty::transform()
        );

        m_properties.addProperty(
            ClipProperty::color()
        );

        // default text property is fine here
        m_properties.addProperty(
            ClipProperty::text()
                ->setDefaultKeyframe("Hello\nWorld!")
        );

        m_properties.addProperty(
            ClipProperty::dropdown()
                ->setId("font")
                ->setName("Font")
                ->setOptions(DropdownOptions{
                    .options = std::vector<std::string>({
                        "Inter",
                        "JetBrains Mono",
                        "Noto Sans",
                        "Noto Serif",
                        "Open Sans",
                        "Oswald",
                        "Raleway",
                        "Raleway Dots",
                        "Roboto",
                        "Source Code Pro",
                        "Source Serif 4"
                    })
                }.toString())
                ->setDefaultKeyframe("Inter")
        );

        m_properties.addProperty(
            ClipProperty::number()
                ->setId("size")
                ->setName("Font Size")
                ->setDefaultKeyframe(Vector1D{ .number = 90 }.toString())
        );

        m_metadata.name = "Text";

        previewFrame = std::make_shared<Frame>(
            500,
            500
        );
        previewFrame->clearFrame();
        State::get().textRenderer->drawText(previewFrame.get(), "A", "resources/fonts/Inter.ttf", { 0, 0 }, { 0, 0, 0, 255 }, 500);
    }

    Vector2D Text::getSize() {
        auto fontSize = Vector1D::fromString(m_properties.getProperty("size")->data).number;

        return { (int)std::round(size.x), (int)std::round(size.y) };
    }

    Vector2D Text::getPos() {
        Vector2D position = Transform::fromString(m_properties.getProperty("transform")->data).position;
        return position;
    }

    void Text::render(Frame* frame) {
        auto& state = State::get();
        auto text = m_properties.getProperty("text")->data;
        auto font = fmt::format("resources/fonts/{}.ttf", m_properties.getProperty("font")->data);
        auto fontSize = Vector1D::fromString(m_properties.getProperty("size")->data).number;
        auto color = RGBAColor::fromString(m_properties.getProperty("color")->data);
        auto transform = Transform::fromString(m_properties.getProperty("transform")->data);

        size = state.textRenderer->drawText(frame, text, font, transform, color, fontSize);
    }

    GLuint Text::getPreviewTexture(int) {
        return previewFrame->textureID;
    }

    Vector2D Text::getPreviewSize() { return { 500, 500 }; }
}
