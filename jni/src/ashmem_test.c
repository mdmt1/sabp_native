#include <dlfcn.h>
#include "macros.c"
#include "types.h"

i32 ashmem_test(JNI_ARGS u32 sdk_int)
{
    const char *dln;

    if (sdk_int >= 26) {
        dln = "libandroid.so";
    }
    else if (sdk_int >= 21) {
        dln = NULL;
    }
    else {
        dln = "libcutils.so";
    }

    void *handle = dlopen(dln, 0);

    if (handle == NULL) {
        return 1;
    }

    const char *fn_create = (sdk_int >= 26)?
        "ASharedMemory_create" :
        "ashmem_create_region";

    if (dlsym(handle, fn_create) == NULL) {
        return 2;
    }

    const char *fn_get_size = (sdk_int >= 26)?
        "ASharedMemory_getSize" :
        "ashmem_get_size_region";

    if (dlsym(handle, fn_get_size) == NULL) {
        return 3;
    }

    const char *fn_set_prot = (sdk_int >= 26)?
        "ASharedMemory_setProt" :
        "ashmem_set_prot_region";

    if (dlsym(handle, fn_set_prot) == NULL) {
        return 4;
    }

    return 0;
}
