#include <clips/default/text.hpp>

#include <state.hpp>
#include <clips/properties/transform.hpp>
#include <clips/properties/color.hpp>
#include <clips/properties/text.hpp>
#include <clips/properties/dropdown.hpp>
#include <clips/properties/number.hpp>

namespace clips {
    Text::Text(): Clip(60, 120) {
        addProperty(
            std::make_shared<TransformProperty>()
        );

        addProperty(
            std::make_shared<ColorProperty>()
                ->setId("color")
                ->setName("Color")
        );

        // default text property is fine here
        // (default = "Hello\nWorld!")
        addProperty(
            std::make_shared<TextProperty>("Hello\nWorld!")
                ->setId("text")
                ->setName("Text")
        );

        // font
        // default = "Inter"
        // options = 
        // "Inter",
        // "JetBrains Mono",
        // "Noto Sans",
        // "Noto Serif",
        // "Open Sans",
        // "Oswald",
        // "Raleway",
        // "Raleway Dots",
        // "Roboto",
        // "Source Code Pro",
        // "Source Serif 4"
        addProperty(
            std::make_shared<DropdownProperty>(std::vector<std::string>{
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
            }, "Inter")
                ->setId("font")
                ->setName("Font")
        );

        // font size
        // default = 90
        addProperty(
            std::make_shared<NumberProperty>(90)
                ->setId("size")
                ->setName("Font Size")
        );

        m_metadata.name = "Text";

        previewFrame = std::make_shared<Frame>(
            500,
            500
        );
        previewFrame->clearFrame({ 255, 255, 255, 255 });
        State::get().textRenderer->drawText(previewFrame.get(), "A", "resources/fonts/Inter.ttf", { 0, 0 }, { 0, 0, 0, 255 }, 500);
    }

    Vector2D Text::getSize() {
        auto fontSize = getProperty<NumberProperty>("size").unwrap()->data;

        return { (int)std::round(size.x), (int)std::round(size.y) };
    }

    Vector2D Text::getPos() {
        Vector2D position = getProperty<TransformProperty>("transform").unwrap()->data.position;
        return position;
    }

    void Text::render(Frame* frame) {
        auto& state = State::get();
        auto text = getProperty<TextProperty>("text").unwrap()->data;
        auto font = fmt::format("resources/fonts/{}.ttf", getProperty<DropdownProperty>("font").unwrap()->data);
        auto fontSize = getProperty<NumberProperty>("size").unwrap()->data;
        auto color = getProperty<ColorProperty>("color").unwrap()->data;
        auto transform = getProperty<TransformProperty>("transform").unwrap()->data;

        size = state.textRenderer->drawText(frame, text, font, transform, color.fade(opacity), fontSize);
    }

    GLuint Text::getPreviewTexture(int) {
        return previewFrame->textureID;
    }

    Vector2D Text::getPreviewSize() { return { 500, 500 }; }
}
