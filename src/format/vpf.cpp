#include <cstring>
#include <format/vpf.hpp>

extern "C" {
#include <libavutil/opt.h>
}
#include <fmt/base.h>
#include <fmt/format.h>

void VPFFile::extractAudio(std::string filename) {
    AVFormatContext* fmt_ctx = nullptr;
    const AVCodec* decoder = nullptr;
    AVStream* audio_stream = nullptr;
    const AVCodec* encoder = nullptr;
    AVCodecContext* enc_ctx = nullptr;
    AVStream* out_stream = nullptr;
    AVFrame* frame = nullptr;
    AVPacket* pkt = nullptr;
    AVAudioFifo* fifo = nullptr;
    SwrContext* swr_ctx = nullptr;
    AVPacket* out_pkt = nullptr;
    AVFormatContext* out_fmt_ctx = nullptr;
    AVCodecContext* dec_ctx = nullptr;

    int audio_stream_index = 0;

    auto output_filename = fmt::format("{}.mp3", filename);

    if (avformat_open_input(&fmt_ctx, filename.c_str(), nullptr, nullptr) < 0) {
        goto cleanup;
    }

    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        goto cleanup;
    }

    audio_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &decoder, 0);
    if (audio_stream_index < 0) {
        goto cleanup;
    }

    audio_stream = fmt_ctx->streams[audio_stream_index];

    dec_ctx = avcodec_alloc_context3(decoder);
    if (!dec_ctx) goto cleanup;
    if (avcodec_parameters_to_context(dec_ctx, audio_stream->codecpar) < 0) goto cleanup;
    if (avcodec_open2(dec_ctx, decoder, nullptr) < 0) {
        goto cleanup;
    }

    encoder = avcodec_find_encoder(AV_CODEC_ID_MP3);
    if (!encoder) {
        goto cleanup;
    }

    enc_ctx = avcodec_alloc_context3(encoder);
    if (!enc_ctx) goto cleanup;
    enc_ctx->ch_layout       = dec_ctx->ch_layout;
    enc_ctx->sample_rate     = dec_ctx->sample_rate;
    enc_ctx->sample_fmt      = encoder->sample_fmts[0];
    enc_ctx->bit_rate        = 192000;
    if (avcodec_open2(enc_ctx, encoder, nullptr) < 0) {
        goto cleanup;
    }

    avformat_alloc_output_context2(&out_fmt_ctx, nullptr, nullptr, output_filename.c_str());
    if (!out_fmt_ctx) {
        goto cleanup;
    }

    out_stream = avformat_new_stream(out_fmt_ctx, nullptr);
    if (!out_stream) goto cleanup;
    avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);

    if (!(out_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&out_fmt_ctx->pb, output_filename.c_str(), AVIO_FLAG_WRITE) < 0) {
            goto cleanup;
        }
    }

    if (avformat_write_header(out_fmt_ctx, nullptr) < 0) {
        goto cleanup;
    }

    pkt = av_packet_alloc();
    frame = av_frame_alloc();
    if (!pkt || !frame) goto cleanup;

    swr_alloc_set_opts2(
        &swr_ctx,
        &enc_ctx->ch_layout,
        enc_ctx->sample_fmt,
        enc_ctx->sample_rate,
        &dec_ctx->ch_layout,
        dec_ctx->sample_fmt,
        dec_ctx->sample_rate,
        0, nullptr
    );
    if (!swr_ctx) goto cleanup;
    if (swr_init(swr_ctx) < 0) goto cleanup;

    fifo = av_audio_fifo_alloc(enc_ctx->sample_fmt, enc_ctx->ch_layout.nb_channels, 1);
    if (!fifo) goto cleanup;

    while (av_read_frame(fmt_ctx, pkt) >= 0) {
        if (pkt->stream_index == audio_stream_index) {
            if (avcodec_send_packet(dec_ctx, pkt) == 0) {
                while (avcodec_receive_frame(dec_ctx, frame) == 0) {
                    AVFrame* tmp_frame = av_frame_alloc();
                    if (!tmp_frame) { av_frame_unref(frame); goto cleanup; }
                    tmp_frame->nb_samples = frame->nb_samples;
                    tmp_frame->ch_layout = enc_ctx->ch_layout;
                    tmp_frame->format = enc_ctx->sample_fmt;
                    tmp_frame->sample_rate = enc_ctx->sample_rate;
                    if (av_frame_get_buffer(tmp_frame, 0) < 0) { av_frame_free(&tmp_frame); goto cleanup; }

                    swr_convert_frame(swr_ctx, tmp_frame, frame);

                    av_audio_fifo_write(fifo, (void**)tmp_frame->data, tmp_frame->nb_samples);

                    av_frame_free(&tmp_frame);

                    while (av_audio_fifo_size(fifo) >= enc_ctx->frame_size) {
                        AVFrame* enc_frame = av_frame_alloc();
                        if (!enc_frame) goto cleanup;
                        enc_frame->nb_samples = enc_ctx->frame_size;
                        enc_frame->ch_layout = enc_ctx->ch_layout;
                        enc_frame->format = enc_ctx->sample_fmt;
                        enc_frame->sample_rate = enc_ctx->sample_rate;
                        if (av_frame_get_buffer(enc_frame, 0) < 0) { av_frame_free(&enc_frame); goto cleanup; }

                        av_audio_fifo_read(fifo, (void**)enc_frame->data, enc_ctx->frame_size);
                        avcodec_send_frame(enc_ctx, enc_frame);

                        AVPacket* tmp_out_pkt = av_packet_alloc();
                        if (!tmp_out_pkt) { av_frame_free(&enc_frame); goto cleanup; }
                        while (avcodec_receive_packet(enc_ctx, tmp_out_pkt) == 0) {
                            av_interleaved_write_frame(out_fmt_ctx, tmp_out_pkt);
                            av_packet_unref(tmp_out_pkt);
                        }
                        av_packet_free(&tmp_out_pkt);
                        av_frame_free(&enc_frame);
                    }
                }
            }
        }
        av_packet_unref(pkt);
    }

    avcodec_send_packet(dec_ctx, nullptr);
    while (avcodec_receive_frame(dec_ctx, frame) == 0) {
        AVFrame* tmp_frame = av_frame_alloc();
        if (!tmp_frame) goto cleanup;
        tmp_frame->nb_samples     = frame->nb_samples;
        tmp_frame->ch_layout = enc_ctx->ch_layout;
        tmp_frame->format         = enc_ctx->sample_fmt;
        tmp_frame->sample_rate    = enc_ctx->sample_rate;
        if (av_frame_get_buffer(tmp_frame, 0) < 0) { av_frame_free(&tmp_frame); goto cleanup; }
        swr_convert_frame(swr_ctx, tmp_frame, frame);
        av_audio_fifo_write(fifo, (void**)tmp_frame->data, tmp_frame->nb_samples);
        av_frame_free(&tmp_frame);

        while (av_audio_fifo_size(fifo) >= enc_ctx->frame_size) {
            AVFrame* enc_frame = av_frame_alloc();
            if (!enc_frame) goto cleanup;
            enc_frame->nb_samples = enc_ctx->frame_size;
            enc_frame->ch_layout = enc_ctx->ch_layout;
            enc_frame->format = enc_ctx->sample_fmt;
            enc_frame->sample_rate = enc_ctx->sample_rate;
            if (av_frame_get_buffer(enc_frame, 0) < 0) { av_frame_free(&enc_frame); goto cleanup; }
            av_audio_fifo_read(fifo, (void**)enc_frame->data, enc_ctx->frame_size);
            avcodec_send_frame(enc_ctx, enc_frame);

            AVPacket* tmp_out_pkt = av_packet_alloc();
            if (!tmp_out_pkt) { av_frame_free(&enc_frame); goto cleanup; }
            while (avcodec_receive_packet(enc_ctx, tmp_out_pkt) == 0) {
                av_interleaved_write_frame(out_fmt_ctx, tmp_out_pkt);
                av_packet_unref(tmp_out_pkt);
            }
            av_packet_free(&tmp_out_pkt);
            av_frame_free(&enc_frame);
        }
    }

    while (av_audio_fifo_size(fifo) > 0) {
        int frame_size = std::min(av_audio_fifo_size(fifo), enc_ctx->frame_size);
        AVFrame* enc_frame = av_frame_alloc();
        if (!enc_frame) goto cleanup;
        enc_frame->nb_samples = frame_size;
        enc_frame->ch_layout = enc_ctx->ch_layout;
        enc_frame->format = enc_ctx->sample_fmt;
        enc_frame->sample_rate = enc_ctx->sample_rate;
        if (av_frame_get_buffer(enc_frame, 0) < 0) { av_frame_free(&enc_frame); goto cleanup; }

        av_audio_fifo_read(fifo, (void**)enc_frame->data, frame_size);
        avcodec_send_frame(enc_ctx, enc_frame);

        AVPacket* tmp_out_pkt = av_packet_alloc();
        if (!tmp_out_pkt) { av_frame_free(&enc_frame); goto cleanup; }
        while (avcodec_receive_packet(enc_ctx, tmp_out_pkt) == 0) {
            av_interleaved_write_frame(out_fmt_ctx, tmp_out_pkt);
            av_packet_unref(tmp_out_pkt);
        }
        av_packet_free(&tmp_out_pkt);
        av_frame_free(&enc_frame);
    }

    avcodec_send_frame(enc_ctx, nullptr);
    out_pkt = av_packet_alloc();
    if (out_pkt) {
        while (avcodec_receive_packet(enc_ctx, out_pkt) == 0) {
            av_interleaved_write_frame(out_fmt_ctx, out_pkt);
            av_packet_unref(out_pkt);
        }
        av_packet_free(&out_pkt);
        out_pkt = nullptr;
    }

    if (out_fmt_ctx) {
        av_write_trailer(out_fmt_ctx);
    }

cleanup:
    if (fifo) {
        av_audio_fifo_free(fifo);
    }
    if (swr_ctx) {
        swr_free(&swr_ctx);
    }
    if (frame) {
        av_frame_free(&frame);
    }
    if (pkt) {
        av_packet_free(&pkt);
    }
    if (dec_ctx) {
        avcodec_free_context(&dec_ctx);
    }
    if (enc_ctx) {
        avcodec_free_context(&enc_ctx);
    }
    if (fmt_ctx) {
        avformat_close_input(&fmt_ctx);
    }
    if (out_fmt_ctx) {
        if (!(out_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&out_fmt_ctx->pb);
        }
        avformat_free_context(out_fmt_ctx);
    }
}

void VPFFile::combineAV(std::string audio, std::string video, std::string disco) {
    AVFormatContext *in_v_fmt = nullptr, *in_a_fmt = nullptr, *out_fmt = nullptr;
    AVCodecContext *dec_ctx = nullptr, *enc_ctx = nullptr;
    AVStream *in_v_stream = nullptr, *in_a_stream = nullptr;
    AVStream *out_v_stream = nullptr, *out_a_stream = nullptr;
    SwrContext *swr = nullptr;
    int ret = 0;

    auto fferr = [](int err) {
        char buf[256];
        av_strerror(err, buf, sizeof(buf));
        return std::string(buf);
    };

    if ((ret = avformat_open_input(&in_v_fmt, video.c_str(), nullptr, nullptr)) < 0) {
        fmt::print("open video input: {}", fferr(ret)); return;
    }
    if ((ret = avformat_find_stream_info(in_v_fmt, nullptr)) < 0) {
        fmt::print("find video stream info: {}", fferr(ret)); return;
    }

    if ((ret = avformat_open_input(&in_a_fmt, audio.c_str(), nullptr, nullptr)) < 0) {
        fmt::print("open audio input: {}", fferr(ret)); return;
    }
    if ((ret = avformat_find_stream_info(in_a_fmt, nullptr)) < 0) {
        fmt::print("find audio stream info: {}", fferr(ret)); return;
    }

    ret = av_find_best_stream(in_v_fmt, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (ret < 0) { fmt::print("no video stream: {}", fferr(ret)); return; }
    in_v_stream = in_v_fmt->streams[ret];

    ret = av_find_best_stream(in_a_fmt, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (ret < 0) { fmt::print("no audio stream: {}", fferr(ret)); return; }
    in_a_stream = in_a_fmt->streams[ret];

    const AVCodec *dec = avcodec_find_decoder(in_a_stream->codecpar->codec_id);
    if (!dec) { fmt::print("No audio decoder"); return; }
    dec_ctx = avcodec_alloc_context3(dec);
    avcodec_parameters_to_context(dec_ctx, in_a_stream->codecpar);
    if ((ret = avcodec_open2(dec_ctx, dec, nullptr)) < 0) {
        fmt::print("open audio decoder: {}", fferr(ret)); return;
    }

    const AVCodec *enc = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!enc) { fmt::print("No AAC encoder"); return; }
    enc_ctx = avcodec_alloc_context3(enc);
    enc_ctx->sample_rate = dec_ctx->sample_rate;
    enc_ctx->ch_layout = dec_ctx->ch_layout;
    av_channel_layout_default(&enc_ctx->ch_layout, dec_ctx->ch_layout.nb_channels);
    enc_ctx->sample_fmt = enc->sample_fmts ? enc->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
    enc_ctx->bit_rate = 128000;
    enc_ctx->time_base = AVRational{1, enc_ctx->sample_rate};

    if ((ret = avcodec_open2(enc_ctx, enc, nullptr)) < 0) {
        fmt::print("open AAC encoder: {}", fferr(ret)); return;
    }

    swr = swr_alloc();
    av_opt_set_chlayout(swr, "in_chlayout",  &dec_ctx->ch_layout, 0);
    av_opt_set_int(swr, "in_sample_rate",   dec_ctx->sample_rate, 0);
    av_opt_set_sample_fmt(swr, "in_sample_fmt", (AVSampleFormat)dec_ctx->sample_fmt, 0);
    av_opt_set_chlayout(swr, "out_chlayout", &enc_ctx->ch_layout, 0);
    av_opt_set_int(swr, "out_sample_rate",  enc_ctx->sample_rate, 0);
    av_opt_set_sample_fmt(swr, "out_sample_fmt", enc_ctx->sample_fmt, 0);
    if ((ret = swr_init(swr)) < 0) {
        fmt::print("swr_init: {}", fferr(ret)); return;
    }

    AVDictionary *opts = nullptr;
    av_dict_set(&opts, "max_muxing_queue_size", "9999", 0);

    if ((ret = avformat_alloc_output_context2(&out_fmt, nullptr, nullptr, disco.c_str())) < 0) {
        fmt::print("alloc output context: {}", fferr(ret)); return;
    }

    out_v_stream = avformat_new_stream(out_fmt, nullptr);
    avcodec_parameters_copy(out_v_stream->codecpar, in_v_stream->codecpar);
    out_v_stream->time_base = in_v_stream->time_base;

    out_a_stream = avformat_new_stream(out_fmt, nullptr);
    avcodec_parameters_from_context(out_a_stream->codecpar, enc_ctx);
    out_a_stream->time_base = enc_ctx->time_base;

    if (!(out_fmt->oformat->flags & AVFMT_NOFILE)) {
        if ((ret = avio_open(&out_fmt->pb, disco.c_str(), AVIO_FLAG_WRITE)) < 0) {
            fmt::print("avio_open: {}", fferr(ret)); return;
        }
    }

    if ((ret = avformat_write_header(out_fmt, &opts)) < 0) {
        fmt::print("write header: {}", fferr(ret));
        av_dict_free(&opts);
        return;
    }
    av_dict_free(&opts);

    AVFrame *in_frame = av_frame_alloc();
    AVFrame *out_frame = av_frame_alloc();
    AVPacket pkt;

    while (av_read_frame(in_v_fmt, &pkt) >= 0) {
        if (pkt.stream_index == in_v_stream->index) {
            av_packet_rescale_ts(&pkt, in_v_stream->time_base, out_v_stream->time_base);
            pkt.stream_index = out_v_stream->index;
            av_interleaved_write_frame(out_fmt, &pkt);
            av_packet_unref(&pkt);
        }
    }

    while (av_read_frame(in_a_fmt, &pkt) >= 0) {
        if (pkt.stream_index != in_a_stream->index) {
            av_packet_unref(&pkt);
            continue;
        }

        ret = avcodec_send_packet(dec_ctx, &pkt);
        av_packet_unref(&pkt);
        if (ret < 0) break;

        while ((ret = avcodec_receive_frame(dec_ctx, in_frame)) >= 0) {
            out_frame->nb_samples = in_frame->nb_samples;
            out_frame->format = enc_ctx->sample_fmt;
            out_frame->sample_rate = enc_ctx->sample_rate;
            av_channel_layout_copy(&out_frame->ch_layout, &enc_ctx->ch_layout);

            if ((ret = av_frame_get_buffer(out_frame, 0)) < 0) {
                fmt::print("av_frame_get_buffer {}", ret);
                goto cleanup;
            }

            int converted_samples = swr_convert(
                swr,
                out_frame->data, out_frame->nb_samples,
                (const uint8_t **)in_frame->data, in_frame->nb_samples
            );
            if (converted_samples < 0) { fmt::print("swr_convert: {}", converted_samples); goto cleanup; }

            int frame_size = enc_ctx->frame_size;
            int offset = 0;
            while (converted_samples - offset >= frame_size) {
                AVFrame *chunk = av_frame_alloc();
                chunk->nb_samples = frame_size;
                chunk->format = enc_ctx->sample_fmt;
                chunk->sample_rate = enc_ctx->sample_rate;
                av_channel_layout_copy(&chunk->ch_layout, &enc_ctx->ch_layout);

                if ((ret = av_frame_get_buffer(chunk, 0)) < 0) {
                    fmt::print("av_frame_get_buffer {}", ret);
                    av_frame_free(&chunk);
                    break;
                }

                for (int ch = 0; ch < enc_ctx->ch_layout.nb_channels; ch++) {
                    memcpy(chunk->data[ch],
                           out_frame->data[ch] + offset * av_get_bytes_per_sample((AVSampleFormat)chunk->format),
                           frame_size * av_get_bytes_per_sample((AVSampleFormat)chunk->format));
                }

                chunk->pts = av_rescale_q(in_frame->pts + offset,
                                          in_a_stream->time_base,
                                          enc_ctx->time_base);

                ret = avcodec_send_frame(enc_ctx, chunk);
                if (ret < 0) {
                    fmt::print("send frame: {}", ret);
                    av_frame_free(&chunk);
                    break;
                }

                AVPacket out_pkt;
                av_init_packet(&out_pkt);
                out_pkt.data = nullptr;
                out_pkt.size = 0;

                while ((ret = avcodec_receive_packet(enc_ctx, &out_pkt)) >= 0) {
                    av_packet_rescale_ts(&out_pkt, enc_ctx->time_base, out_a_stream->time_base);
                    out_pkt.stream_index = out_a_stream->index;
                    av_interleaved_write_frame(out_fmt, &out_pkt);
                    av_packet_unref(&out_pkt);
                }

                av_frame_free(&chunk);
                offset += frame_size;
            }

            av_frame_unref(out_frame);
            av_frame_unref(in_frame);
        }
    }

    avcodec_send_frame(enc_ctx, nullptr);
    while ((ret = avcodec_receive_packet(enc_ctx, &pkt)) >= 0) {
        av_packet_rescale_ts(&pkt, enc_ctx->time_base, out_a_stream->time_base);
        pkt.stream_index = out_a_stream->index;
        av_interleaved_write_frame(out_fmt, &pkt);
        av_packet_unref(&pkt);
    }

    av_write_trailer(out_fmt);

cleanup:
    av_frame_free(&in_frame);
    av_frame_free(&out_frame);
    swr_free(&swr);
    avcodec_free_context(&dec_ctx);
    avcodec_free_context(&enc_ctx);
    avformat_close_input(&in_v_fmt);
    avformat_close_input(&in_a_fmt);
    if (!(out_fmt->oformat->flags & AVFMT_NOFILE)) avio_closep(&out_fmt->pb);
    avformat_free_context(out_fmt);
}
