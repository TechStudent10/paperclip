#pragma once

#include <unordered_map>
#include <string>
#include <vector>

#include <common.hpp>
#include <frame.hpp>

class TextRenderer {
protected:
    std::unordered_map<std::string, std::vector<unsigned char>> fontBuffers;
public:
    TextRenderer();
    void loadFont(std::string fontName);
    float drawText(Frame* frame, std::string text, std::string fontName, Vector2D pos, RGBAColor color, float pixelHeight = 48.f);
};
