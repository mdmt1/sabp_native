#pragma once

#include "types.h"
#include <unistd.h>
#include <sys/stat.h>

#include "buf.c"


#include "ffmpeg_sess.h"
#include "ffmpeg_sess.c"
#include "c.h"

int read_fn(void *s_r, u8 *buf, int buf_size_int)
{
    Session *s = (Session *) s_r;

    usize buf_size = (usize) buf_size_int;

    isize ret = pread64(s->fd, buf, buf_size, s->fd_pos);

    if (ret <= 0) {
         // 0 on EOF, -1 on error, look at libavformat/aviobuf.c
        return (int) ret;
    }

    s->fd_pos += ret;

    return (int) ret;
}

i64 seek_fn(void *s_r, i64 off, int whence)
{
    Session *s = (Session *) s_r;

    // look at libavformat/aviobuf.c avio_seek function

    if (whence == AVSEEK_SIZE) {
        return (i64) s->fd_len;
    }

    assert(whence == SEEK_SET);

    if ((u64) off > s->fd_len) {
        return -1;
    }

    s->fd_pos = (u64) off;
    return 0;
}

#define IO_BUF_LEN (4 * 1024)

void prepare_l2(Session *sess, u32 *out_dur_ms)
{
    u8 *buf = av_malloc(IO_BUF_LEN);

    if (buf == NULL) {
        return;
    }

    AVIOContext *io_ctx = avio_alloc_context(buf, IO_BUF_LEN,
        0 /* forbid writing */, sess,
        read_fn, NULL /* no write fn */,
        seek_fn
    );

    if (io_ctx == NULL) {
        av_free(buf);
        return;
    }

    sess->io_ctx = io_ctx;

    AVFormatContext *fmt_ctx = avformat_alloc_context();

    if (fmt_ctx == NULL) {
        return;
    }

//    will be freed by ffmpeg if avformat_open_input fails
//    sess->fmt_ctx = fmt_ctx;

    fmt_ctx->pb = io_ctx;

    int ret = avformat_open_input(&fmt_ctx, NULL, NULL, NULL);

    if (ret != 0) {
        log_ffmpeg_err("avformat_open_input", ret);
        // fmt_ctx was freed by ffmpeg in this case
        return;
    }

    sess->fmt_ctx = fmt_ctx;


    if (avformat_find_stream_info(fmt_ctx, NULL) != 0) {
        return;
    }

    AVStream *stream;

    {
        ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);

        if (ret < 0) {
            log_ffmpeg_err("av_find_best_stream", ret);
            return;
        }

        int stream_idx = ret;

        stream = fmt_ctx->streams[stream_idx];

        sess->stream = stream;
    }

    {
        AVRational tb = stream->time_base;
        sess->time_base_numerator = (u64) tb.num * 1000;
        sess->time_base_denominator = (u32) tb.den;
    }

    {
        i64 dur_u = stream->duration;
        u64 dur_ms_l;

        if (dur_u != AV_NOPTS_VALUE)
        {
            u64 dur = (u64) dur_u;

            dur_ms_l = convert_ff_ts_to_ms(sess, dur);
        }
        else {
            dur_ms_l = (u64) fmt_ctx->duration * 1000 / AV_TIME_BASE;
        }

        if (
            dur_ms_l < 1
            ||
            dur_ms_l > I32_MAX
        )
        {
            return;
        }

        *out_dur_ms = (u32) dur_ms_l;
    }
}

void prepare_l1(Session *sess,
                u32 *out_dur_ms, bool *out_success)
{
    prepare_l2(sess, out_dur_ms);

    if (*out_dur_ms == 0) {
        return;
    }
    AVCodecContext *dec_ctx = avcodec_alloc_context3(NULL);

    if (dec_ctx == NULL) {
        return;
    }

    sess->dec_ctx = dec_ctx;

    int ret = avcodec_parameters_to_context(dec_ctx, sess->stream->codecpar);

    if (ret < 0) {
        log_ffmpeg_err("avcodec_parameters_to_context", ret);
        return;
    }

    enum AVCodecID codec_id = dec_ctx->codec_id;

    AVCodec *dec;

    dec = avcodec_find_decoder(codec_id);

    if (dec == NULL) {
        log_d("decoder not found");
        return;
    }

    ret = avcodec_open2(dec_ctx, dec, NULL);

    if (ret != 0) {
        log_ffmpeg_err("avcodec_open2", ret);
        return;
    }

    i32 in_ch_count = dec_ctx->channels;

    if (in_ch_count < 1 || in_ch_count > U8_MAX) {
        return;
    }

    sess->in_ch_count = (u8) in_ch_count;

    u64 in_ch_layout = dec_ctx->channel_layout;

    if (in_ch_layout == 0) {

        in_ch_layout = (u64) av_get_default_channel_layout(in_ch_count);

        if (in_ch_layout == 0) {
            return;
        }
    }

    sess->in_ch_layout = in_ch_layout;

    u64 out_ch_layout;

    if (
        in_ch_layout == AV_CH_LAYOUT_STEREO
        ||
        in_ch_layout == AV_CH_LAYOUT_MONO
    )
    {
        out_ch_layout = in_ch_layout;
    }
    else {
        out_ch_layout = AV_CH_LAYOUT_STEREO;
    }

    sess->out_ch_layout = out_ch_layout;
    sess->out_ch_count = (u8) av_get_channel_layout_nb_channels(out_ch_layout);

    sess->packet = av_packet_alloc();

    if (sess->packet == NULL) {
        return;
    }

    sess->frame = av_frame_alloc();

    if (sess->frame == NULL) {
        return;
    }

    *out_success = true;
}

#define PREPARE_OUT_SIZE 9

u32 ffmpeg_prepare(JNI_ARGS
                   i32 fd, u64 fd_len,
                   u32 native_sample_rate,
                   jlong j_out)
{
    Session *sess = calloc(1, sizeof(Session));

    sess->fd = fd;

    sess->fd_len = fd_len;

    sess->native_sample_rate = native_sample_rate;

    u32 dur_ms = 0;

    bool success = false;

    prepare_l1(sess, &dur_ms, &success);

    if (! success) {
        free_sess_l1(sess, true, true);
        return 0;
    }

    {
        Vb *vb = malloc(sizeof(Vb));
        vb_init(vb);
        sess->leftover_samples = vb;
    }

    Vb vb_;
    Vb *vb = &vb_;

    vb_init_wrap(vb, j_out, PREPARE_OUT_SIZE);
    vb_w_u8(vb, (u8) sess->out_ch_count);
    vb_w_u64(vb, (u64) sess);

    return dur_ms;
}
