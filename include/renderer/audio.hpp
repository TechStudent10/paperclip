#pragma once

#include <string>
#include <map>
#include <vector>

#include <clips/clip.hpp>
#include <miniaudio.h>

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
