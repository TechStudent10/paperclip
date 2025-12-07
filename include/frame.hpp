#pragma once

#include <vector>
#include <common.hpp>

#include <glad/include/glad/gl.h>
#include <SDL3/SDL_opengl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Frame {
protected:
    enum class ShapeType {
        Rect,
        Circle
    };

    void primitiveDraw(Transform transform, Vector2D size, RGBAColor color, ShapeType type = ShapeType::Rect);
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

    void clearFrame(RGBAColor color = { 0, 0, 0, 255 });

    void putPixel(Vector2D position, RGBAColor color);
    RGBAColor getPixel(Vector2D position);

    void drawRect(Dimensions dimensions, RGBAColor color);
    void drawLine(Vector2D start, Vector2D end, RGBAColor color, int thickness = 1);
    void drawCircle(Transform transform, int radius, RGBAColor color, bool filled = true);

    void drawTexture(GLuint texture, Vector2D size, Transform transform, GLuint VAO, GLuint VBO, GLuint EBO, float opacity = 1.f);
    void drawTextureYUV(GLuint textureY, GLuint textureU, GLuint textureV, Vector2D size, Transform transform, GLuint VAO, GLuint VBO, GLuint EBO, float opacity = 1.f);

    // 0.5, 0.5 = center
    // 0, 0 = top left
    // 0, 1 = top right
    // 1, 0 = bottom left
    // 1, 1 = bottom right
    glm::mat4 createBaseMatrix(Vector2DF anchorPoint = { 0.5, 0.5 });
    glm::mat4 createModelFromTransform(Transform transform, Vector2D pos, Vector2D size, bool reverseY = false);

    std::vector<unsigned char> getFrameData();
};