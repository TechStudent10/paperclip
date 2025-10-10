#pragma once

inline auto textureVertex = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform vec2 screenSize;
out vec2 TexCoord;

vec2 normalize() {
    return vec2(
        aPos.x / (screenSize.x / 2.f) - 1,
        aPos.y / (screenSize.y / 2.f) - 1
    );
}

void main() {
    vec2 normalized = normalize();
    gl_Position = vec4(normalized, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

inline auto textureFragment = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D texture1;

void main() {
    FragColor = texture(texture1, TexCoord);
    // FragColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);
}
)";
