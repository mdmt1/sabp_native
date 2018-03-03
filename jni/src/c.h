#pragma once

#include <stdio.h>
#include <libavutil/error.h>
#include <libavutil/log.h>
#include "types.h"
#include "assert.h"

#include "android/log.h"


#ifdef NDEBUG

const bool c_dev = false;
#define log_d(...)
#define log_ffmpeg_err(fn, err)

#else
const bool c_dev = true;
#define log_d(...) log_d_l1(__FUNCTION__, __VA_ARGS__)
#define log_ffmpeg_err(fn, err) log_ffmpeg_err_l1(fn, err, __FUNCTION__)

#endif

#ifdef SABP_FULL

#if SABP_FULL == 1
const bool c_full = true;
#elif SABP_FULL == 0
const bool c_full = false;
#else
#error SABP_FULL must be 1 or 0
#endif

#else
const bool c_full = true;
#endif

#define LOG_MSG_BUF_SIZE 512

void log_d_l1(const char *tag, const char *msg, ...) {

    char arr[LOG_MSG_BUF_SIZE];

    va_list l;
    va_start(l, msg);

    vsnprintf(arr, LOG_MSG_BUF_SIZE, msg, l);

    va_end(l);

    __android_log_write(ANDROID_LOG_DEBUG, tag, arr);
}

void failed_assertion(const char *msg, int an_int) {
    __assert(msg, an_int, msg);
}

void log_ffmpeg_err_l1(const char *fn_name, int err, const char *parent_fn_name) {

    char arr[LOG_MSG_BUF_SIZE];

    snprintf(arr, LOG_MSG_BUF_SIZE, "FFmpeg error in function %s: \"%s\"", fn_name, av_err2str(err));

    __android_log_write(ANDROID_LOG_ERROR, parent_fn_name, arr);
}

void ffmpeg_log_callback(void *avcl, int level, const char *fmt, va_list vl)
{

//    if (level == AV_LOG_TRACE) {
//        return;
//    }
//
    if (level > AV_LOG_INFO) {
        return;
    }

    const char *av_class_name;

    if (avcl != NULL) {
        AVClass *c = *(AVClass**) avcl;
        av_class_name = c->class_name;
    }
    else {
        av_class_name = "Unknown AVClass";
    }

    const char *lev;

#define swc(exp) case exp: lev = #exp; break;

    switch (level) {

        swc(AV_LOG_PANIC)
        swc(AV_LOG_FATAL)
        swc(AV_LOG_ERROR)
        swc(AV_LOG_WARNING)
        swc(AV_LOG_INFO)
        swc(AV_LOG_VERBOSE)
        swc(AV_LOG_DEBUG)
        swc(AV_LOG_TRACE)
        default:
            lev = "AV_LOG level is unknown";
    }

#undef swc

    char arr1[1024];
    int print_prefix = 1;
    av_log_format_line(avcl, level, fmt, vl, arr1, 1024, &print_prefix);

    char arr2[2048];
    snprintf(arr2, 2048, "%s || %s", av_class_name, arr1);

    __android_log_write(ANDROID_LOG_DEBUG, lev, arr2);
}
