#include "ffmpeg_sess.c"
#include "sonic.c"
#include "ffmpeg_sess.h"

jboolean ffmpeg_set_speed(JNI_ARGS jlong j_sess, f32 speed, jboolean pitch_compensation)
{
    Session *s = (Session *) j_sess;

    if (! c_full) {
        pitch_compensation = true;
    }

    if (pitch_compensation) {

        if (! set_out_sample_rate(s, s->native_sample_rate)) {
            return false;
        }

        if (! c_full) {
            return true;
        }

        if (speed != 1.0f) {

            if (s->sonic == NULL) {

                s->sonic = sonicCreateStream(
                    s->native_sample_rate,
                    (u8) s->out_ch_count
                );

                if (s->sonic == NULL) {
                    return false;
                }
            }

            s->sonic->speed = speed;
        }

        return true;
    }

    // speed change via resampling

    u32 sample_rate = (u32) ((f64) s->native_sample_rate / (f64) speed);

    return (jboolean) set_out_sample_rate(s, sample_rate);
}
