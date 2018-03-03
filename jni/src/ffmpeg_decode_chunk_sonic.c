#include "ffmpeg_sess.c"
#include "ffmpeg_sess.h"
#include "vb.h"
#include "ffmpeg_av_wrapper.h"

bool decode_chunk_sonic_l1(Session *sess, u32 min_chunk_len, u64* out_first_ts)
{
    sonicStream sonic = sess->sonic;
    min_chunk_len = out_bytes_to_samples(sess, min_chunk_len);

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

                if (sonic->numOutputSamples < min_chunk_len) {
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

        enlargeInputBufferIfNeeded(sonic, max_out_sample_cnt);

        u8 *dst_buf = (u8 *) sonic->inputBuffer + out_samples_to_bytes(sess, sonic->numInputSamples);

        ret = swr_convert(swr_ctx, &dst_buf, max_out_sample_cnt, (const u8 **) &frame->data, in_sample_cnt);

        if (ret < 0) {
            log_ffmpeg_err("swr_convert", ret);
            return false;
        }

        sonic->numInputSamples += ret;

        changeSpeed(sonic);

        goto m_receive_frame;
    }
}

i32 ffmpeg_decode_chunk_sonic(JNI_ARGS jlong j_sess, jlong j_dst, u32 dst_cap_in_bytes, jboolean flush_sonic, u32 min_chunk_len)
{
    Session *sess = (Session *) j_sess;

    if (sess->eof) {
        return 0;
    }

    u32 dst_cap = out_bytes_to_samples(sess, dst_cap_in_bytes);

    sonicStream sonic = sess->sonic;
    sonic->outputBuffer = (short *) j_dst;
    sonic->out_buf_malloced = false;
    sonic->numOutputSamples = 0;
    sonic->outputBufferSize = dst_cap;

    bool chunk_start_set = false;
    u64 chunk_start_ts = 0;

    {
        Vb *ls = sess->leftover_samples;

        if (ls->len != 0)
        {
            u32 off = sess->leftover_samples_off;
            u32 len = ls->len - off;

            u32 len_in_samples = out_bytes_to_samples(sess, len);

            enlargeInputBufferIfNeeded(sonic, len_in_samples);

            u8 *dst = (u8 *) sonic->inputBuffer + out_samples_to_bytes(sess, sonic->numInputSamples);

            memcpy(
                dst,
                ls->buf + off,
                len
            );

            sonic->numInputSamples += len_in_samples;

            ls->len = 0;

            chunk_start_ts = sess->pos_ts;
            chunk_start_set = true;
        }
    }

    if (flush_sonic) {

        sonicFlushStream(sonic);
    }
    else {
        bool ret = decode_chunk_sonic_l1(sess, min_chunk_len, (chunk_start_set? NULL : &chunk_start_ts));

        if (! ret) {

            if (sonic->out_buf_malloced) {
                free(sonic->outputBuffer);
            }

            return -1;
        }
    }

    u32 out_len_in_samples = sonic->numOutputSamples;
    u32 out_len = out_samples_to_bytes(sess, out_len_in_samples);

    if (! flush_sonic) {

        u32 pos_ms = convert_ff_ts_to_ms(sess, chunk_start_ts);

        enlargeOutputBufferIfNeeded(sonic, 1);

        u8_arr_set_u32((u8 *) sonic->outputBuffer + out_len, pos_ms);

        out_len += 4; // pos_ms
    }

    if (! sonic->out_buf_malloced) {
        return out_len;
    }

    // happens very rarely
    u8 *out = (u8 *) j_dst;

    return_leftover_buf_with_len(sonic->outputBuffer, out_len, out);

    return out_samples_to_bytes(sess, sonic->outputBufferSize);
}
