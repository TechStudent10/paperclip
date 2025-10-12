#pragma once

inline auto shapeVertex = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform vec2 screenSize;

out vec2 vertPos;

vec2 normalize() {
    return vec2(
        aPos.x / (screenSize.x / 2.f) - 1,
        aPos.y / (screenSize.y / 2.f) - 1
    );
}

void main() {
    vertPos = aPos.xy;
    vec2 normalized = normalize();
    gl_Position = vec4(normalized.x, normalized.y, 0.0, 1.0);
}   
)";

inline auto shapeFragment = R"(
#version 330 core
out vec4 FragColor;

in vec2 vertPos;

uniform vec4 outColor;
uniform vec2 screenSize;

// different types/circle stuff
uniform int type;
uniform float radius;
uniform vec2 center;

void main() {
    switch (type) {
        case 1:    
            if (sqrt(pow(gl_FragCoord.x - center.x, 2) + pow(gl_FragCoord.y - center.y, 2)) > radius)
                discard;
            break;
        default: break;
    }

    FragColor = outColor;
} 
)";
