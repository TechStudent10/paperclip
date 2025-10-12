#pragma once

extern "C" {
#include <libavutil/opt.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#include <string>
#include <vector>
#include <memory>

#include <frame.hpp>

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
