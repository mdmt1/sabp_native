APP_PLATFORM = android-16

ifeq ($(SABP_RELEASE),1)
    APP_CFLAGS = -Oz -fvisibility=hidden -DSABP_FULL=$(SABP_FULL) -DSABP_RELEASE=$(SABP_RELEASE) -DNDEBUG
    APP_LDFLAGS = -flto -Wl,-gc-sections
else
    APP_CFLAGS = -Og
#    APP_LDFLAGS = -flto -Wl,-gc-sections
endif

APP_CFLAGS += -Wall -Wextra -Wconversion -Wpedantic \
-Wno-conversion -Wno-sign-conversion -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable -Wno-missing-field-initializers \
#-Werror
#-Wno-shorten-64-to-32


ifeq ($(APP_ABI), x86)
    APP_LDFLAGS += -Wl,--no-warn-shared-textrel
endif
