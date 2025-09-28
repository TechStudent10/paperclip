#pragma once
#include <memory>
#include <unordered_map>
#include <vector>
#include <array>
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

#include <cereal/cereal.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/complex.hpp>
#include <cereal/types/base_class.hpp>

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

    // archive
    template<class Archive>
    void serialize(Archive& archive) {
        archive(x, y);
    }
};

struct Vector1D {
    int number;

    JSON_METHODS(Vector1D);

    // archive
    template<class Archive>
    void serialize(Archive& archive) {
        archive(number);
    }
};

struct Dimensions {
    Vector2D pos;
    Vector2D size;
    
    JSON_METHODS(Dimensions)

    // archive
    template<class Archive>
    void serialize(Archive& archive) {
        archive(pos, size);
    }
};

struct RGBAColor {
    int r; int g; int b; int a = 255;

    JSON_METHODS(RGBAColor)

    // archive
    template<class Archive>
    void serialize(Archive& archive) {
        archive(r, g, b, a);
    }
};

struct DropdownOptions {
    std::vector<std::string> options;

    JSON_METHODS(DropdownOptions)

    // archive
    template<class Archive>
    void serialize(Archive& archive) {
        archive(options);
    }
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

    // archive
    template<class Archive>
    void serialize(Archive& archive) {
        archive(easing, mode);
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

    // archive
    template<class Archive>
    void serialize(Archive& archive) {
        archive(
            id,
            name,
            type,
            data,
            options,
            keyframes,
            keyframeInfo
        );
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

    // archive
    template<class Archive>
    void serialize(Archive& archive) {
        archive(properties);
    }
};

struct ClipMetadata {
    std::string name;
    // archive
    template<class Archive>
    void serialize(Archive& archive) {
        archive(name);
    }
};

// archive
// ex: CEREAL_CLIP(, myThing)
// yes the stupid comma is needed
#define CEREAL_CLIP(...) \
    template<class Archive> \
    void serialize(Archive& archive) { \
        archive( \
            cereal::base_class<Clip>(this) \
            __VA_ARGS__ \
        ); \
    }

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

    // archive
    template<class Archive>
    void serialize(Archive& archive) {
        archive(
            m_properties,
            m_metadata,
            startFrame,
            duration
        );
    }
};


class Rectangle : public Clip {
public:
    Rectangle();
    void render(Frame* frame) override;

    CEREAL_CLIP()
};

class Circle : public Clip {
public:
    Circle();
    void render(Frame* frame) override;

    CEREAL_CLIP()
};

class Text : public Clip {
public:
    Text();
    void render(Frame* frame) override;
    CEREAL_CLIP()
};


class ImageClip : public Clip {
private:
    unsigned char* imageData;
    int width, height;

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

    template<class Archive>
    void serialize(Archive& archive) {
        archive(
            cereal::base_class<Clip>(this),
            path
        );
    }
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

    void render(Frame* frame) override;
    
    template<class Archive>
    void serialize(Archive& archive) {
        archive(
            cereal::base_class<Clip>(this),
            path
        );
    }
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
    
    template<class Archive>
    void serialize(Archive& archive) {
        archive(
            cereal::base_class<Clip>(this),
            path
        );
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

    // archive
    template<class Archive>
    void serialize(Archive& archive) {
        archive(clips);
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

    // archive
    template<class Archive>
    void serialize(Archive& archive) {
        archive(clips);
    }
};

struct ExtClipMetadata {
    std::string filePath;
    int frameCount;

    // archive
    template<class Archive>
    void serialize(Archive& archive) {
        archive(filePath, frameCount);
    }
};

class Video {
protected:
    std::vector<std::shared_ptr<VideoTrack>> videoTracks;
public:
    int framerate;

    Vector2D resolution;
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

    // archive
    template<class Archive>
    void serialize(Archive& archive) {
        archive(
            framerate,
            resolution,
            videoTracks,
            audioTracks,
            clipPool,
            imagePool,
            audioClipPool
        );
    }
};

class TextRenderer {
protected:
    std::unordered_map<std::string, std::vector<unsigned char>> fontBuffers;
public:
    TextRenderer();
    void loadFont(std::string fontName);
    void drawText(Frame* frame, std::string text, std::string fontName, Vector2D pos, RGBAColor color, float pixelHeight = 48.f);
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

#define REGISTER(type) CEREAL_REGISTER_TYPE(type) \
    CEREAL_REGISTER_POLYMORPHIC_RELATION(Clip, type)

REGISTER(Rectangle)
REGISTER(ImageClip)
REGISTER(Circle)
REGISTER(Text)
REGISTER(VideoClip)
REGISTER(AudioClip)
