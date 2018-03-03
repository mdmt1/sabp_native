#pragma once

#include "types.h"
#include "macros.c"

typedef struct
{
    u8 rptr buf;
    u32 len;
    u32 cap;
    u32 flags;
    jlong j_buf;
} Vb;

#include "vb.c"
