#include "ffmpeg_sess.c"
#include "vb.h"

#include "ffmpeg_sess.h"
#include "c.h"
#include "ffmpeg_av_wrapper.h"

bool decode_chunk_l1(Session *sess, Vb *dst, u32 min_chunk_len, u64* out_first_ts)
{
    m_read_frame:
    {
        Res_read_frame r = read_frame(sess);

        if (r == READ_FRAME_RES_SUCCESS) {
            send_packet(sess);
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
        Res_receive_frame r = receive_frame(sess);

        if (r != RECEIVE_FRAME_RES_SUCCESS) {

            if (r == RECEIVE_FRAME_RES_NOT_ENOUGH_INPUT) {

                if (dst->len < min_chunk_len) {
                    goto m_read_frame;
                }
                else {
                    return true;
                }
            }
            else if (r == RECEIVE_FRAME_RES_END_OF_FILE) {
                sess->eof = true;
                return true;
            }
            else if (r == RECEIVE_FRAME_RES_BENIGN_ERROR) {
                goto m_receive_frame;
            }
        }

        AVFrame *frame = sess->frame;

        if (frame->sample_rate != sess->dec_ctx->sample_rate) {
            log_d("frame->sample_rate %u, sess->dec_ctx->sample_rate %u", frame->sample_rate, sess->dec_ctx->sample_rate);
        }

        sess->pos_ts = (u64) frame->best_effort_timestamp;

        if (out_first_ts != NULL) {
            *out_first_ts = sess->pos_ts;
            out_first_ts = NULL;
        }

        int in_sample_cnt = frame->nb_samples;

        SwrContext *swr_ctx = sess->swr_ctx;

        int ret = swr_get_out_samples(swr_ctx, in_sample_cnt);

        if (ret < 0) {
            log_ffmpeg_err("swr_get_out_samples", ret);
            return false;
        }

        u32 max_out_sample_cnt = (u32) ret;

        u32 req_cap = out_samples_to_bytes(sess, max_out_sample_cnt);

        vb_ensure_cap(dst, req_cap);

        u8 *dst_addr = dst->buf + dst->len;

        ret = swr_convert(swr_ctx, &dst_addr, max_out_sample_cnt, (const u8 **) &frame->data, in_sample_cnt);

        if (ret < 0) {
            log_ffmpeg_err("swr_convert", ret);
            return false;
        }

        int out_sample_cnt = ret;

        int out_byte_cnt = out_samples_to_bytes(sess, (u32) out_sample_cnt);

        dst->len += (u32) out_byte_cnt;

        goto m_receive_frame;
    }
}



i32 ffmpeg_decode_chunk(JNI_ARGS jlong j_sess, jlong j_dst, u32 dst_cap,
                        u32 min_chunk_len) {
    Session *sess = (Session *) j_sess;

    if (sess->eof) {
        return 0;
    }

    u8 *dst = (u8 *) j_dst;

    Vb vb_;
    Vb *vb = &vb_;

    vb_init_wrap(vb, j_dst, dst_cap);

    bool chunk_start_set = false;
    u64 chunk_start_ts = 0;

    {
        Vb *ls = sess->leftover_samples;

        if (ls->len != 0)
        {
            u32 off = sess->leftover_samples_off;
            u32 len = ls->len - off;

            vb_w_u8_arr_opaque(vb, ls->buf + off, len);

            ls->len = 0;

            chunk_start_ts = sess->pos_ts;
            chunk_start_set = true;
        }
    }


    bool ret = decode_chunk_l1(sess, vb, min_chunk_len, (chunk_start_set? NULL : &chunk_start_ts));

    if (! ret) {
        vb_free_buf(vb);
        return -1;
    }

    u32 chunk_start_ms = convert_ff_ts_to_ms(sess, chunk_start_ts);

    vb_w_u32(vb, chunk_start_ms);

    if ((vb->flags & VB_FLAG_BUF_FROM_JAVA_OVERFLOWED) == 0) {
        return vb->len;
    }

    // happens very rarely
    return_leftover_buf_with_len(vb->buf, vb->len, dst);

    return vb->cap;
}
