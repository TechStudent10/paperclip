#pragma once

#include <map>
#include <unordered_map>
#include <easings.hpp>

#include <common.hpp>
#include <frame.hpp>
#include <utils.hpp>

#include <Geode/Result.hpp>

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

// this is weird however it allows me
// to use template arguments in ClipProperty
// :double_thumbs:
class ClipPropertyBase : public std::enable_shared_from_this<ClipPropertyBase> {
public:
    ClipPropertyBase() {}

    Clip* clip;
    std::string id = "";
    std::string name = "";
    PropertyType type;
    std::map<int, PropertyKeyframeMeta> keyframeInfo;

    virtual void drawProperty() {}
    void _drawProperty();

    virtual void updateData(float progress, int previous, int next) {}
    virtual std::vector<int> getKeyframes() { fmt::println("calling the virtual"); return {}; }
    virtual void writeData(qn::HeapByteWriter& writer) {}
    virtual void readData(qn::ByteReader& reader) {}

    virtual void addKeyframe(int frame) {}
    virtual void removeKeyframe(int frame) {}

    void processKeyframe(int frame);

    void write(qn::HeapByteWriter& writer) {
        writer.writeI16(debug((int)type));
        UNWRAP_WITH_ERR(writer.writeStringU32(id));
        UNWRAP_WITH_ERR(writer.writeStringU32(name));

        writer.writeI64(getKeyframes().size());
        // writes the keyframes
        writeData(writer);

        writer.writeI16(keyframeInfo.size());
        for (auto info : keyframeInfo) {
            writer.writeI16(info.first);
            info.second.write(writer);
        }
    }

    void read(qn::ByteReader& reader) {
        id = reader.readStringU32().unwrapOr("");
        name = reader.readStringU32().unwrapOr("");
        
        // read the keyframes
        readData(reader);

        auto infoSize = reader.readI16().unwrapOr(0);
        for (int j = 0; j < infoSize; j++) {
            auto key = reader.readI16().unwrapOr(0);
            PropertyKeyframeMeta info;
            info.read(reader);
            keyframeInfo[key] = info;
        }
    }

    virtual ~ClipPropertyBase() = default;
    
    template<typename T, std::enable_if_t<std::is_base_of_v<ClipPropertyBase, T>, bool> = true>
    std::shared_ptr<T> as() {
        return std::static_pointer_cast<T>(shared_from_this());
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

    Clip(int startFrame, int duration, std::string uID): uID(uID), startFrame(startFrame), duration(duration) {}

    std::unordered_map<std::string, std::shared_ptr<ClipPropertyBase>> m_properties;
    ClipMetadata m_metadata;

    int startFrame;
    int duration;

    int fadeInFrame = 0; // fade-in frame
    int fadeOutFrame = 0; // fade-out frame

    float opacity = 0;

    std::string uID; // unique ID
    std::vector<std::string> linkedClips; // linked clip IDs

    template<typename T>
    geode::Result<std::shared_ptr<T>, std::string> getProperty(const std::string& id) {
        if (!m_properties.contains(id)) {
            return geode::Err(fmt::format("Could not find property with id `{}`", id));
        }

        return geode::Ok(m_properties[id]->as<T>());
    }

    void addProperty(std::shared_ptr<ClipPropertyBase> property) {
        property->clip = this;
        m_properties[property->id] = property;
    }

    void dispatchChange();

    virtual void render(Frame* frame) {}
    virtual void onDelete() {}

    virtual void write(qn::HeapByteWriter& writer);

    virtual void read(qn::ByteReader& reader);

    virtual ClipType getType() { return ClipType::None; }
    virtual Vector2D getSize() { return { 0, 0 }; }
    virtual Vector2D getPos() { return { 0, 0 }; }
    
    virtual GLuint getPreviewTexture(int frame) {
        fmt::println("unimplemented");
        return 0;
    }
    virtual Vector2D getPreviewSize() { return { 0, 0 }; }
};

#include <imgui.h>

template<typename T>
class ClipProperty : public ClipPropertyBase {
public:
    ClipProperty() {}
    T data;
    std::map<int, T> keyframes;

    void addKeyframe(int frame) override {
        keyframes[frame] = data;
    }

    void removeKeyframe(int frame) override {
        keyframes.erase(frame);
    }

    std::vector<int> getKeyframes() override {
        std::vector<int> res;

        for (auto [key, _] : keyframes) {
            res.push_back(key);
        }

        return res;
    }

    void setData(T data, int keyframe) {
        if (keyframes.size() == 1) {
            keyframe = 0;
        }

        keyframes[keyframe] = data;
        // this->data = data;
        clip->dispatchChange();
    }

    std::shared_ptr<ClipProperty<T>> setId(std::string_view id) {
        this->id = id;
        return as<ClipProperty<T>>();
    }
    std::shared_ptr<ClipProperty<T>> setName(std::string_view name) {
        this->name = name;
        return as<ClipProperty<T>>();
    }
    std::shared_ptr<ClipProperty<T>> setType(PropertyType type) {
        this->type = type;
        return as<ClipProperty<T>>();
    }
};
