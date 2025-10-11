#include <fstream>
#include <iostream>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#include <fmt/base.h>
#include <fmt/format.h>

#include <renderer/text.hpp>
#include <frame.hpp>

TextRenderer::TextRenderer() {}

void TextRenderer::loadFont(std::string fontName) {
    std::ifstream fontFile(fontName, std::ios::binary);
    if (!fontFile) {
        std::cerr << "oops font not found!\n";
        return;
    }

    fontBuffers[fontName] = std::vector<unsigned char>((std::istreambuf_iterator<char>(fontFile)), std::istreambuf_iterator<char>());
}

float TextRenderer::drawText(Frame* frame, std::string text, std::string fontName, Vector2D pos, RGBAColor color, float pixelHeight) {
    if (!fontBuffers.contains(fontName)) {
        loadFont(fontName);
    }

    auto fontBuffer = fontBuffers[fontName];

    stbtt_fontinfo font;
    stbtt_InitFont(&font, fontBuffer.data(), 0);
    float scale = stbtt_ScaleForPixelHeight(&font, pixelHeight);

    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);
    int baseline = (int)(ascent * scale);

    int cursor = pos.x;
    for (char character : text) {
        int glyph = stbtt_FindGlyphIndex(&font, character);

        int advance, lsb,
            x0, y0, x1, y1;
        
        stbtt_GetGlyphHMetrics(&font, glyph, &advance, &lsb);
        stbtt_GetGlyphBitmapBox(&font, glyph, scale, scale, &x0, &y0, &x1, &y1);

        int dw = x1 - x0;
        int dh = y1 - y0;

        // fmt::println("dw: {}, dh: {}", dw, dh);
        if (dw * dh < 0) return cursor - pos.x;

        std::vector<unsigned char> bitmap(dw * dh);
        stbtt_MakeGlyphBitmap(&font, bitmap.data(), dw, dh, dw, scale, scale, glyph);
        stbtt_MakeGlyphBitmapSubpixel(&font, bitmap.data(), dw, dh, dw, scale, scale, 0.f, 0.f, glyph);
        for (int row = 0; row < dh; row++) {
            for (int col = 0; col < dw; col++) {
                unsigned char alpha = bitmap[row * dw + col];
                if (alpha > 0) {
                    int dstX = cursor + x0 + col;
                    int dstY = pos.y + baseline + y0 + row;
                    if (dstX >= 0 && dstX < frame->width && dstY >= 0 && dstY < frame->height) {
                        frame->putPixel({dstX, dstY}, color);
                    }
                }
            }
        }

        cursor += (int)(advance * scale);

        if (&character != &text.back()) {
            int kerning = stbtt_GetGlyphKernAdvance(&font, glyph, stbtt_FindGlyphIndex(&font, *( &character + 1 )));
            cursor += (int)(kerning * scale);
        }
    }

    return cursor - pos.x;
}
