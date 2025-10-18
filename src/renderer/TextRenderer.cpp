#include <fmt/base.h>
#include <fmt/format.h>

#include <renderer/text.hpp>
#include <frame.hpp>

#include <shaders/shader.hpp>
#include <shaders/text.hpp>

TextRenderer::TextRenderer() {
    textShaderProgram = shader::createProgram(textVertex, textFragment);

    if (FT_Init_FreeType(&ft)) {
        fmt::println("could not init freetype!");
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void TextRenderer::loadFont(std::string fontName) {
    FT_Face face;
    if (FT_New_Face(ft, fontName.c_str(), 0, &face)) {
        fmt::println("could not load font!");
        return;
    }

    FT_Set_Pixel_Sizes(face, 0, LOAD_SIZE);

    // load font into memory
    glPixelStorei(GL_UNPACK_ALIGNMENT, 0);

    fonts[fontName] = {};

    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            fmt::println("could not load glyph: {}", c);
            continue;
        }

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        fonts[fontName].characters[c] = Character {
            .texture = texture,
            .size = { static_cast<int>(face->glyph->bitmap.width), static_cast<int>(face->glyph->bitmap.rows) },
            .bearing = { face->glyph->bitmap_left, face->glyph->bitmap_top },
            .advance = face->glyph->advance.x
        };
    }

    fonts[fontName].ascent = face->size->metrics.ascender;
    fonts[fontName].lineHeight = face->size->metrics.height >> 6; // >> 6 = convert to pixels

    FT_Done_Face(face);
}

Vector2DF TextRenderer::getTextSize(std::string text, std::string fontName, float scale) {
    if (!fonts.contains(fontName)) {
        loadFont(fontName);
    }

    auto font = fonts[fontName];
    float lineHeight = font.lineHeight * scale;
    float maxWidth = 0.f;
    float currentWidth = 0.f;
    int lineCount = 1;

    for (char c : text) {
        if (c == '\n') {
            maxWidth = std::max(maxWidth, currentWidth);
            currentWidth = 0.f;
            lineCount++;
            continue;
        }

        auto character = font.characters[c];
        currentWidth += (character.advance >> 6) * scale;
    }

    maxWidth = std::max(maxWidth, currentWidth);
    float height = lineCount * lineHeight;

    return { maxWidth, height };
}

Vector2DF TextRenderer::drawText(Frame* frame, std::string text, std::string fontName, Vector2D pos, RGBAColor color, float pixelHeight, float rotation) {
    if (!fonts.contains(fontName)) {
        loadFont(fontName);
    }

    float scale = pixelHeight / LOAD_SIZE;

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindFramebuffer(GL_FRAMEBUFFER, frame->fbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glm::mat4 projection = glm::ortho(0.f, (float)frame->width, 0.f, (float)frame->height, -1.f, 1.f);

    auto size = getTextSize(text, fontName, scale);

    auto center = glm::vec2(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f);
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(center, 0.0f));
    model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));
    model = glm::translate(model, glm::vec3(-center.x, -center.y, 0.f));

    projection = projection * model;

    glUseProgram(textShaderProgram);
    glUniform4f(
        glGetUniformLocation(textShaderProgram, "textColor"),
        color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f
    );
    glUniformMatrix4fv(
        glGetUniformLocation(textShaderProgram, "projection"),
        1, GL_FALSE, glm::value_ptr(projection)
    );

    glUniform1i(
        glGetUniformLocation(textShaderProgram, "text"),
        0
    );

    float cursorX = pos.x;
    float baseline = pos.y;
    float maxCursorX = cursorX;

    Font font = fonts[fontName];

    int lines = 1;

    for (char character : text) {
        if (character == '\n') {
            baseline += font.lineHeight * scale;
            cursorX = pos.x;
            lines++;
            continue;
        }

        Character ch = font.characters[character];
        
        float ascent = font.ascent >> 6;

        float xPos = cursorX + ch.bearing.x * scale;
        float yPos = baseline + (ascent - ch.bearing.y) * scale;

        float w = ch.size.x * scale;
        float h = ch.size.y * scale;

        // little debugging thing
        // fmt::println("x: {}, y: {}, w: {}, h: {}, cursor: {}", xPos, yPos, w, h, cursorX);

        float vertices[6][4] = {
            { xPos,     yPos + h,   0.0f, 1.0f },            
            { xPos,     yPos,       0.0f, 0.0f },
            { xPos + w, yPos,       1.0f, 0.0f },

            { xPos,     yPos + h,   0.0f, 1.0f },
            { xPos + w, yPos,       1.0f, 0.0f },
            { xPos + w, yPos + h,   1.0f, 1.0f }           
        };

        glActiveTexture(GL_TEXTURE0);

        glBindTexture(GL_TEXTURE_2D, ch.texture);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // frame->drawTexture(ch.texture, { (int)xPos, (int)yPos }, { (int)w, (int)h });

        cursorX += (int)((ch.advance >> 6) * scale);

        if (cursorX > maxCursorX) {
            maxCursorX = cursorX;
        }
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return { maxCursorX - pos.x, font.lineHeight * scale * lines };
}
