#include "ffmpeg_prepare.c"
#include "ffmpeg_sess.h"
#include "vb.h"

u32 get_file_info_l1(Session *s, u32 dur_ms, jlong out, u32 out_cap) {

    Vb vb_;

    Vb *vb = &vb_;

    vb_init_wrap(vb, out, out_cap);

    vb_w_u32(vb, dur_ms);

    AVFormatContext *fmt_ctx = s->fmt_ctx;

    u32 chapter_cnt = fmt_ctx->nb_chapters;

    vb_w_u31(vb, chapter_cnt);

    for (u32 i = 0; i < chapter_cnt; ++i) {

        AVChapter *chapter = fmt_ctx->chapters[i];

        AVRational tb = chapter->time_base;

        u64 start_ms_l = (u64) chapter->start * (u64) tb.num * 1000 / (u64) tb.den;

        u32 start_ms;

        if (start_ms_l > I32_MAX) {
            start_ms = U32_MAX; // will be rejected on java side
        }
        else {
            start_ms = (u32) start_ms_l;
        }

        vb_w_u32(vb, start_ms);

        {
            AVDictionary *d = chapter->metadata;

            AVDictionaryEntry *e = NULL;

            char *name = NULL;
            u32 name_len = 0;

            while ((e = av_dict_get(d, "", e, AV_DICT_IGNORE_SUFFIX)) != NULL) {

                if (strcmp(e->key, "title") == 0) {

                    size_t val_len = strlen(e->value);

                    if (val_len < 1024) {
                        name = e->value;
                        name_len = (u32) val_len;
                    }

                    break;
                }
            }

            vb_w_u8_arr(vb, (u8 *) name, name_len);
        }
    }

    vb_export_to_java(vb);

    return vb->len;
}

const u32 GET_FILE_INFO_FLAG_ONLY_DUR_MS = 1;

u32 ffmpeg_get_file_info(JNI_ARGS i32 fd, u64 fd_len, jlong out, u32 out_cap, u32 flags)
{
    Session sess_ = { 0 };
    Session *sess = &sess_;
    sess->fd = fd;
    sess->fd_len = fd_len;

    u32 dur_ms = 0;

    prepare_l2(sess, &dur_ms);

    u32 ret;

    if (dur_ms != 0) {

        if (flags & GET_FILE_INFO_FLAG_ONLY_DUR_MS) {
            ret = dur_ms;
        }
        else {
            ret = get_file_info_l1(sess, dur_ms, out, out_cap);
        }
    }
    else {
        ret = 0;
    }

    free_sess_l1(sess, true, false);

    return ret;
}


//    if (c_dev) {
//
//        if (fmt_ctx->nb_chapters != 0) {
//
//
//
//            for (u32 i = 0; i < fmt_ctx->nb_chapters; ++i) {
//
//                AVDictionary *d = fmt_ctx->chapters[i]->metadata;
//
//                AVDictionaryEntry *e = NULL;
//
//                while (e = av_dict_get(d, "", e, 2)) {
//                    log_d("chapter %u key: %s, value: %s", i, e->key, e->value);
//                }
//            }
//        }
//
//        if (fmt_ctx->metadata != NULL) {
//
//            AVDictionaryEntry *e = NULL;
//
//            while (e = av_dict_get(fmt_ctx->metadata, "", e, 2)) {
//                log_d("fmt ctx metadata key: %s, value: %s", e->key, e->value);
//            }
//        }
//
//
//
//    }


//        if (stream->metadata != NULL) {
//
//            AVDictionaryEntry *e = NULL;
//
//            while (e = av_dict_get(stream->metadata, "", e, 2)) {
//                log_d("stream metadata key: %s, value: %s", e->key, e->value);
//            }
//        }
