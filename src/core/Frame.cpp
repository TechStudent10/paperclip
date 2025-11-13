#include "utils.hpp"
#include <cstdint>
#include <frame.hpp>

#include <shaders/shape.hpp>
#include <shaders/texture.hpp>

#include <shaders/shader.hpp>

#include <glm/gtc/type_ptr.hpp>

Frame::Frame(int width, int height) : width(width), height(height) {
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        width, height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        nullptr
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureID, 0);

    shapeShaderProgram = shader::createProgram(shapeVertex, shapeFragment);
    texShaderProgram = shader::createProgram(textureVertex, textureFragment);
    texYUVShaderProgram = shader::createProgram(textureVertex, textureFragmentYUV);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
}

void Frame::clearFrame(RGBAColor color) {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glViewport(0, 0, width, height);
    glClearColor(color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

std::vector<unsigned char> Frame::getFrameData() {
    if (imageData.size() <= 0) {
        imageData.resize(width * height * 4);
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(
        0, 0,
        width, height,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        imageData.data()
    );

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    return imageData;
}

void Frame::putPixel(Vector2D position, RGBAColor color) {}

RGBAColor Frame::getPixel(Vector2D position) {
    fmt::println("getPixel unimplemented");
    return {
        0,
        0,
        0,
        0
    };
}

Vector2DF normalizeToOpenGL(Vector2D in, Vector2D resolution) {
    return {
        .x = in.x / ((float)resolution.x / 2.f) - 1,
        .y = in.y / ((float)resolution.y / 2.f) - 1
    };
}

glm::mat4 Frame::createBaseMatrix(Vector2DF anchorPoint) {
    // anchorPoint.x = std::clamp(anchorPoint.x, 0.f, 1.f);
    // anchorPoint.y = std::clamp(anchorPoint.y, 0.f, 1.f);

    // float left = -width * anchorPoint.x;
    // float bottom = -height * anchorPoint.y;

    // return glm::ortho(
    //     left,
    //     left + width,
    //     bottom,
    //     bottom + height,
    //     -1000.f, 1000.f
    // );

    float fov = glm::radians(20.f);
    float camDist = (height * 0.5f) / tan(fov * 0.5f);

    return glm::perspective(
        fov,
        (float)width / (float)height,
        0.1f,
        camDist * 10.f
    ) * glm::lookAt(
        glm::vec3(0.f, 0.f, camDist),
        glm::vec3(0.f, 0.f, 0.f),
        glm::vec3(0.f, 1.f, 0.f)
    );
}

glm::mat4 Frame::createModelFromTransform(Transform transform, Vector2D pos, Vector2D size, bool reverseY) {
    glm::vec3 anchorOffset = glm::vec3(
        0.5f * size.x,
        0.5f * size.y,
        0.0f
    );

    glm::vec3 screenOffset = glm::vec3(
        (-0.5f + transform.anchorPoint.x) * width,
        (reverseY ? (0.5f - transform.anchorPoint.y) : (-0.5f + transform.anchorPoint.y)) * height, // Y flipped so 0 = top
        0.0f
    );

    glm::mat4 model = glm::mat4(1.0);
    // auto center = glm::vec2(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f);
    model = glm::translate(model, screenOffset + glm::vec3(pos.x, pos.y, 0.f));
    model = glm::translate(model, +anchorOffset);
    model = glm::rotate(model, glm::radians(transform.rotation), glm::vec3(0, 0, 1)); // rotate
    model = glm::rotate(model, glm::radians(transform.pitch), glm::vec3(1, 0, 0)); // pitch
    model = glm::rotate(model, glm::radians(transform.roll), glm::vec3(0, 1, 0)); // roll
    model = glm::scale(model, glm::vec3(1.0f));
    // model = glm::translate(model, glm::vec3(-center.x, -center.y, 0.f));
    return model * glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, -1.0f, 1.0f));
}

void Frame::primitiveDraw(Transform transform, Vector2D size, RGBAColor color, ShapeType type) {
    Vector2D pos = transform.position - size / 2;
    Vector2D resolution = { width, height };

    float halfW = size.x * 0.5f;
    float halfH = size.y * 0.5f;

    // top left
    Vector2D tl = pos;
    // top right
    Vector2D tr = { pos.x + size.x, pos.y };
    // bottom left
    Vector2D bl = { pos.x, pos.y + size.y };
    // bottom right
    Vector2D br = { pos.x + size.x, pos.y + size.y };

    float vertices[] = {
        -halfW,  halfH, 0.0f,   // top left 
        halfW,  halfH, 0.0f,  // top right
        -halfW, -halfH, 0.0f,  // bottom left
        halfW, -halfH, 0.0f  // bottom right
    };
    unsigned int indices[] = {  // note that we start from 0!
        0, 1, 3,   // first triangle
        0, 2, 3    // second triangle
    };

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glm::mat4 matrix = createBaseMatrix(transform.anchorPoint);
    glm::mat4 model = createModelFromTransform(transform, pos, size);

    matrix = matrix * model;

    glUseProgram(shapeShaderProgram);
    glUniform4f(
        glGetUniformLocation(shapeShaderProgram, "outColor"),
        color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f
    );
    glUniform2f(
        glGetUniformLocation(shapeShaderProgram, "screenSize"),
        (float)resolution.x, (float)resolution.y
    );
    glUniform1i(
        glGetUniformLocation(shapeShaderProgram, "type"),
        (int)type
    );
    glUniformMatrix4fv(
        glGetUniformLocation(shapeShaderProgram, "matrix"),
        1, GL_FALSE, glm::value_ptr(matrix)
    );
    if (type == ShapeType::Circle) {
        glUniform1f(
            glGetUniformLocation(shapeShaderProgram, "radius"),
            size.x / 2.f
        );
        glUniform2f(
            glGetUniformLocation(shapeShaderProgram, "center"),
            (pos.x + (width * transform.anchorPoint.x)) + (size.x / 2.f),
            (pos.y + (height * transform.anchorPoint.y)) + (size.y / 2.f)
        );
    }

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Frame::drawRect(Dimensions dimensions, RGBAColor color) {
    primitiveDraw(dimensions.transform, dimensions.size, color);
}

void Frame::drawTexture(GLuint texture, Vector2D size, Transform transform, GLuint VAO, GLuint VBO, GLuint EBO) {
    Vector2D pos = transform.position - size / 2;
    // stolen from the primitive draw lmao
    Vector2D resolution = { width, height };

    // top left
    Vector2D tl = pos;
    // top right
    Vector2D tr = { pos.x + size.x, pos.y };
    // bottom left
    Vector2D bl = { pos.x, pos.y + size.y };
    // bottom right
    Vector2D br = pos + size;

    float left = -transform.anchorPoint.x * size.x;
    float right = (1.f - transform.anchorPoint.x) * size.x;
    float top = (1.f - transform.anchorPoint.y) * size.y;
    float bottom = -transform.anchorPoint.y * size.y;

    float halfW = size.x * 0.5f;
    float halfH = size.y * 0.5f;

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    float vertices[] = {
        -halfW,  halfH, 0.0f, 0.0f, 1.0f,   // top left 
        halfW,  halfH, 0.0f, 1.0f, 1.0f,  // top right
        -halfW, -halfH, 0.0f, 0.0f, 0.0f,  // bottom left
        halfW, -halfH, 0.0f, 1.0f, 0.0f  // bottom right
    };

    unsigned int indices[] = {  // note that we start from 0!
        0, 1, 3,   // first triangle
        0, 2, 3    // second triangle
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glm::mat4 matrix = createBaseMatrix(transform.anchorPoint);
    glm::mat4 model = createModelFromTransform(transform, pos, size);

    matrix = matrix * model;

    glUseProgram(texShaderProgram);
    glUniform1i(
        glGetUniformLocation(texShaderProgram, "texture1"),
        0
    );
    glUniformMatrix4fv(
        glGetUniformLocation(texShaderProgram, "matrix"),
        1, GL_FALSE, glm::value_ptr(matrix)
    );

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Frame::drawTextureYUV(GLuint textureY, GLuint textureU, GLuint textureV, Vector2D size, Transform transform, GLuint VAO, GLuint VBO, GLuint EBO) {
    Vector2D pos = transform.position - size / 2;
    // stolen from the primitive draw lmao
    Vector2D resolution = { width, height };

    // top left
    Vector2D tl = pos;
    // top right
    Vector2D tr = { pos.x + size.x, pos.y };
    // bottom left
    Vector2D bl = { pos.x, pos.y + size.y };
    // bottom right
    Vector2D br = { pos.x + size.x, pos.y + size.y };

    float halfW = size.x * 0.5f;
    float halfH = size.y * 0.5f;

    float vertices[] = {
        -halfW,  halfH, 0.0f, 0.0f, 1.0f,   // top left 
        halfW,  halfH, 0.0f, 1.0f, 1.0f,  // top right
        -halfW, -halfH, 0.0f, 0.0f, 0.0f,  // bottom left
        halfW, -halfH, 0.0f, 1.0f, 0.0f  // bottom right
    };

    unsigned int indices[] = {  // note that we start from 0!
        0, 1, 3,   // first triangle
        0, 2, 3    // second triangle
    };

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glm::mat4 matrix = createBaseMatrix(transform.anchorPoint);
    glm::mat4 model = createModelFromTransform(transform, pos, size);
    glm::mat4 flipY = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, -1.0f, 1.0f));

    matrix = flipY * matrix * model;

    glUseProgram(texYUVShaderProgram);
    glUniform1i(
        glGetUniformLocation(texYUVShaderProgram, "textureY"),
        0
    );
    glUniform1i(
        glGetUniformLocation(texYUVShaderProgram, "textureU"),
        1
    );
    glUniform1i(
        glGetUniformLocation(texYUVShaderProgram, "textureV"),
        2
    );

    glUniformMatrix4fv(
        glGetUniformLocation(texYUVShaderProgram, "matrix"),
        1, GL_FALSE, glm::value_ptr(matrix)
    );

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureY);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureU);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, textureV);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Frame::drawLine(Vector2D start, Vector2D end, RGBAColor color, int thickness) {
    // brensenham line algorithm
    // https://en.wikipedia.org/wiki/Bresenham's_line_algorithm
    
    auto plotLineLow = [this, color, thickness](Vector2D start, Vector2D end) {
        int dx = end.x - start.x;
        int dy = end.y - start.y;

        int yi = 1;
        if (dy < 0) {
            yi = -1;
            dy = -dy;
        }
        int D = (dy * 2) - dx;
        int y = start.y;
        for (int x = start.x; x < end.x; x++) {
            for (int i = -thickness/2; i <= thickness/2; i++) {
                for (int j = -thickness/2; j <= thickness/2; j++) {
                    putPixel({ x + i, y + j }, color);
                }
            }

            if (D > 0) {
                y = y + yi;
                D = D + (2 * (dy - dx));
            } else {
                D = D + dy * 2;
            }
        }
    };

    auto plotLineHigh = [this, color, thickness](Vector2D start, Vector2D end) {
        int dx = end.x - start.x;
        int dy = end.y - start.y;
        int xi = 1;
        if (dx < 0) {
            xi = -1;
            dx = -dx;
        }

        int D = (dx * 2) - dy;
        int x = start.x;

        for (int y = start.y; y < end.y; y++) {
            for (int i = -thickness/2; i <= thickness/2; i++) {
                for (int j = -thickness/2; j <= thickness/2; j++) {
                    putPixel({ x + i, y + j }, color);
                }
            }
            if (D > 0) {
                x = x + xi;
                D = D + (2 * (dx - dy));
            } else {
                D = D + dx * 2;
            }
        }
    };

    if (abs(end.y - start.y) < abs(end.x - start.x)) {
        if (start.x > end.x) {
            plotLineLow(end, start);
        } else {
            plotLineLow(start, end);
        }
    } else {
        if (start.y > end.y) {
            plotLineHigh(end, start);
        } else {
            plotLineHigh(start, end);
        }
    }
}

void Frame::drawCircle(Transform transform, int radius, RGBAColor color, bool filled) {
    // transform.position = transform.position - radius;
    // TODO: deal with filled
    primitiveDraw(transform, { radius * 2, radius * 2 }, color, ShapeType::Circle);
}