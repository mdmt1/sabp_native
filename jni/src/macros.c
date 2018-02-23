#pragma once

#define rptr *restrict
#define JNI (*env)->

#define RFIF(exp) \
    if (! (exp)) { \
        return false; \
    }

#define JNI_ARGS JNIEnv *env, jclass j_class,
