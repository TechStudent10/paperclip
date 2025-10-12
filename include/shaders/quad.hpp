#pragma once

inline auto quadVertex = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
out vec2 TexCoord;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    TexCoord = aUV;
}
)";

inline auto quadFragment = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D uTexture;

void main() {
    FragColor = texture(uTexture, TexCoord);
}
)";
