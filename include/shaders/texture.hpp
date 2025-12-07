#pragma once

inline auto textureVertex = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 matrix;

void main() {
    gl_Position = matrix * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";

inline auto textureFragment = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D texture1;
uniform float opacity;

void main() {
    FragColor = vec4(texture(texture1, TexCoord).xyz, opacity);
    // FragColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);
}
)";

inline auto textureFragmentYUV = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D textureY;
uniform sampler2D textureU;
uniform sampler2D textureV;
uniform float opacity;

void main() {
    float y = texture(textureY, TexCoord).r;
    float u = texture(textureU, TexCoord).r - 0.5f;
    float v = texture(textureV, TexCoord).r - 0.5f;

    float r = y + 1.402f * v;
    float g = y - 0.344146f * u - 0.714136f * v;
    float b = y + 1.772f * u;

    FragColor = vec4(r, g, b, opacity);
    // FragColor = vec4(y, y, y, y);
    // FragColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);
}
)";
