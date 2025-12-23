#include <Application.hpp>

#include <stb_image.h>

// shoutout chatgpt
void GLDebugCallback(GLenum source,
                              GLenum type,
                              GLuint id,
                              GLenum severity,
                              GLsizei length,
                              const GLchar* message,
                              const void* userParam)
{
    // Ignore known non-critical messages (optional)
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
        return;

    fmt::print("---------------\nOpenGL Debug Message ({}): {}\n", id, message);

    const char* srcStr = "Unknown";
    switch (source) {
        case GL_DEBUG_SOURCE_API:             srcStr = "API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   srcStr = "Window System"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: srcStr = "Shader Compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     srcStr = "Third Party"; break;
        case GL_DEBUG_SOURCE_APPLICATION:     srcStr = "Application"; break;
        case GL_DEBUG_SOURCE_OTHER:           srcStr = "Other"; break;
    }

    const char* typeStr = "Other";
    switch (type) {
        case GL_DEBUG_TYPE_ERROR:               typeStr = "Error"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: typeStr = "Deprecated Behaviour"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  typeStr = "Undefined Behaviour"; break;
        case GL_DEBUG_TYPE_PORTABILITY:         typeStr = "Portability"; break;
        case GL_DEBUG_TYPE_PERFORMANCE:         typeStr = "Performance"; break;
        case GL_DEBUG_TYPE_MARKER:              typeStr = "Marker"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          typeStr = "Push Group"; break;
        case GL_DEBUG_TYPE_POP_GROUP:           typeStr = "Pop Group"; break;
    }

    const char* sevStr = "Unknown";
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:         sevStr = "HIGH"; break;
        case GL_DEBUG_SEVERITY_MEDIUM:       sevStr = "MEDIUM"; break;
        case GL_DEBUG_SEVERITY_LOW:          sevStr = "LOW"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: sevStr = "NOTIFICATION"; break;
    }

    fmt::print("Source: {}\nType: {}\n", srcStr, typeStr);
    fmt::print("Severity: {}\n\n", sevStr);
}

bool Application::initSDL() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fmt::println("SDL Init error: {}", SDL_GetError());
        return false;
    }

    SDL_SetAppMetadata("paperclip", "1.0", "com.underscored.paperclip");

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

    window = SDL_CreateWindow("Paperclip", 1200, 800, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        fmt::println("no window?");
        return false;
    }

    
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    // SDL_GL_LoadLibrary(nullptr);
    gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        fmt::println("no GL Context");
        return false;
    }
    SDL_GL_MakeCurrent(window, gl_context);
    
    SDL_GL_SetSwapInterval(1); // Enable vsync
    
    int version = gladLoadGL((GLADloadfunc)&SDL_GL_GetProcAddress);
    if (version == 0) {
        fmt::println("could NOT initalize GLAD");
        return false;
    }
    fmt::println("GLAD loaded OpenGL {}.{}", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));
    fmt::println("GL Version: {}", (const char*)glGetString(GL_VERSION));
    fmt::println("GLSL Version: {}", (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));

    // glEnable(GL_DEBUG_OUTPUT);
    // glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // Ensures callback is called immediately
    // glDebugMessageCallback(GLDebugCallback, nullptr);
    // glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    glViewport(0, 0, w, h);
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    SDL_GL_SwapWindow(window);

    if (!glGenTextures) fmt::println("glGenTextures is null!");
    if (!glBindTexture) fmt::println("glBindTexture is null!");

    // wireframe mode
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    fmt::println("SUCCESS!!!!!!!!!!!!!!!!!!!! i think");

    return true;
}

bool Application::initIcons() {
    // icons relative to resources/imgs
    std::unordered_map<IconType, std::string> paths = {
        { IconType::PlayPause, "play.png" }
    };

    int w = 0, h = 0, ch = 0;
    for (auto img : paths) {
        unsigned char* data = stbi_load(fmt::format("resources/imgs/{}", img.second).c_str(), &w, &h, &ch, 3);

        if (!data) {
            return false;
        }

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

        icons[img.first] = {
            .texture = texture,
            .w = w,
            .h = h
        };

        stbi_image_free(data);
    }

    return true;
}

void Application::setup() {
    if (!initSDL()) {
        fmt::println("could not initialize SDL");
        return;
    }

    if (!initImGui()) {
        fmt::println("could not initalize ImGui");
        return;
    }

    if (!initIcons()) {
        fmt::println("could not initialize icons");
        return;
    }
}
