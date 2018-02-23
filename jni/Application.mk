APP_PLATFORM = android-16

ifeq ($(NDK_DEBUG),0)
    APP_CFLAGS = -O2 -fvisibility=hidden -DSABP_FULL=$(SABP_FULL)
    APP_LDFLAGS = -flto -Wl,-gc-sections
else
ifeq ($(NDK_DEBUG),1)
    APP_CFLAGS = -Og
#    APP_LDFLAGS = -flto -Wl,-gc-sections
else
    $(error NDK_DEBUG should be 1 or 0)
endif
endif

APP_CFLAGS += -Wall -Wextra -Wconversion -Wpedantic \
-Wno-sign-conversion -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable -Wno-missing-field-initializers \
#-Werror
#-Wno-shorten-64-to-32


ifeq ($(APP_ABI), x86)
    APP_LDFLAGS += -Wl,--no-warn-shared-textrel
endif
