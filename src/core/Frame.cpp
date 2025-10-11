#include <cstdint>
#include <frame.hpp>

#include <shaders/shape.hpp>
#include <shaders/texture.hpp>

#include <shaders/shader.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
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

    // Set the list of draw buffers.
    GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

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

void Frame::clearFrame() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glViewport(0, 0, width, height);
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

const std::vector<unsigned char>& Frame::getFrameData() const {
    fmt::println("getFrameData unimplemented");
    std::vector<unsigned char> res;
    return res;
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

void Frame::primitiveDraw(Vector2D pos, Vector2D size, RGBAColor color, ShapeType type) {
    Vector2D resolution = { width, height };

    // top left
    Vector2D tl = pos;
    // top right
    Vector2D tr = { pos.x + size.x, pos.y };
    // bottom left
    Vector2D bl = { pos.x, pos.y + size.y };
    // bottom right
    Vector2D br = { pos.x + size.x, pos.y + size.y };

    float vertices[] = {
        (float)tl.x,  (float)tl.y, 0.0f,   // top left 
        (float)tr.x,  (float)tr.y, 0.0f,  // top right
        (float)bl.x, (float)bl.y, 0.0f,  // bottom left
        (float)br.x, (float)br.y, 0.0f,  // bottom right
    };
    unsigned int indices[] = {  // note that we start from 0!
        0, 1, 3,   // first triangle
        0, 2, 3    // second triangle
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

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
    if (type == ShapeType::Circle) {
        glUniform1f(
            glGetUniformLocation(shapeShaderProgram, "radius"),
            size.x / 2.f
        );
        glUniform2f(
            glGetUniformLocation(shapeShaderProgram, "center"),
            pos.x + (size.x / 2.f), pos.y + (size.y / 2.f)
        );
    }

    glBindVertexArray(VAO);
    
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Frame::drawRect(Dimensions dimensions, RGBAColor color) {
    primitiveDraw(dimensions.pos, dimensions.size, color);
}

void Frame::drawTexture(GLuint texture, Vector2D pos, Vector2D size, float rotation) {
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

    float vertices[] = {
        (float)tl.x,  (float)tl.y, 0.0f, 0.0f, 1.0f,   // top left 
        (float)tr.x,  (float)tr.y, 0.0f, 1.0f, 1.0f,  // top right
        (float)bl.x, (float)bl.y, 0.0f, 0.0f, 0.0f,  // bottom left
        (float)br.x, (float)br.y, 0.0f, 1.0f, 0.0f  // bottom right
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

    glm::mat4 matrix = glm::ortho(0.f, (float)width, 0.f, (float)height, -1.f, 1.f);
    glm::mat4 model = glm::mat4(1.0f);
    auto center = glm::vec2(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f);
    model = glm::translate(model, glm::vec3(center, 0.0f));
    model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));
    model = glm::translate(model, glm::vec3(-center.x, -center.y, 0.f));

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

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Frame::drawTextureYUV(GLuint textureY, GLuint textureU, GLuint textureV, Vector2D pos, Vector2D size, float rotation) {
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

    float vertices[] = {
        (float)tl.x,  (float)tl.y, 0.0f, 0.0f, 1.0f,   // top left 
        (float)tr.x,  (float)tr.y, 0.0f, 1.0f, 1.0f,  // top right
        (float)bl.x, (float)bl.y, 0.0f, 0.0f, 0.0f,  // bottom left
        (float)br.x, (float)br.y, 0.0f, 1.0f, 0.0f  // bottom right
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

    glm::mat4 matrix = glm::ortho(0.f, (float)width, 0.f, (float)height, -1.f, 1.f);
    glm::mat4 model = glm::mat4(1.0f);
    auto center = glm::vec2(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f);
    model = glm::translate(model, glm::vec3(center, 0.0f));
    model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));
    model = glm::translate(model, glm::vec3(-center.x, -center.y, 0.f));

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

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

void Frame::drawCircle(Vector2D center, int radius, RGBAColor color, bool filled) {
    // TODO: deal with filled
    primitiveDraw({ center.x - radius, center.y - radius }, { radius * 2, radius * 2 }, color, ShapeType::Circle);
}