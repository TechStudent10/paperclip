#pragma once

#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>
}

namespace VPFFile {
    // these arent the best argument names
    // but i like the album and the song
    // so we're sticking with it
    // (audio = audio input file, video = video input file, disco = overall output)
    void combineAV(std::string audio, std::string video, std::string disco);

    void extractAudio(std::string filename);
    
};
