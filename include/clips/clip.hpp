#pragma once

#include <map>
#include <unordered_map>
#include <easings.hpp>

#include <common.hpp>
#include <frame.hpp>
#include <utils.hpp>

enum class PropertyType {
    Text,
    Number,
    Percent,
    Dimensions,
    Color,
    Position,
    Transform,
    Dropdown
};

struct PropertyKeyframeMeta {
    animation::Easing easing;
    animation::EasingMode mode;

    void write(qn::HeapByteWriter& writer) {
        writer.writeI16((int)easing);
        writer.writeI16((int)mode);
    }

    void read(qn::ByteReader& reader) {
        easing = (animation::Easing)(reader.readI16().unwrapOr(0));
        mode = (animation::EasingMode)(reader.readI16().unwrapOr(0));
    }
};

class Clip;

class ClipProperty {
public:
    ClipProperty() {}
    Clip* clip;
    std::string id;
    std::string name;
    PropertyType type;
    std::string data; // can be interpreted by each individual clip
    std::string options; // can be interpreted by each individual property
    std::map<int, std::string> keyframes;
    std::map<int, PropertyKeyframeMeta> keyframeInfo;

    void write(qn::HeapByteWriter& writer) {
        UNWRAP_WITH_ERR(writer.writeStringU32(id));
        UNWRAP_WITH_ERR(writer.writeStringU32(name));
        writer.writeI16((int)type);
        UNWRAP_WITH_ERR(writer.writeStringU32(data));
        UNWRAP_WITH_ERR(writer.writeStringU32(options));

        writer.writeI16(keyframes.size());
        for (auto keyframe : keyframes) {
            writer.writeI16(keyframe.first);
            UNWRAP_WITH_ERR(writer.writeStringU32(keyframe.second));
        }

        writer.writeI16(keyframeInfo.size());
        for (auto info : keyframeInfo) {
            writer.writeI16(info.first);
            info.second.write(writer);
        }
    }

    void read(qn::ByteReader& reader) {
        id = reader.readStringU32().unwrapOr("");
        name = reader.readStringU32().unwrapOr("");
        type = (PropertyType)(reader.readI16().unwrapOr(0));
        data = reader.readStringU32().unwrapOr("");
        options = reader.readStringU32().unwrapOr("");

        auto keyframesSize = reader.readI16().unwrapOr(0);
        for (int i = 0; i < keyframesSize; i++) {
            auto key = reader.readI16().unwrapOr(0);
            auto keyframe = reader.readStringU32().unwrapOr("{}");
            keyframes[key] = keyframe;
        }

        auto infoSize = reader.readI16().unwrapOr(0);
        for (int j = 0; j < infoSize; j++) {
            auto key = reader.readI16().unwrapOr(0);
            PropertyKeyframeMeta info;
            info.read(reader);
            keyframeInfo[key] = info;
        }
    }

    ClipProperty* setId(std::string_view id) {
        this->id = id;
        return this;
    }
    ClipProperty* setName(std::string_view name) {
        this->name = name;
        return this;
    }
    ClipProperty* setType(PropertyType type) {
        this->type = type;
        return this;
    }
    ClipProperty* setDefaultKeyframe(std::string_view data) {
        this->keyframes[0] = data;
        return this;
    }
    ClipProperty* setOptions(std::string_view options) {
        this->options = options;
        return this;
    }

    void drawProperty();

    static ClipProperty* create() {
        return new ClipProperty();
    }

    // defaults that most clips are gonna use (but can override)

    static ClipProperty* text() {
        return ClipProperty::create()
            ->setType(PropertyType::Text)
            ->setId("text")
            ->setName("Text")
            ->setDefaultKeyframe("Hello World!");
    }

    static ClipProperty* number() {
        return ClipProperty::create()
            ->setType(PropertyType::Number);
    }

    static ClipProperty* dropdown() {
        return ClipProperty::create()
            ->setType(PropertyType::Dropdown);
    }

    static ClipProperty* dimensions() {
        return ClipProperty::create()
            ->setType(PropertyType::Dimensions)
            ->setId("dimensions")
            ->setName("Dimensions")
            ->setDefaultKeyframe(Dimensions{ .size = { .x = 500, .y = 500 }, .transform = { .position = { .x = 10, .y = 10 } } }.toString());
    }

    static ClipProperty* color() {
        return ClipProperty::create()
            ->setType(PropertyType::Color)
            ->setId("color")
            ->setName("Color")
            ->setDefaultKeyframe(RGBAColor{ .r = 0, .g = 0, .b = 0, .a = 255 }.toString());
    }
    
    static ClipProperty* position() {
        return ClipProperty::create()
            ->setType(PropertyType::Position)
            ->setId("position")
            ->setName("Position")
            ->setDefaultKeyframe(Vector2D{ .x = 0, .y = 0 }.toString());
    }

    static ClipProperty* transform() {
        return ClipProperty::create()
            ->setType(PropertyType::Transform)
            ->setId("transform")
            ->setName("Transform")
            ->setDefaultKeyframe(Transform{
                .position = { .x = 0, .y = 0 },
                .anchorPoint = { 0.5f ,0.5f },
                .rotation = 0,
                .pitch = 0,
                .roll = 0
            }.toString());
    }
};

struct ClipProperties {
private:
    std::map<std::string, std::shared_ptr<ClipProperty>> properties;
    Clip* clip;

    friend class Clip;
public:
    void addProperty(ClipProperty* property);
    std::map<std::string, std::shared_ptr<ClipProperty>> getProperties() { return properties; }
    std::shared_ptr<ClipProperty> getProperty(std::string id);
    void setKeyframe(std::string id, int frame, std::string data);
    void setKeyframeMeta(std::string id, int frame, PropertyKeyframeMeta data);

    void write(qn::HeapByteWriter& writer) {
        writer.writeI16(properties.size());
        for (auto prop : properties) {
            UNWRAP_WITH_ERR(writer.writeStringU32(prop.first));
            prop.second->write(writer);
        }
    }

    void read(qn::ByteReader& reader) {
        auto size = reader.readI16().unwrapOr(0);
        for (int i = 0; i < size; i++) {
            auto key = reader.readStringU32().unwrapOr("");
            auto prop = std::make_shared<ClipProperty>();
            prop->read(reader);
            properties[key] = prop;
        }
    }
};

struct ClipMetadata {
    std::string name;
    
    void write(qn::HeapByteWriter& writer) {
        UNWRAP_WITH_ERR(writer.writeStringU32(name));
    }
    void read(qn::ByteReader& reader) {
        name = reader.readStringU32().unwrapOr("");
    }
};

// lesson learned: always use enum class.
// - underscored
enum class ClipType {
    Rectangle,
    Circle,
    Text,
    Image,
    Video,
    Audio,
    None
};

class Clip {
protected:
    Clip(int startFrame, int duration): Clip(startFrame, duration, utils::generateUUID()) {}
public:
    Clip(): Clip(0, 60) {}
    Clip(std::string uID): Clip(0, 60, uID) {}

    Clip(int startFrame, int duration, std::string uID): uID(uID), startFrame(startFrame), duration(duration) {
        m_properties.clip = this;
    }

    ClipProperties m_properties;
    ClipMetadata m_metadata;

    int startFrame;
    int duration;

    std::string uID; // unique ID
    std::vector<std::string> linkedClips; // linked clip IDs

    virtual void render(Frame* frame) {}
    virtual void onDelete() {}

    virtual void write(qn::HeapByteWriter& writer) {
        writer.writeI16((int)getType());
        fmt::println("wrote {} as type", (int)getType());
        m_properties.write(writer);
        m_metadata.write(writer);
        writer.writeI16(startFrame);
        writer.writeI16(duration);
    }

    virtual void read(qn::ByteReader& reader) {
        m_properties.read(reader);
        m_metadata.read(reader);
        startFrame = reader.readI16().unwrapOr(0);
        duration = reader.readI16().unwrapOr(0);
    }

    virtual ClipType getType() { return ClipType::None; }
    virtual Vector2D getSize() { return { 0, 0 }; }
    virtual Vector2D getPos() { return { 0, 0 }; }
    
    virtual GLuint getPreviewTexture(int frame) {
        fmt::println("unimplemented");
        return 0;
    }
    virtual Vector2D getPreviewSize() { return { 0, 0 }; }
};