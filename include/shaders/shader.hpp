#pragma once

#include <glad/include/glad/gl.h>
#include <SDL3/SDL_opengl.h>

namespace shader {
    GLuint compileShader(GLenum type, const char* source);
    GLuint createProgram(const char* vertex, const char* fragment);
}
