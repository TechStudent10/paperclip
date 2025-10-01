#pragma once
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>

extern "C" {
#include <libavutil/opt.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#include <glaze/glaze.hpp>

#include <format/vpf.hpp>
#include <miniaudio.h>

#include <easings.hpp>

#include <mlt++/Mlt.h>

#include <format/binary/reader.hpp>
#include <format/binary/writer.hpp>

#include <fmt/base.h>
#include <fmt/format.h>

#define JSON_METHODS(className) public: \
    std::string toString() { \
        auto res = glz::write_json(this).value_or("{}"); \
        return res; \
    } \
    \
    static className fromString(std::string string) { \
        auto obj = glz::read_json<className>(string); \
        if (obj) { \
            return obj.value(); \
        } else { \
            return {}; \
        } \
    }

struct Vector2D {
    int x;
    int y;

    JSON_METHODS(Vector2D)

    void write(qn::HeapByteWriter& writer) {
        writer.writeI16(x);
        writer.writeI16(y);
    }

    void read(qn::ByteReader& reader) {
        x = reader.readI16().unwrapOr(0);
        y = reader.readI16().unwrapOr(0);
    }
};

struct Vector1D {
    int number;

    JSON_METHODS(Vector1D);
};

struct Dimensions {
    Vector2D pos;
    Vector2D size;
    
    JSON_METHODS(Dimensions)
};

struct RGBAColor {
    int r; int g; int b; int a = 255;

    JSON_METHODS(RGBAColor)
};

struct DropdownOptions {
    std::vector<std::string> options;

    JSON_METHODS(DropdownOptions)
};

class Frame {
protected:
public:
    std::vector<unsigned char> imageData;
    int width;
    int height;
    Frame(int width, int height) : width(width), height(height) {
        this->imageData = std::vector<unsigned char>(width * height * 4);
        this->imageData.resize(width * height * 4);
        clearFrame();
    }

    void clearFrame() {
        std::fill(this->imageData.begin(), this->imageData.end(), 255);
    }

    void putPixel(Vector2D position, RGBAColor color);
    RGBAColor getPixel(Vector2D position);

    void drawRect(Dimensions dimensions, RGBAColor color);
    void drawLine(Vector2D start, Vector2D end, RGBAColor color, int thickness = 1);
    void drawCircle(Vector2D center, int radius, RGBAColor color, bool filled = true);

    const std::vector<unsigned char>& getFrameData() const;
};

enum class PropertyType {
    Text,
    Number,
    Percent,
    Dimensions,
    Color,
    Position,
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

class ClipProperty {
public:
    ClipProperty() {}
    std::string id;
    std::string name;
    PropertyType type;
    std::string data; // can be interpreted by each individual clip
    std::string options; // can be interpreted by each individual property
    std::map<int, std::string> keyframes;
    std::map<int, PropertyKeyframeMeta> keyframeInfo;

    void write(qn::HeapByteWriter& writer) {
        writer.writeStringU32(id);
        writer.writeStringU32(name);
        writer.writeI16((int)type);
        writer.writeStringU32(data);
        writer.writeStringU32(options);

        writer.writeI16(keyframes.size());
        for (auto keyframe : keyframes) {
            writer.writeI16(keyframe.first);
            writer.writeStringU32(keyframe.second);
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
            ->setDefaultKeyframe(Dimensions{ .pos = { .x = 10, .y = 10 }, .size = { .x = 500, .y = 500 } }.toString());
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
};

struct ClipProperties {
private:
    std::map<std::string, std::shared_ptr<ClipProperty>> properties;

public:
    void addProperty(ClipProperty* property);
    std::map<std::string, std::shared_ptr<ClipProperty>> getProperties() { return properties; }
    std::shared_ptr<ClipProperty> getProperty(std::string id);
    void setKeyframe(std::string id, int frame, std::string data);
    void setKeyframeMeta(std::string id, int frame, PropertyKeyframeMeta data);

    void write(qn::HeapByteWriter& writer) {
        writer.writeI16(properties.size());
        for (auto prop : properties) {
            writer.writeStringU32(prop.first);
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
        writer.writeStringU32(name);
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
    None
};

class Clip {
protected:
    Clip(int startFrame, int duration): startFrame(startFrame), duration(duration) {};
public:
    Clip(): Clip(0, 60) {}
    ClipProperties m_properties;
    ClipMetadata m_metadata;
    int startFrame;
    int duration;
    virtual void render(Frame* frame) {}
    virtual void onDelete() {}

    virtual void write(qn::HeapByteWriter& writer) {
        writer.writeI16((int)getType());
        fmt::print("wrote {} as type", (int)getType());
        m_properties.write(writer);
        m_metadata.write(writer);
        writer.writeI16(startFrame);
        writer.writeI16(duration);
    }

    virtual void read(qn::ByteReader& reader) {
        m_properties.read(reader);
        m_metadata.read(reader);
        auto res = reader.readI16();
        startFrame = res.unwrapOr(0);
        duration = reader.readI16().unwrapOr(0);
    }

    virtual ClipType getType() { return ClipType::None; }
    virtual Vector2D getSize() { return { 0, 0 }; }
    virtual Vector2D getPos() { return { 0, 0 }; }
};


class Rectangle : public Clip {
public:
    Rectangle();
    void render(Frame* frame) override;

    ClipType getType() override { return ClipType::Rectangle; }
    Vector2D getSize() override;
    Vector2D getPos() override;
};

class Circle : public Clip {
public:
    Circle();
    void render(Frame* frame) override;

    ClipType getType() override { return ClipType::Circle; }
    Vector2D getSize() override;
    Vector2D getPos() override;
};

class Text : public Clip {
public:
    float width = 0.f;

    Text();
    void render(Frame* frame) override;

    ClipType getType() override { return ClipType::Text; }
    Vector2D getSize() override;
    Vector2D getPos() override;
};

class ImageClip : public Clip {
private:
    unsigned char* imageData;
    std::vector<unsigned char> resizedData;

    int width, height;
    int scaledW = 0, scaledH = 0;

    bool initialize();
public:
    bool initialized = false;
    std::string path;
    ImageClip();
    ImageClip(const std::string& path);
    ~ImageClip();

    ImageClip& operator=( const ImageClip& ) = delete;

    void render(Frame* frame) override;
    void onDelete() override;

    ClipType getType() override { return ClipType::Image; }

    void write(qn::HeapByteWriter& writer) override {
        Clip::write(writer);
        writer.writeStringU32(path);
    }

    void read(qn::ByteReader& reader) override {
        Clip::read(reader);
        path = reader.readStringU32().unwrapOr("");
    }

    Vector2D getSize() override;
    Vector2D getPos() override;
};

class VideoClip : public Clip {
private:
    bool decodeFrame(int frameNumber);

    bool initialize();

    mlt_profile profile;
    mlt_producer producer;

    int width = 0, height = 0, fps = 0;
    std::vector<unsigned char> vidFrame;
    std::string path;
    bool initialized = false;
public:
    VideoClip(const std::string& path);
    VideoClip();
    ~VideoClip();

    ClipType getType() override { return ClipType::Video; }

    void render(Frame* frame) override;
    
    void write(qn::HeapByteWriter& writer) override {
        Clip::write(writer);
        writer.writeStringU32(path);
    }

    void read(qn::ByteReader& reader) override {
        Clip::read(reader);
        path = reader.readStringU32().unwrapOr("");
    }

    Vector2D getSize() override;
    Vector2D getPos() override;
};

class AudioClip : public Clip {
private:
    ma_sound sound;
    std::string path;
    bool initialized = false;

    bool initalize();
public:
    bool playing = false;
    AudioClip(const std::string& path);
    AudioClip();
    ~AudioClip();

    void play();
    void seekToSec(float seconds);
    void setVolume(float volume);
    void stop();

    float getCursor();
    
    void write(qn::HeapByteWriter& writer) override {
        Clip::write(writer);
        writer.writeStringU32(path);
    }

    void read(qn::ByteReader& reader) override {
        Clip::read(reader);
        path = reader.readStringU32().unwrapOr("");
    }
};

class VideoRenderer;

class VideoTrack {
private:
    std::vector<std::shared_ptr<Clip>> clips;
public:
    VideoTrack() {
        clips = {};
    }

    void addClip(std::shared_ptr<Clip> clip) {
        clips.push_back(clip);
    }

    void removeClip(std::shared_ptr<Clip> clip) {
        clips.erase(std::remove(clips.begin(), clips.end(), clip), clips.end());
    }

    std::vector<std::shared_ptr<Clip>> getClips() {
        return clips;
    }

    void render(Frame* frame, int currentFrame);

    void write(qn::HeapByteWriter& writer) {
        writer.writeI16(clips.size());
        for (auto clip : clips) {
            clip->write(writer);
        }
    }

    void read(qn::ByteReader& reader) {
        auto size = reader.readI16().unwrapOr(0);
        for (int i = 0; i < size; i++) {
            auto clipType = (ClipType)(reader.readI16().unwrapOr(0));
            fmt::print("love me some {}", (int)clipType);
            std::shared_ptr<Clip> clip;

            switch (clipType) {
                case ClipType::Rectangle:
                    fmt::print("making rect");
                    clip = std::make_shared<Rectangle>();
                    break;
                case ClipType::Circle:
                    fmt::print("making circ");
                    clip = std::make_shared<Circle>();
                    break;
                case ClipType::Text:
                    fmt::print("making text");
                    clip = std::make_shared<Text>();
                    break;
                case ClipType::Image:
                    fmt::print("making imag");
                    clip = std::make_shared<ImageClip>();
                    break;
                case ClipType::Video:
                    fmt::print("making vide");
                    clip = std::make_shared<VideoClip>();
                    break;
                default:
                    fmt::print("making god knows what");
                    clip = std::make_shared<Clip>();
                    break;
            }

            clip->read(reader);
            fmt::print("{}", clip->m_metadata.name);
            clips.push_back(clip);
        }
    }
};

class AudioTrack {
private:
    std::vector<std::shared_ptr<AudioClip>> clips;
public:
    AudioTrack() {
        clips = {};
    }

    void addClip(std::shared_ptr<AudioClip> clip) {
        clips.push_back(clip);
    }

    void removeClip(std::shared_ptr<AudioClip> clip) {
        clips.erase(std::remove(clips.begin(), clips.end(), clip), clips.end());
    }

    std::vector<std::shared_ptr<AudioClip>> getClips() {
        return clips;
    }

    void processTime();
    void onPlay();
    void onStop();

    void write(qn::HeapByteWriter& writer) {
        writer.writeI16(clips.size());
        for (auto clip : clips) {
            clip->write(writer);
        }
    }

    void read(qn::ByteReader& reader) {
        auto size = reader.readI16().unwrapOr(0);
        for (int i = 0; i < size; i++) {
            auto clip = std::make_shared<AudioClip>();
            clip->read(reader);
            clips.push_back(clip);
        }
    }
};

struct ExtClipMetadata {
    std::string filePath;
    int frameCount;

    void write(qn::HeapByteWriter& writer) {
        writer.writeStringU32(filePath);
        writer.writeI16(frameCount);
    }

    void read(qn::ByteReader& reader) {
        filePath = reader.readStringU32().unwrapOr("");
        frameCount = reader.readI16().unwrapOr(0);
    }
};

class Video {
protected:
public:
    int framerate;

    Vector2D resolution;
    std::vector<std::shared_ptr<VideoTrack>> videoTracks;
    std::vector<std::shared_ptr<AudioTrack>> audioTracks;
    std::vector<ExtClipMetadata> clipPool;
    std::vector<ExtClipMetadata> imagePool;
    std::vector<ExtClipMetadata> audioClipPool;
    Video(int framerate, Vector2D resolution): framerate(framerate), resolution(resolution) {}
    Video(): Video(30, {1920, 1080}) {}

    int frameCount = 0;
    void recalculateFrameCount();

    int addTrack(std::shared_ptr<VideoTrack> track) {
        videoTracks.push_back(track);
        return videoTracks.size() - 1;
    }
    void addClip(int trackIdx, std::shared_ptr<Clip> clip);

    const std::vector<std::shared_ptr<VideoTrack>>& getTracks() const { return videoTracks; }

    void render(VideoRenderer* renderer);
    Frame* renderAtFrame(int frame);
    void renderIntoFrame(int frameNum, std::shared_ptr<Frame> frame);

    int getFPS() { return framerate; }
    Vector2D getResolution() { return resolution; }

    int frameForTime(float time);
    float timeForFrame(int time);

    void write(qn::HeapByteWriter& writer) {
        writer.writeI16(framerate);
        resolution.write(writer);
        
        writer.writeI16(videoTracks.size());
        for (auto track : videoTracks) {
            track->write(writer);
        }

        writer.writeI16(audioTracks.size());
        for (auto audTrack : audioTracks) {
            audTrack->write(writer);
        }

        writer.writeI16(clipPool.size());
        for (auto clip : clipPool) {
            clip.write(writer);
        }

        writer.writeI16(audioClipPool.size());
        for (auto audClip : audioClipPool) {
            audClip.write(writer);
        }

        writer.writeI16(imagePool.size());
        for (auto imgClip : imagePool) {
            imgClip.write(writer);
        }
    }

    void read(qn::ByteReader& reader) {
        framerate = reader.readI16().unwrapOr(0);
        resolution.read(reader);

        auto vidSize = reader.readI16().unwrapOr(0);
        for (int i = 0; i < vidSize; i++) {
            auto track = std::make_shared<VideoTrack>();
            track->read(reader);
            videoTracks.push_back(track);
        }

        auto audSize = reader.readI16().unwrapOr(0);
        for (int i = 0; i < audSize; i++) {
            auto track = std::make_shared<AudioTrack>();
            track->read(reader);
            audioTracks.push_back(track);
        }

        auto poolSize = reader.readI16().unwrapOr(0);
        for (int i = 0; i < poolSize; i++) {
            ExtClipMetadata meta;
            meta.read(reader);
            clipPool.push_back(meta);
        }

        auto audPoolSize = reader.readI16().unwrapOr(0);
        for (int i = 0; i < audPoolSize; i++) {
            ExtClipMetadata meta;
            meta.read(reader);
            audioClipPool.push_back(meta);
        }

        auto imgPoolSize = reader.readI16().unwrapOr(0);
        for (int i = 0; i < imgPoolSize; i++) {
            ExtClipMetadata meta;
            meta.read(reader);
            imagePool.push_back(meta);
        }
    }
};

class TextRenderer {
protected:
    std::unordered_map<std::string, std::vector<unsigned char>> fontBuffers;
public:
    TextRenderer();
    void loadFont(std::string fontName);
    float drawText(Frame* frame, std::string text, std::string fontName, Vector2D pos, RGBAColor color, float pixelHeight = 48.f);
};

class VideoRenderer {
protected:
    AVFormatContext* fmt_ctx = nullptr;
    const AVCodec* codec = nullptr;
    AVStream* stream = nullptr;
    AVStream* audio_stream = nullptr;
    AVCodecContext* codec_ctx = nullptr;
    AVCodecContext* audio_codec_ctx = nullptr;
    AVFrame* frame = nullptr;
    SwsContext* sws_ctx = nullptr;

    int width;
    int height;

    int fps;

    int currentFrame = 0;
    int audioCurrentFrame = 0;

    std::string filename;
public:
    VideoRenderer(std::string filename, int width, int height, int fps);
    void addFrame(std::shared_ptr<Frame> frame);
    void addAudio(std::vector<float>& data);
    void finish();
};

struct AudioRenderFile {
    ma_decoder decoder;
    std::string_view path;
    float startTime;
    float endTime;
    std::map<int, std::string> keyframes;
    std::map<int, PropertyKeyframeMeta> keyframeInfo;
};

class AudioRenderer {
protected:
    ma_encoder encoder;
    ma_decoder_config decoderConfig;
    std::vector<AudioRenderFile> clips;

    std::string outputPath;
    float length;
public:
    AudioRenderer(std::string_view outputPath, float length);

    void addClip(std::string path, float start, float end, std::shared_ptr<ClipProperty> volume);
    void render(float fps);
};
