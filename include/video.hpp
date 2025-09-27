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
    std::span<const char* const> options;
    std::string data = "";

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
};


class ClipProperty {
private:
    ClipProperty() {}
public:
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
};

struct ClipMetadata {
    std::string name;
};

class Clip {
protected:
    Clip(int startFrame, int duration): startFrame(startFrame), duration(duration) {};
public:
    ClipProperties m_properties;
    ClipMetadata m_metadata;
    int startFrame;
    int duration;
    virtual void render(Frame* frame) {}
    virtual void onDelete() {}
};

class Rectangle : public Clip {
public:
    Rectangle();
    void render(Frame* frame) override;
};

class Circle : public Clip {
public:
    Circle();
    void render(Frame* frame) override;
};

class Text : public Clip {
protected:
    constexpr static std::array<const char*, 11> FONT_NAMES = {
        "Inter",
        "JetBrains Mono",
        "Noto Sans",
        "Noto Serif",
        "Open Sans",
        "Oswald",
        "Raleway",
        "Raleway Dots",
        "Roboto",
        "Source Code Pro",
        "Source Serif 4"
    };
public:
    Text();
    void render(Frame* frame) override;
};

class ImageClip : public Clip {
private:
    unsigned char* imageData;
    int width, height;
public:
    ImageClip(const std::string& path);
    ~ImageClip();

    ImageClip& operator=( const ImageClip& ) = delete;

    void render(Frame* frame) override;
    void onDelete() override;
};

class VideoClip : public Clip {
private:
    bool decodeFrame(int frameNumber);

    mlt_profile profile;
    mlt_producer producer;

    int width = 0, height = 0, fps = 0;
    std::vector<unsigned char> vidFrame;
public:
    VideoClip(const std::string& path);
    ~VideoClip();

    void render(Frame* frame) override;
};

class AudioClip : public Clip {
private:
    ma_sound sound;
public:
    bool playing = false;
    AudioClip(const std::string& path);
    ~AudioClip();

    void play();
    void seekToSec(float seconds);
    void setVolume(float volume);
    void stop();

    float getCursor();
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
};

struct ExtClipMetadata {
    std::string filePath;
    int frameCount;
};

class Video {
protected:
    int framerate;

    Vector2D resolution;

    std::vector<VideoTrack*> videoTracks;
public:
    std::vector<AudioTrack*> audioTracks;
    std::vector<ExtClipMetadata> clipPool;
    std::vector<ExtClipMetadata> imagePool;
    std::vector<ExtClipMetadata> audioClipPool;
    Video(int framerate, Vector2D resolution): framerate(framerate), resolution(resolution) {}

    int frameCount = 0;
    void recalculateFrameCount();

    int addTrack(VideoTrack* track) {
        videoTracks.push_back(track);
        return videoTracks.size() - 1;
    }
    void addClip(int trackIdx, std::shared_ptr<Clip> clip);

    const std::vector<VideoTrack*>& getTracks() const { return videoTracks; }

    void render(VideoRenderer* renderer);
    Frame* renderAtFrame(int frame);
    void renderIntoFrame(int frameNum, std::shared_ptr<Frame> frame);

    int getFPS() { return framerate; }
    Vector2D getResolution() { return resolution; }

    int frameForTime(float time);
    float timeForFrame(int time);
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
