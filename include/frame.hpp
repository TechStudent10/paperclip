#pragma once

#include <vector>
#include <common.hpp>

class Frame {
protected:
public:
    std::vector<unsigned char> imageData;
    int width;
    int height;
    Frame(int width, int height) : width(width), height(height) {
        this->imageData = std::vector<unsigned char>(width * height * 4);
        this->imageData.resize(width * height * 4);
        clearFrame();
    }

    void clearFrame() {
        std::fill(this->imageData.begin(), this->imageData.end(), 255);
    }

    void putPixel(Vector2D position, RGBAColor color);
    RGBAColor getPixel(Vector2D position);

    void drawRect(Dimensions dimensions, RGBAColor color);
    void drawLine(Vector2D start, Vector2D end, RGBAColor color, int thickness = 1);
    void drawCircle(Vector2D center, int radius, RGBAColor color, bool filled = true);

    const std::vector<unsigned char>& getFrameData() const;
};