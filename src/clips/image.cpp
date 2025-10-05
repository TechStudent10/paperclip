#include <clips/default/image.hpp>

#define STB_IMAGE_IMPLEMENTATION
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

        m_metadata.name = path;

        if (path.empty()) return;

        initialize();
    }

    bool ImageClip::initialize() {
        if (initialized) {
            return true;
        }

        imageData = stbi_load(path.c_str(), &width, &height, NULL, 3);
        if (imageData == NULL) {
            fmt::println("could not load image {}", path);
            return false;
        }

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

    ImageClip::ImageClip(): ImageClip("") {
        fmt::println("construct: {} ({})", path, sizeof(imageData));
    }

    void ImageClip::render(Frame* frame) {
        initialize();

        float scaleX = (float)Vector1D::fromString(m_properties.getProperty("scale-x")->data).number / 100.f;
        float scaleY = (float)Vector1D::fromString(m_properties.getProperty("scale-y")->data).number / 100.f;
        int rotation = Vector1D::fromString(m_properties.getProperty("rotation")->data).number;
        if (scaleX <= 0 || scaleY <= 0) {
            return;
        }
        auto position = Vector2D::fromString(m_properties.getProperty("position")->data);

        int currentScaledW = static_cast<int>(std::floor(width * scaleX));
        int currentScaledH = static_cast<int>(std::floor(height * scaleY));
        
        if (currentScaledW != scaledW || currentScaledH != scaledH) {
            // fmt::println("scale change!");
            scaledW = currentScaledW;
            scaledH = currentScaledH;
            
            resizedData.resize(scaledW * scaledH * 3);
            auto res = stbir_resize_uint8_linear(
                imageData, width, height, width * 3,
                resizedData.data(), scaledW, scaledH, scaledW * 3,
                stbir_pixel_layout::STBIR_RGB
            );
            // fmt::println("{}", res == 0);
        }

        utils::drawImage(frame, resizedData.data(), { scaledW, scaledH }, position, rotation);
    }

    ImageClip::~ImageClip() {
        onDelete();
    }

    void ImageClip::onDelete() {
        if (imageData) {
            fmt::println("deletion!");
            stbi_image_free(imageData);
        }
    }
}
