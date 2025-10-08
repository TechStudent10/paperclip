#pragma once

inline auto shapeVertex = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform vec2 screenSize;

vec2 normalize() {
    return vec2(
        aPos.x / (screenSize.x / 2.f) - 1,
        aPos.y / (screenSize.y / 2.f) - 1
    );
}

void main() {
    vec2 normalized = normalize();
    gl_Position = vec4(normalized.x, normalized.y, 0.0, 1.0);
}   
)";

inline auto shapeFragment = R"(
#version 330 core
out vec4 FragColor;

uniform vec4 outColor;

void main() {
    FragColor = outColor;
    //gl_CullDistance[0] = 0.5;
} 
)";
