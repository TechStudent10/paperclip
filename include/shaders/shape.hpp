#pragma once

inline auto shapeVertex = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 matrix;

void main() {
    gl_Position = matrix * vec4(aPos, 1.0);
}   
)";

inline auto shapeFragment = R"(
#version 330 core
out vec4 FragColor;

uniform vec4 outColor;
uniform mat4 matrix;

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
