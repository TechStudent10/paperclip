#include <shaders/shader.hpp>
#include <fmt/base.h>

namespace shader {
    GLuint compileShader(GLenum type, const char* source) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        int success = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            fmt::println("no shader comp! ({})", infoLog);
        }

        return shader;
        return 0;
    }

    GLuint createProgram(const char* vertex, const char* fragment) {
        GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertex);
        GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragment);

        GLuint program = glCreateProgram();

        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        int success = 0;
        glGetProgramiv(program, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(program, 512, nullptr, infoLog);
            fmt::println("no program link! ({})", infoLog);
        }

        return program;
        return 0;
    }
}
