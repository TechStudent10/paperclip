#pragma once

inline auto textVertex = R"(
#version 330 core
layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
out vec2 TexCoords;

uniform mat4 projection;

void main() {
    // gl_Position = projection * vec4(0.0, 0.0, 0.0, 1.0);
    // gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
    TexCoords = vertex.zw;
}
)";

inline auto textFragment = R"(
#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D text;
uniform vec4 textColor;

void main() {
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
    FragColor = textColor * sampled;
    // FragColor = vec4(0.0, 0.0, 0.0, 1.0);
}
)";
