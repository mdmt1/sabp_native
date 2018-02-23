#pragma once

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "buf.c"

#include "sys/mman.h"

#define OP_OPEN_PATH 0
#define OP_STAT_PATH 1
#define OP_STAT_FD 2
#define OP_CLOSE 3
#define OP_UNLINK 4
#define OP_RMDIR 5
#define OP_MKDIR 6
#define OP_LSEEK_SET 7

const u32 OPEN_MODE_FILE = 0;
const u32 OPEN_MODE_DIR = 1;

const u32 OFF_MODE = 0;
const u32 OFF_SIZE = 4;
//const u32 OFF_DEV = 12;
//const u32 OFF_INO = 20;

#define JAVA_STAT_SIZE 12

bool stat_l1(int stat_ret, struct stat *ss, jlong j_out)
{
    if (stat_ret != 0) {
        return false;
    }

    u8 *out = (u8 *) j_out;
    u32 mode = ss->st_mode;
    u8_arr_set_u32(out + OFF_MODE, mode);

    u64 size = (u64) ss->st_size;
    u8_arr_set_u64(out + OFF_SIZE, size);

    return true;
}

i32 fs_op(
    JNI_ARGS
    u32 op, jint fd,
    i32 open_mode,
    jlong j_path,
    jlong j_out_stat
)
{
    char *path = (char *) j_path;

    i32 ret;

    bool op_is_open = false;
    bool op_is_stat = false;

    struct stat ss;

    switch (op)
    {
        case OP_OPEN_PATH:
        {
            int flags = (open_mode == OPEN_MODE_FILE)?
                O_RDONLY :
                (O_RDONLY | O_DIRECTORY);


            ret = open(path, flags);

            op_is_open = true;

            break;
        }

        case OP_STAT_PATH:
        {
            ret = stat(path, &ss);
            op_is_stat = true;
            break;
        }

        case OP_STAT_FD:
        {
            ret = fstat(fd, &ss);
            op_is_stat = true;
            break;
        }

        case OP_CLOSE:
        {
            close(fd);
            return 0;
        }

        case OP_UNLINK:
        {
            return unlink(path);
        }

        case OP_RMDIR:
        {
            return rmdir(path);
        }

        case OP_MKDIR:
        {
            return mkdir(path, 0777);
        }

        case OP_LSEEK_SET:
        {
            off64_t off = (off64_t) j_path;

            return (lseek64(fd, off, SEEK_SET) == off)? 0 : -1;
        }

        default:
        {
            return 0;
        }
    }

    if (
        op_is_open
        &&
        j_out_stat != 0
        &&
        ret != -1

    )
    {
        int out_fd = ret;
        int stat_ret = fstat(out_fd, &ss);

        if (! stat_l1(stat_ret, &ss, j_out_stat))
        {
            close(out_fd);
            ret = -1;
        }
    }

    if (op_is_stat)
    {
        stat_l1(ret, &ss, j_out_stat);
    }

    return ret;
}
