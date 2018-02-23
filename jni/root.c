#include "src/buf.c"

#include "src/ffmpeg_prepare.c"
#include "src/ffmpeg_sess.c"
#include "src/ffmpeg_seek.c"
#include "src/ffmpeg_decode_chunk.c"
#include "src/ffmpeg_decode_chunk_sonic.c"
#include "src/ffmpeg_set_speed.c"
#include "src/ffmpeg_get_file_info.c"
#include "src/fs_op.c"

JNIEXPORT void Java_n_N_i(JNIEnv *env, jclass class)
{
    ffmpeg_init();

    if (c_full)
    {
        JNINativeMethod methods[] =
        {
            { "get_buf_addr", "(Ljava/lang/Object;Z)J", (void *) get_buf_addr },
            { "mem_copy", "(JJIZ)V", (void *) mem_copy },
            { "ffmpeg_prepare", "(IJJIJ)I", (void *) ffmpeg_prepare },
            { "ffmpeg_set_speed", "(JFZ)Z", (void *) ffmpeg_set_speed },
            { "ffmpeg_decode_chunk", "(JJII)I", (void *) ffmpeg_decode_chunk },
            { "ffmpeg_decode_chunk_sonic", "(JJIZI)I", (void *) ffmpeg_decode_chunk_sonic },
            { "ffmpeg_seek", "(JI)Z", (void *) ffmpeg_seek },
            { "ffmpeg_free_sess", "(JZ)V", (void *) ffmpeg_free_sess },
            { "ffmpeg_get_file_info", "(IJJJII)I", (void *) ffmpeg_get_file_info },
            { "fs_op", "(IIIJJ)I", (void *) fs_op },
        };

        int ret = (*env)->RegisterNatives(env, class, methods, 10);

        assert(ret == 0);
    }
    else {

        JNINativeMethod methods[] =
        {
            { "get_buf_addr", "(Ljava/lang/Object;Z)J", (void *) get_buf_addr },
            { "mem_copy", "(JJIZ)V", (void *) mem_copy },
            { "ffmpeg_prepare", "(IJJIJ)I", (void *) ffmpeg_prepare },
            { "ffmpeg_set_speed", "(JFZ)Z", (void *) ffmpeg_set_speed },
            { "ffmpeg_decode_chunk", "(JJII)I", (void *) ffmpeg_decode_chunk },
    //        { "ffmpeg_decode_chunk_sonic", "(JJIZI)I", (void *) ffmpeg_decode_chunk_sonic },
            { "ffmpeg_seek", "(JI)Z", (void *) ffmpeg_seek },
            { "ffmpeg_free_sess", "(JZ)V", (void *) ffmpeg_free_sess },
            { "ffmpeg_get_file_info", "(IJJJII)I", (void *) ffmpeg_get_file_info },
            { "fs_op", "(IIIJJ)I", (void *) fs_op },
        };

        int ret = (*env)->RegisterNatives(env, class, methods, 9);

        assert(ret == 0);
    }
}
