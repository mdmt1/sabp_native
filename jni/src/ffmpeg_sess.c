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

SwrContext *get_swr_ctx(Session *s, AVFrame *frame)
{
    u64 ch_layout = frame->channel_layout;

    if (ch_layout == 0) {
        ch_layout = s->dec_ctx->channel_layout;
    }

    u32 sample_rate = (u32) frame->sample_rate;

    Swr_ctx_container *arr = s->swr_cache;

    for (int i = 0; i < SWR_CACHE_CAP; ++i) {

        Swr_ctx_container c = arr[i];

        if (c.instance == NULL) {
            break;
        }

        if (
            c.in_sample_rate == sample_rate
            &&
            c.in_ch_layout == ch_layout
        )
        {
            if (i != 0) {
                log_d("SwrContext move to top from %u", i);
                arr[i] = arr[0];
                arr[0] = c;
            }

            return c.instance;
        }
    }

    // cached context not found

    // NULL-safe

    SwrContext *swr_ctx = swr_alloc_set_opts(
        NULL,
        s->out_ch_layout, AV_SAMPLE_FMT_S16,
        s->out_sample_rate,
        ch_layout, s->dec_ctx->sample_fmt, sample_rate,
        0, NULL
    );

    if (swr_ctx == NULL) {
        return NULL;
    }

    int ret = swr_init(swr_ctx);

    if (ret != 0) {
        log_ffmpeg_err("swr_init", ret);
        swr_free(&swr_ctx);
        return NULL;
    }

    {
        Swr_ctx_container last = arr[SWR_CACHE_CAP - 1];

        if (last.instance != NULL) {
            log_d("freed SwrContext in_ch_layout %llu, in_sample_rate %u", last.in_ch_layout, last.in_sample_rate);
        }

        swr_free(&last.instance);

        memmove(arr + 1, arr, sizeof(Swr_ctx_container) * (SWR_CACHE_CAP - 1));
    }

    Swr_ctx_container c = {
        .instance = swr_ctx,
        .in_ch_layout = ch_layout,
        .in_sample_rate = sample_rate
    };

    arr[0] = c;

    log_d("allocated SwrContext in_ch_layout %llu, in_sample_rate %u out_ch_layout %llu out_sample_rate %u", ch_layout, sample_rate, s->out_ch_layout, s->out_sample_rate);

    return swr_ctx;
}

void free_all_swr_contexts(Session *s)
{
    Swr_ctx_container *arr = s->swr_cache;

    for (int i = 0; i < SWR_CACHE_CAP; ++i) {

        Swr_ctx_container *c = &arr[i];

        if (c->instance == NULL) {
            return;
        }

        swr_free(&c->instance);

        log_d("freed SwrContext in_ch_layout %llu, in_sample_rate %u", c->in_ch_layout, c->in_sample_rate);
    }
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
