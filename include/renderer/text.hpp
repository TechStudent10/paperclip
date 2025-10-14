#pragma once

#include <unordered_map>
#include <string>

#include <common.hpp>
#include <frame.hpp>

#include <freetype/freetype.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

struct Character {
    GLuint texture;
    Vector2D size;
    Vector2D bearing;
    long advance;
};

struct Font {
    long ascent;
    long lineHeight;
    std::map<char, Character> characters;
};

class TextRenderer {
protected:
    FT_Library ft;
    std::unordered_map<std::string, Font> fonts;
    GLuint textShaderProgram;

    static constexpr float LOAD_SIZE = 100.f;
public:
    TextRenderer();
    void loadFont(std::string fontName);

    Vector2DF drawText(Frame* frame, std::string text, std::string fontName, Vector2D pos, RGBAColor color, float pixelHeight = 48.f, float rotation = 0.f);
    Vector2DF getTextSize(std::string text, std::string fontName, float scale = 1.f);
};
