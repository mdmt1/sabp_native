#pragma once

#include "c.h"
#include "buf.c"
#include "vb.h"

const u32 VB_FLAG_BUF_FROM_JAVA = 1;
const u32 VB_FLAG_BUF_FROM_JAVA_OVERFLOWED = (1 << 1);

void vb_init_wrap(Vb *vb, jlong j_buf, u32 cap) {

    vb->buf = (u8 *) j_buf;
    vb->len = 0;
    vb->cap = cap;
    vb->j_buf = j_buf;
    vb->flags = VB_FLAG_BUF_FROM_JAVA;
}

void vb_init(Vb *vb) {

    vb->buf = NULL;
    vb->len = 0;
    vb->cap = 0;
    vb->j_buf = 0;
    vb->flags = 0;
}

void vb_ensure_cap(Vb *vb, u32 cap) {

    u32 len = vb->len;
    u32 req_cap = len + cap;

    if (req_cap <= vb->cap)
    {
        return;
    }

    u32 new_cap = req_cap + 1024;

    u32 flags = vb->flags;

    if (flags & VB_FLAG_BUF_FROM_JAVA) {

        if (flags & VB_FLAG_BUF_FROM_JAVA_OVERFLOWED) {

            vb->buf = realloc(vb->buf, new_cap);
        }
        else {
            u8 *new_buf = malloc(new_cap);
            memcpy(new_buf, vb->buf, len);

            vb->buf = new_buf;
            vb->flags |= VB_FLAG_BUF_FROM_JAVA_OVERFLOWED;
        }
    }
    else {
        vb->buf = realloc(vb->buf, new_cap);
    }

    vb->cap = new_cap;
}

void vb_skip(Vb *vb, u32 cnt) {

    vb_ensure_cap(vb, cnt);

    vb->len += cnt;
}

void vb_w_u8(Vb *vb, u8 a_u8) {

    vb_ensure_cap(vb, 1);

    vb->buf[vb->len++] = a_u8;
}

void vb_w_u31(Vb *vb, u32 u31) {

    vb_ensure_cap(vb, 5);

    bool eov = false;

    while (! eov)
    {
        u32 u7 = u31 & 0x7F;
        eov = (u31 == u7);

        if (eov) {
            u7 |= 0x80;
        }
        else {
            u31 >>= 7;
        }

        vb->buf[vb->len++] = (u8) u7;
    }
}

void vb_w_u32(Vb *vb, u32 v) {

    vb_ensure_cap(vb, 4);

    u8 *b = vb->buf;
    u32 l = vb->len;

    b[l] = (u8) v;
    b[l + 1] = (u8) (v >> 8);
    b[l + 2] = (u8) (v >> 16);
    b[l + 3] = (u8) (v >> 24);

    vb->len = l + 4;
}

void vb_w_u64(Vb *vb, u64 v) {

    vb_ensure_cap(vb, 8);

    u8 *b = vb->buf;
    u32 l = vb->len;

    b[l] = (u8) v;
    b[l + 1] = (u8) (v >> 8);
    b[l + 2] = (u8) (v >> 16);
    b[l + 3] = (u8) (v >> 24);
    b[l + 4] = (u8) (v >> 32);
    b[l + 5] = (u8) (v >> 40);
    b[l + 6] = (u8) (v >> 48);
    b[l + 7] = (u8) (v >> 56);

    vb->len = l + 8;
}

void vb_w_u63(Vb *vb, u64 v) {

    vb_ensure_cap(vb, 10);

    bool eov = false;

    while (! eov)
    {
        u64 u7 = v & 0x7F;
        eov = (bool) (v == u7);

        if (eov) {
            u7 |= 0x80;
        }
        else {
            v >>= 7;
        }

        vb->buf[vb->len++] = (u8) u7;
    }
}

void vb_w_u8_arr(Vb *vb, u8 *arr, u32 arr_len) {

    vb_w_u31(vb, arr_len);

    if (arr_len == 0) {
        return;
    }

    vb_ensure_cap(vb, arr_len);

    u8 *buf = vb->buf + vb->len;

    memcpy(buf, arr, arr_len);

    vb->len += arr_len;
}

void vb_w_u8_arr_opaque(Vb *vb, u8 *arr, u32 arr_len) {

    if (arr_len == 0) {
        return;
    }

    vb_ensure_cap(vb, arr_len);

    u8 *buf = vb->buf + vb->len;

    log_d("vb_len %d arr_len %d", vb->len, arr_len);

    memcpy(buf, arr, arr_len);

    vb->len += arr_len;
}

void vb_w_c_str_opaque(Vb *vb, char *str) {

    u32 str_len = (u32) strlen(str);
    vb_ensure_cap(vb, str_len);

    if (str_len == 0) {
        return;
    }

    u8 *buf = vb->buf + vb->len;
    memcpy(buf, str, str_len);

    vb->len += str_len;
}

void vb_free_buf(Vb *vb) {

    if (
        (vb->flags & (VB_FLAG_BUF_FROM_JAVA | VB_FLAG_BUF_FROM_JAVA_OVERFLOWED))
        ==
        VB_FLAG_BUF_FROM_JAVA

    ) { // buf not overflowed
        return;
    }

    // free of NULL is allowed: no-op
    free(vb->buf);
}

void u8_arr_set_u32(u8 *arr, u32 val)
{
    arr[0] = (u8) val;
    arr[1] = (u8) (val >> 8);
    arr[2] = (u8) (val >> 16);
    arr[3] = (u8) (val >> 24);
}

void u8_arr_set_u64(u8 *arr, u64 val)
{
    arr[0] = (u8) val;
    arr[1] = (u8) (val >> 8);
    arr[2] = (u8) (val >> 16);
    arr[3] = (u8) (val >> 24);
    arr[4] = (u8) (val >> 32);
    arr[5] = (u8) (val >> 40);
    arr[6] = (u8) (val >> 48);
    arr[7] = (u8) (val >> 56);
}

void vb_export_to_java(Vb *vb) {

    u32 export_flags = VB_FLAG_BUF_FROM_JAVA | VB_FLAG_BUF_FROM_JAVA_OVERFLOWED;

    if ((vb->flags & export_flags) == export_flags) {

        u8_arr_set_u64((u8 *) vb->j_buf, (u64) vb->buf);
        return;
    }

    assert(vb->flags == VB_FLAG_BUF_FROM_JAVA);
}
