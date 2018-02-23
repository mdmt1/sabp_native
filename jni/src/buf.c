#pragma once

#include "types.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "macros.c"
#include "vb.c"

u64 get_buf_addr(JNI_ARGS jobject buf, jboolean regular)
{
    if (regular) {
        jboolean is_copy;
        jbyte *addr = JNI GetByteArrayElements(env, buf, &is_copy);
        JNI ReleaseByteArrayElements(env, buf, addr, 0);

        if (is_copy) {
            // may happen on ART
            return 0;
        }

        return (u64) addr;
    }

    return (u64) JNI GetDirectBufferAddress(env, buf);
}

void mem_copy(JNI_ARGS jlong j_src, jlong j_dst, u32 len, jboolean free_src)
{
    u8 *src = (u8 *) j_src;
    u8 *dst = (u8 *) j_dst;

    memcpy(dst, src, (size_t) len);

    if (free_src) {
        free(src);
    }
}

void return_leftover_buf_with_len(void *buf, u32 len, u8 *dst)
{
    u8_arr_set_u64(dst, (u64) buf);
    u8_arr_set_u32(dst + 8, len);
}

