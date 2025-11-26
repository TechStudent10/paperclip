#include <clips/default/image.hpp>

#include <filesystem>
#include <stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize2.h>

#include <utils.hpp>

#include <clips/properties/transform.hpp>
#include <clips/properties/number.hpp>

namespace clips {
    ImageClip::ImageClip(const std::string& path): Clip(0, 120), path(path) {
        addProperty(
            std::make_shared<TransformProperty>()
        );

        // scale x (default = 100)
        addProperty(
            std::make_shared<NumberProperty>()
                ->setId("scale-x")
                ->setName("Scale X")
        );

        // scale y (default = 100)
        addProperty(
            std::make_shared<NumberProperty>()
                ->setId("scale-y")
                ->setName("Scale Y")
        );

        m_metadata.name = std::filesystem::path(path).filename().string();

        glGenBuffers(1, &VBO);
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &EBO);

        if (path.empty()) return;

        initialize();
    }

    bool ImageClip::initialize() {
        if (initialized) {
            return true;
        }

        stbi_set_flip_vertically_on_load(true);

        imageData = stbi_load(path.c_str(), &width, &height, NULL, 3);
        if (imageData == NULL) {
            fmt::println("could not load image {}", path);
            return false;
        }

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, imageData);

        stbi_image_free(imageData);

        previewFrame = std::make_shared<Frame>(
            width,
            height
        );
        previewFrame->clearFrame();
        previewFrame->drawTexture(texture, { width, height }, { .position = { 0, 0 } }, VAO, VBO, EBO);

        initialized = true;

        return true;
    }

    Vector2D ImageClip::getSize() {    
        return { scaledW, scaledH };
    }

    Vector2D ImageClip::getPos() {
        Vector2D position = getProperty<TransformProperty>("transform").unwrap()->data.position;
        return position;
    }

    ImageClip::ImageClip(): ImageClip("") {}

    void ImageClip::render(Frame* frame) {
        initialize();

        float scaleX = (float)getProperty<NumberProperty>("scale-x").unwrap()->data / 100.f;
        float scaleY = (float)getProperty<NumberProperty>("scale-y").unwrap()->data / 100.f;;
        if (scaleX <= 0 || scaleY <= 0) {
            return;
        }

        auto transform = getProperty<TransformProperty>("transform").unwrap()->data;

        scaledW = static_cast<int>(std::floor(width * scaleX));
        scaledH = static_cast<int>(std::floor(height * scaleY));

        frame->drawTexture(texture, { scaledW, scaledH }, transform, VAO, VBO, EBO);
    }

    GLuint ImageClip::getPreviewTexture(int) {
        return previewFrame->textureID;
    }

    Vector2D ImageClip::getPreviewSize() { return { width, height }; }

    ImageClip::~ImageClip() {
        onDelete();
    }

    void ImageClip::onDelete() {
    }
}
