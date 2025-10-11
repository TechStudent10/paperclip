#include <video.hpp>
#include <iostream>

VideoRenderer::VideoRenderer(std::string filename, int width, int height, int fps): width(width), height(height), fps(fps), filename(filename) {
    // a bunch of ffmpeg boilerplate
    avformat_network_init();

    avformat_alloc_output_context2(&fmt_ctx, nullptr, nullptr, filename.c_str());
    if (!fmt_ctx) {
        std::cerr << "Could not allocate format context\n";
        return;
    }

    codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        std::cerr << "H.264 codec not found\n";
        return;
    }

    stream = avformat_new_stream(fmt_ctx, nullptr);

    codec_ctx = avcodec_alloc_context3(codec);
    codec_ctx->codec_id = AV_CODEC_ID_H264;
    codec_ctx->width = width;
    codec_ctx->height = height;
    codec_ctx->time_base = AVRational{1, fps};
    codec_ctx->framerate = AVRational{fps, 1};
    codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    codec_ctx->gop_size = 12;
    codec_ctx->max_b_frames = 2;

    if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        std::cerr << "Could not open codec\n";
        return;
    }

    avcodec_parameters_from_context(stream->codecpar, codec_ctx);
    stream->time_base = codec_ctx->time_base;

    if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&fmt_ctx->pb, filename.c_str(), AVIO_FLAG_WRITE) < 0) {
            std::cerr << "Could not open output file\n";
            return;
        }
    }

    if (avformat_write_header(fmt_ctx, nullptr) < 0) {
        std::cerr << "Error writing header\n";
        return;
    }

    frame = av_frame_alloc();
    frame->format = codec_ctx->pix_fmt;
    frame->width  = width;
    frame->height = height;
    av_frame_get_buffer(frame, 0);

    sws_ctx = sws_getContext(
        width, height, AV_PIX_FMT_RGBA,
        width, height, AV_PIX_FMT_YUV420P,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );
}

void VideoRenderer::addFrame(std::shared_ptr<Frame> vidFrame) {
    auto frameData = vidFrame->getFrameData();
    const uint8_t* src_slices[1] = { frameData.data() };
    int src_stride[1] = { 4 * width };
    sws_scale(sws_ctx, src_slices, src_stride, 0, height, frame->data, frame->linesize);

    frame->pts = av_rescale_q(currentFrame, AVRational{1, fps}, codec_ctx->time_base);
    
    if (avcodec_send_frame(codec_ctx, frame) == 0) {
        AVPacket pkt;
        av_init_packet(&pkt);
        pkt.data = nullptr;
        pkt.size = 0;
        while (avcodec_receive_packet(codec_ctx, &pkt) == 0) {
            pkt.stream_index = stream->index;
            av_packet_rescale_ts(&pkt, codec_ctx->time_base, stream->time_base);
            av_interleaved_write_frame(fmt_ctx, &pkt);
            av_packet_unref(&pkt);
        }
    }

    currentFrame++;
}

void VideoRenderer::addAudio(std::vector<float>& data) {
    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!codec) throw std::runtime_error("AAC encoder not found");

    AVStream* audio_st = avformat_new_stream(fmt_ctx, codec);
    if (!audio_st) throw std::runtime_error("Failed to create audio stream");

    AVCodecContext* enc_ctx = avcodec_alloc_context3(codec);
    av_channel_layout_default(&enc_ctx->ch_layout, 2);
    enc_ctx->sample_rate = 48000;
    enc_ctx->sample_fmt  = codec->sample_fmts[0];
    enc_ctx->bit_rate    = 128000;
    enc_ctx->time_base   = {1, 48000};

    if (avcodec_open2(enc_ctx, codec, nullptr) < 0)
        throw std::runtime_error("Failed to open AAC encoder");

    if (avcodec_parameters_from_context(audio_st->codecpar, enc_ctx) < 0)
        throw std::runtime_error("Failed to copy encoder parameters");

    avformat_write_header(fmt_ctx, nullptr);

    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        // uh oh error!
        return;
    }

    frame->nb_samples     = enc_ctx->frame_size;
    frame->format         = AV_SAMPLE_FMT_FLT;
    frame->sample_rate    = 48000;
    av_channel_layout_copy(&frame->ch_layout, &enc_ctx->ch_layout);

    int ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        av_frame_free(&frame);
        return;
    }

    bool planar = av_sample_fmt_is_planar((AVSampleFormat)frame->format);
    int planes = planar ? 2 : 1;
    int samples_per_plane = planar ? enc_ctx->frame_size : enc_ctx->frame_size * 2;

    // Copy PCM samples safely
    for (int ch = 0; ch < planes; ch++) {
        float* dst = reinterpret_cast<float*>(frame->data[ch]);
        const float* src = data.data() + ch * 2 + (planar ? ch * enc_ctx->frame_size : 0);

        if (planar) {
            for (int i = 0; i < enc_ctx->frame_size; i++) {
                dst[i] = data[(ch + i) * 2 + ch];
            }
        } else {
            memcpy(dst, src, samples_per_plane * sizeof(float));
        }
    }

    // Now send frame to encoder
    ret = avcodec_send_frame(enc_ctx, frame);
    av_frame_free(&frame);

    av_write_trailer(fmt_ctx);
    avcodec_free_context(&enc_ctx);
}

void VideoRenderer::finish() {
    avcodec_send_frame(codec_ctx, nullptr);  // flush signal

    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = nullptr;
    pkt.size = 0;

    while (avcodec_receive_packet(codec_ctx, &pkt) == 0) {
        av_packet_rescale_ts(&pkt, codec_ctx->time_base, stream->time_base);
        pkt.stream_index = stream->index;
        av_interleaved_write_frame(fmt_ctx, &pkt);
        av_packet_unref(&pkt);
    }

    av_write_trailer(fmt_ctx);

    if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_close(fmt_ctx->pb);
    }

    avcodec_free_context(&codec_ctx);
    av_frame_free(&frame);
    sws_freeContext(sws_ctx);
    avformat_free_context(fmt_ctx);
}
