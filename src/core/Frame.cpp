#include <cstdint>
#include <frame.hpp>

#include <shaders/shape.hpp>
#include <shaders/shader.hpp>

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

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    // float rectVerts[] = {
    //     0, 0,
    //     1, 0,
    //     1, 1,
    //     0, 0,
    //     1, 1,
    //     0, 1
    // };

    // glGenVertexArrays(1, &rectVAO);
    // glGenBuffers(1, &rectVBO);

    // glBindVertexArray(rectVAO);
    // glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(rectVerts), rectVerts, GL_STATIC_DRAW);

    // glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    // glEnableVertexAttribArray(0);

    // glBindVertexArray(0);
    // glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Frame::clearFrame() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glViewport(0, 0, width, height);
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    // GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
    // glDrawBuffers(1, DrawBuffers);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

const std::vector<unsigned char>& Frame::getFrameData() const {
    fmt::println("getFrameData unimplemented");
    std::vector<unsigned char> res;
    return res;
}

void Frame::putPixel(Vector2D position, RGBAColor color) {
    // fmt::println("putPixel unimplemented");
//     int x = position.x;
//     int y = position.y;

//     if (x < 0 || x >= width || y < 0 || y >= height) return;

//     int loc = (y * width + x) * 4;
//     uint8_t* dst = &imageData[loc];

//     if (color.a == 255) {
//         dst[0] = color.r;
//         dst[1] = color.g;
//         dst[2] = color.b;
//         dst[3] = 255;
//         return;
//     }

//     int alpha = color.a;
//     int invAlpha = 255 - alpha;

//     dst[0] = (dst[0] * invAlpha + color.r * alpha) >> 8; // R
//     dst[1] = (dst[1] * invAlpha + color.g * alpha) >> 8; // G
//     dst[2] = (dst[2] * invAlpha + color.b * alpha) >> 8; // B
//     dst[3] = 255;
}

RGBAColor Frame::getPixel(Vector2D position) {
    fmt::println("getPixel unimplemented");
    // int location = (position.y * width + position.x) * 4;
    // return {
    //     .r = imageData[location],
    //     .g = imageData[location + 1],
    //     .b = imageData[location + 2],
    //     .a = imageData[location + 3]
    // };

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

void Frame::drawRect(Dimensions dimensions, RGBAColor color) {
    Vector2D resolution = { width, height };

    // top left
    Vector2D tl = dimensions.pos;
    // top right
    Vector2D tr = { dimensions.pos.x + dimensions.size.x, dimensions.pos.y };
    // bottom left
    Vector2D bl = { dimensions.pos.x, dimensions.pos.y + dimensions.size.y };
    // bottom right
    Vector2D br = { dimensions.pos.x + dimensions.size.x, dimensions.pos.y + dimensions.size.y };

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

    glBindVertexArray(VAO);
    
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
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
    auto putPixelAlpha = [this](Vector2D p, RGBAColor color, float alpha) {
        RGBAColor existing = getPixel(p);
        RGBAColor blended;
        blended.r = static_cast<uint8_t>(existing.r * (1 - alpha) + color.r * alpha);
        blended.g = static_cast<uint8_t>(existing.g * (1 - alpha) + color.g * alpha);
        blended.b = static_cast<uint8_t>(existing.b * (1 - alpha) + color.b * alpha);
        blended.a = 255;
        putPixel(p, blended);
    };
    
    auto drawSymmetricPixels = [this, radius, putPixelAlpha, center, color](Vector2D point) {
        auto x = point.x;
        auto y = point.y;
        auto cx = center.x;
        auto cy = center.y;

        float dist = std::abs(std::sqrt(x * x + y * y) - radius);
        float alpha = std::max(0.0f, 1.0f - dist);

        putPixelAlpha({ cx + x, cy + y }, color, alpha);
        putPixelAlpha({ cx - x, cy + y }, color, alpha);
        putPixelAlpha({ cx + x, cy - y }, color, alpha);
        putPixelAlpha({ cx - x, cy - y }, color, alpha);
        putPixelAlpha({ cx + y, cy + x }, color, alpha);
        putPixelAlpha({ cx - y, cy + x }, color, alpha);
        putPixelAlpha({ cx + y, cy - x }, color, alpha);
        putPixelAlpha({ cx - y, cy - x }, color, alpha);
    };

    if (filled) {
        // just the equation for a circle lmao
        // x^2 + y^2 = r^2
        for (int dy = -radius; dy <= radius; dy++) {
            float fy = static_cast<float>(dy);
            float dxF = std::sqrt(static_cast<float>(radius * radius) - fy * fy);
            int dx = static_cast<int>(dxF);

            drawLine({ center.x - dx, center.y + dy }, { center.x + dx, center.y + dy }, color);
        }
    } else {
        // midpoint circle algorithm
        // https://en.wikipedia.org/wiki/Midpoint_circle_algorithm
        int t1 = radius / 16;
        int x = radius;
        int y = 0;
        while (x > y) {
            drawSymmetricPixels({ x, y });
            y = y + 1;
            t1 = t1 + y;
            int t2 = t1 - x;
            if (t2 >= 0) {
                t1 = t2;
                x = x - 1;
            }
        }
    }
}