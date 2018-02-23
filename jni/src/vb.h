#pragma once

#include "types.h"
#include "macros.c"

typedef struct
{
    u8 rptr buf;
    u32 len;
    u32 cap;
    jlong j_buf;
    u32 flags;
} Vb;

#include "vb.c"
