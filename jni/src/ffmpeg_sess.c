#pragma once

#include <unistd.h>
#include "ffmpeg_sess.h"
#include "buf.c"

void ffmpeg_init()
{
    av_register_all();

    if (c_dev) {
        av_log_set_callback(ffmpeg_log_callback);

//        log_d("cpu flags: %x", av_get_cpu_flags());
//        log_d("neon support: %i", (av_get_cpu_flags() & AV_CPU_FLAG_NEON) != 0);

    }
}

bool set_out_sample_rate(Session *s, u32 out_sample_rate) {

    if (s->out_sample_rate == out_sample_rate) {
        return true;
    }

    swr_free(&s->swr_ctx);

    SwrContext *c = swr_alloc_set_opts(
        NULL,
        s->out_ch_layout, AV_SAMPLE_FMT_S16,
        out_sample_rate,
        s->in_ch_layout, s->dec_ctx->sample_fmt, s->dec_ctx->sample_rate,
        0, NULL
    );

    if (c == NULL) {
        return false;
    }

    int ret = swr_init(c);

    if (ret != 0) {
        log_ffmpeg_err("swr_init", ret);
        swr_free(&c);
        return false;
    }

    s->swr_ctx = c;
    log_d("out_sample rate: old %u new %u", s->out_sample_rate, out_sample_rate);
    s->out_sample_rate = out_sample_rate;

    return true;
}

u32 out_samples_to_bytes(Session *s, u32 samples)
{
    return samples << s->out_ch_count;
}

u32 out_bytes_to_samples(Session *s, u32 bytes)
{
    return bytes >> s->out_ch_count;
}

u32 convert_ff_ts_to_ms(Session *s, u64 ts)
{
    return (u32) ((u64) ts * s->time_base_numerator / (u64) s->time_base_denominator);
}

u64 convert_ms_to_ff_ts(Session *s, u32 ms)
{
    return (u64) ms * (u64) s->time_base_denominator / s->time_base_numerator;
}

u64 convert_ff_ts_to_out_samples(Session *s, u64 ts)
{
    return (ts * s->time_base_numerator * (u64) s->out_sample_rate) / ((u64) s->time_base_denominator * 1000);
}
