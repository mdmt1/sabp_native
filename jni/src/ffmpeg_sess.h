#pragma once

#include <libavutil/opt.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libswresample/swresample.h>

#include "sonic.c"

typedef struct {

    i32 fd;
    u64 fd_pos;
    u64 fd_len;

    AVFormatContext *fmt_ctx;
    AVPacket *packet;

    AVCodecContext *dec_ctx;
    AVFrame *frame;

    SwrContext *swr_ctx;

    sonicStream sonic;

    AVStream *stream;

    u64 pos_ts;
    u64 time_base_numerator;
    u32 time_base_denominator;

    AVIOContext *io_ctx;

    u32 native_sample_rate;
    u32 out_sample_rate;

    Vb *leftover_samples;
    u32 leftover_samples_off;


    u64 in_ch_layout;
    u64 out_ch_layout;
    u8 in_ch_count;
    u8 out_ch_count;

    bool eof;

} Session;

void free_sess_l1(Session *s, bool close_fd, bool free_self)
{
    if (close_fd) {
        close(s->fd);
    }

    AVIOContext *io_ctx = s->io_ctx;

    if (io_ctx != NULL) {
        av_freep(&io_ctx->buffer);
        avio_context_free(&s->io_ctx);
    }

    // NULL-safe
    avformat_close_input(&s->fmt_ctx);

    // NULL-safe
    av_packet_free(&s->packet);

    // NULL-safe
    avcodec_free_context(&s->dec_ctx);

    // NULL-safe
    av_frame_free(&s->frame);

    // NULL-safe
    swr_free(&s->swr_ctx);

    if (s->sonic != NULL) {
        sonicDestroyStream(s->sonic);
    }

    if (s->leftover_samples != NULL) {
        vb_free_buf(s->leftover_samples);
        free(s->leftover_samples);
    }

    if (free_self) {
        free(s);
    }
}

void ffmpeg_free_sess(JNI_ARGS jlong ptr, jboolean close_fd)
{
    free_sess_l1((Session *) ptr, close_fd, true);
}
