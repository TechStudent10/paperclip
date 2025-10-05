#include <clips/default/text.hpp>

#include <state.hpp>

namespace clips {
    Text::Text(): Clip(60, 120) {
        m_properties.addProperty(
            ClipProperty::position()
        );

        m_properties.addProperty(
            ClipProperty::color()
        );

        // default text property is fine here
        m_properties.addProperty(
            ClipProperty::text()
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
    }

    Vector2D Text::getSize() {
        auto fontSize = Vector1D::fromString(m_properties.getProperty("size")->data).number;

        return { static_cast<int>(width), fontSize };
    }

    Vector2D Text::getPos() {
        Vector2D position = Vector2D::fromString(m_properties.getProperty("position")->data);
        return position;
    }

    void Text::render(Frame* frame) {
        auto& state = State::get();
        auto text = m_properties.getProperty("text")->data;
        auto font = fmt::format("resources/fonts/{}.ttf", m_properties.getProperty("font")->data);
        auto fontSize = Vector1D::fromString(m_properties.getProperty("size")->data).number;
        auto pos = Vector2D::fromString(m_properties.getProperty("position")->data);
        auto color = RGBAColor::fromString(m_properties.getProperty("color")->data);

        width = state.textRenderer->drawText(frame, text, font, pos, color, fontSize);
    }
}
