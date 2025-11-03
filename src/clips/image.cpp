#include <clips/default/image.hpp>

#include <filesystem>
#include <stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize2.h>

#include <utils.hpp>

namespace clips {
    ImageClip::ImageClip(const std::string& path): Clip(0, 120), path(path) {
        m_properties.addProperty(
            ClipProperty::position()
        );

        m_properties.addProperty(
            ClipProperty::number()
                ->setId("scale-x")
                ->setName("Scale X")
                ->setDefaultKeyframe(Vector1D{ .number = 100 }.toString())   
        );

        m_properties.addProperty(
            ClipProperty::number()
                ->setId("scale-y")
                ->setName("Scale Y")
                ->setDefaultKeyframe(Vector1D{ .number = 100 }.toString())   
        );

        m_properties.addProperty(
            ClipProperty::number()
                ->setId("rotation")
                ->setName("Rotation (deg)")
                ->setDefaultKeyframe(Vector1D{ .number = 0 }.toString())
        );

        m_metadata.name = std::filesystem::path(path).filename();

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

        initialized = true;

        return true;
    }

    Vector2D ImageClip::getSize() {    
        return { scaledW, scaledH };
    }

    Vector2D ImageClip::getPos() {
        Vector2D position = Vector2D::fromString(m_properties.getProperty("position")->data);
        return position;
    }

    ImageClip::ImageClip(): ImageClip("") {}

    void ImageClip::render(Frame* frame) {
        initialize();

        float scaleX = (float)Vector1D::fromString(m_properties.getProperty("scale-x")->data).number / 100.f;
        float scaleY = (float)Vector1D::fromString(m_properties.getProperty("scale-y")->data).number / 100.f;
        int rotation = Vector1D::fromString(m_properties.getProperty("rotation")->data).number;
        if (scaleX <= 0 || scaleY <= 0) {
            return;
        }
        auto position = Vector2D::fromString(m_properties.getProperty("position")->data);

        scaledW = static_cast<int>(std::floor(width * scaleX));
        scaledH = static_cast<int>(std::floor(height * scaleY));

        frame->drawTexture(texture, position, { scaledW, scaledH }, VAO, VBO, EBO, rotation);
    }

    ImageClip::~ImageClip() {
        onDelete();
    }

    void ImageClip::onDelete() {
    }
}
