#pragma once

#include "ffmpeg_sess.c"
#include "ffmpeg_sess.h"

typedef enum {
    READ_FRAME_RES_SUCCESS,
    READ_FRAME_RES_END_OF_FILE,
    READ_FRAME_RES_FAILURE
} Res_read_frame;

typedef enum {
    RECEIVE_FRAME_RES_SUCCESS,
    RECEIVE_FRAME_RES_BENIGN_ERROR,
    RECEIVE_FRAME_RES_NOT_ENOUGH_INPUT,
    RECEIVE_FRAME_RES_END_OF_FILE,
} Res_receive_frame;

Res_read_frame read_frame(Session *s)
{
    int ret;

    m_read_frame_again:
    ret = av_read_frame(s->fmt_ctx, s->packet);

    if (ret == 0) {

        if (s->packet->stream_index != s->stream->index) {
            av_packet_unref(s->packet);
            goto m_read_frame_again;
        }

        return READ_FRAME_RES_SUCCESS;
    }

    log_ffmpeg_err("av_read_frame", ret);

    if (ret == AVERROR_EOF) {

        // flush packet
        ret = avcodec_send_packet(s->dec_ctx, NULL);

        if (ret != 0) {
            failed_assertion("send_eof", ret);
        }

        return READ_FRAME_RES_END_OF_FILE;
    }
    else {
        return READ_FRAME_RES_FAILURE;
    }
}

void send_packet(Session *s)
{
    int ret = avcodec_send_packet(s->dec_ctx, s->packet);
    av_packet_unref(s->packet);

    if (ret == 0) {
        return;
    }

    if (
        ret != AVERROR_PATCHWELCOME
        &&
        ret != AVERROR_INVALIDDATA
        &&
        ret != AVERROR(EPERM)
    )
    {
        log_ffmpeg_err("avcodec_send_packet", ret);
    }


    if (ret == AVERROR(EINVAL)) {
        avcodec_flush_buffers(s->dec_ctx);
        return;
    }

    if (
        ret == AVERROR(ENOMEM) || // better crash now
        ret == AVERROR_EOF // should never happen
    )
    {
        failed_assertion("send_packet", ret);
    }

    // per avcodec_send_packet doc, others are "legitimate decoding errors"
}

Res_receive_frame receive_frame(Session *s)
{
    int ret = avcodec_receive_frame(s->dec_ctx, s->frame);

    if (ret == 0) {
//
//        if (c_dev) {
//            AVFrame *f = s->frame;
//
//            if (f->channels != s->prev_ch_cnt) {
//                s->prev_ch_cnt = (u32) f->channels;
//                log_d("change of ch_cnt %u", s->prev_ch_cnt);
//            }
//
//            if (f->sample_rate != s->prev_sample_rate) {
//                s->prev_sample_rate = (u32) f->sample_rate;
//                log_d("change of sample rate %u", s->prev_sample_rate);
//            }
//        }

        return RECEIVE_FRAME_RES_SUCCESS;
    }

    if (ret == AVERROR(EAGAIN)) {
        return RECEIVE_FRAME_RES_NOT_ENOUGH_INPUT;
    }

    log_ffmpeg_err("avcodec_receive_frame", ret);

    if (ret == AVERROR_EOF) {
        return RECEIVE_FRAME_RES_END_OF_FILE;
    }

    return RECEIVE_FRAME_RES_BENIGN_ERROR;
}
