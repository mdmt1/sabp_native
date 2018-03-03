#include "ffmpeg_sess.h"
#include "ffmpeg_sess.c"
#include "c.h"
#include "vb.h"
#include "ffmpeg_av_wrapper.h"
#include "ffmpeg_sess.h"
#include "ffmpeg_sess.c"
#include "c.h"
#include "vb.h"
#include "ffmpeg_av_wrapper.h"

bool ffmpeg_seek_l1(Session *s, const u32 req_pos_ms, u32 pos_ms_for_av_seek_frame, u32 call_counter)
{
    log_d("req_pos_ms %u || pos_ms_for_av_seek_frame %u || call_counter %u", req_pos_ms, pos_ms_for_av_seek_frame, call_counter);

    {
        u64 ts_for_av_seek_frame = convert_ms_to_ff_ts(s, pos_ms_for_av_seek_frame);

        int ret = av_seek_frame(s->fmt_ctx, s->stream->index, ts_for_av_seek_frame, AVSEEK_FLAG_BACKWARD);

        if (ret < 0) {
            log_ffmpeg_err("av_seek_frame", ret);
            return false;
        }
    }

    avcodec_flush_buffers(s->dec_ctx);

    if (s->sonic != NULL) {
        s->sonic->numInputSamples = 0;
    }

    // never NULL
    Vb *dst = s->leftover_samples;
    dst->len = 0;

    if (req_pos_ms == 0) {
        return true;
    }

    const u64 ts = convert_ms_to_ff_ts(s, req_pos_ms);

    const u64 pos_in_samples = convert_ff_ts_to_out_samples(s, ts);

    bool stop = false;

    // c_dev only
    u32 iter_cnt = 0;

    m_read_frame:
    {
        Res_read_frame r = read_frame(s);

        if (r == READ_FRAME_RES_SUCCESS) {
            send_packet(s);
        }
        else if (r == READ_FRAME_RES_END_OF_FILE) {
            goto m_receive_frame;
        }
        else if (r == READ_FRAME_RES_FAILURE) {
            return false;
        }
    };

    m_receive_frame:
    {
        Res_receive_frame r = receive_frame(s);

        if (r != RECEIVE_FRAME_RES_SUCCESS) {

            if (r == RECEIVE_FRAME_RES_NOT_ENOUGH_INPUT) {

                if (stop) {
                    return true;
                }

                goto m_read_frame;
            }
            else if (r == RECEIVE_FRAME_RES_END_OF_FILE) {
                s->eof = true;
                dst->len = 0;
                return false;
            }
            else if (r == RECEIVE_FRAME_RES_BENIGN_ERROR) {
                goto m_receive_frame;
            }
        }

        AVFrame *frame = s->frame;

        u64 frame_ts = (u64) frame->best_effort_timestamp;

        int in_sample_cnt = frame->nb_samples;

        SwrContext *swr_ctx = s->swr_ctx;

        int ret = swr_get_out_samples(swr_ctx, in_sample_cnt);

        if (ret < 0) {
            log_ffmpeg_err("swr_get_out_samples", ret);
            return false;
        }

        u32 max_out_sample_cnt = (u32) ret;

        u32 req_cap = out_samples_to_bytes(s, max_out_sample_cnt);

        vb_ensure_cap(dst, req_cap);

        u32 prev_dst_len = dst->len;

        u8 *dst_addr = dst->buf + dst->len;

        ret = swr_convert(swr_ctx, &dst_addr, max_out_sample_cnt, (const u8 **) &frame->data, in_sample_cnt);

        if (ret < 0) {
            log_ffmpeg_err("swr_convert", ret);
            return false;
        }

        u32 out_sample_cnt = (u32) ret;

        u32 out_byte_cnt = out_samples_to_bytes(s, out_sample_cnt);

        dst->len += (u32) out_byte_cnt;

        iter_cnt++;

        if (! stop) {

            u64 frame_start_in_samples = convert_ff_ts_to_out_samples(s, frame_ts);

            if (frame_start_in_samples > pos_in_samples) {
                // AVSEEK_FLAG_BACKWARD above didn't work, copy whole frame

                u32 ms_diff = convert_ff_ts_to_ms(s, (frame_ts - ts));

                if (call_counter < 10) {

                    log_d("ms_diff %u", ms_diff);

                    // drain frames

                    while (receive_frame(s) != RECEIVE_FRAME_RES_NOT_ENOUGH_INPUT) {}

                    dst->len = 0;

                    ++ call_counter;

                    u32 dec = 100 * call_counter + ms_diff;

                    if (pos_ms_for_av_seek_frame < dec) {
                        pos_ms_for_av_seek_frame = 0;
                    }
                    else {
                        pos_ms_for_av_seek_frame -= dec;
                    }

                    return ffmpeg_seek_l1(s, req_pos_ms, pos_ms_for_av_seek_frame, call_counter);
                }

                log_d("sample perfect seek failed, sample cnt diff %llu || ms diff %u || iter_cnt %u",
                      (frame_start_in_samples - pos_in_samples),
                      ms_diff,
                      iter_cnt);

                s->leftover_samples_off = prev_dst_len;
                s->pos_ts = frame_ts;

                stop = true;
            }
            else {
                u64 frame_end_in_samples = frame_start_in_samples + out_sample_cnt;

                if (frame_end_in_samples >= pos_in_samples)
                {
                    log_d("sample perfect seek achieved || iter_cnt %u", iter_cnt);

                    u32 frame_off_in_samples = (u32) (pos_in_samples - frame_start_in_samples);

                    s->leftover_samples_off = prev_dst_len + out_samples_to_bytes(s, frame_off_in_samples);
                    s->pos_ts = ts;
                    stop = true;
                }
                else {
                    dst->len = 0;
                }
            }
        }

        goto m_receive_frame;
    }
}

jboolean ffmpeg_seek(JNI_ARGS jlong j_sess, u32 pos_ms)
{
    Session *sess = (Session *) j_sess;

    return (jboolean) ffmpeg_seek_l1(sess, pos_ms, pos_ms, 0);
}
