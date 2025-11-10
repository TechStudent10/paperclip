#pragma once

#include <vector>
#include <common.hpp>

#include <glad/include/glad/gl.h>
#include <SDL3/SDL_opengl.h>

class Frame {
protected:
    enum class ShapeType {
        Rect,
        Circle
    };

    void primitiveDraw(Vector2D pos, Vector2D size, RGBAColor color, ShapeType type = ShapeType::Rect);
    std::vector<unsigned char> imageData;
public:
    int width;
    int height;

    GLuint fbo;
    GLuint textureID;

    GLuint shapeShaderProgram;
    GLuint texShaderProgram;
    GLuint texYUVShaderProgram;
    // GLuint rectVAO, rectVBO;
    GLuint VAO, VBO, EBO;

    Frame(int width, int height);

    void clearFrame(RGBAColor color = { 255, 255, 255, 255 });

    void putPixel(Vector2D position, RGBAColor color);
    RGBAColor getPixel(Vector2D position);

    void drawRect(Dimensions dimensions, RGBAColor color);
    void drawLine(Vector2D start, Vector2D end, RGBAColor color, int thickness = 1);
    void drawCircle(Vector2D center, int radius, RGBAColor color, bool filled = true);

    void drawTexture(GLuint texture, Vector2D position, Vector2D size, GLuint VAO, GLuint VBO, GLuint EBO, float rotation = 0);
    void drawTextureYUV(GLuint textureY, GLuint textureU, GLuint textureV, Vector2D position, Vector2D size, GLuint VAO, GLuint VBO, GLuint EBO, float rotation = 0);

    std::vector<unsigned char> getFrameData();
};