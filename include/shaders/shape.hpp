#pragma once

inline auto shapeVertex = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

void main()
{
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}
)";

inline auto shapeFragment = R"(
#version 330 core
out vec4 FragColor;

uniform vec4 outColor;

void main()
{
    FragColor = outColor;
} 
)";
