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

void main() {
    FragColor = texture(texture1, TexCoord);
    // FragColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);
}
)";
