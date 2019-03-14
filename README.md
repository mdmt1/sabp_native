# sabp_native
Native component of [Simple ABP](https://play.google.com/store/apps/details?id=mdmt.sabp&hl=en)

# Build instructions
Prerequisites: Linux, Android NDK 19 (should be installed in its default location: $ANDROID_HOME/ndk-bundle), groovy, gpg, tar, patch, make.

1. Build FFmpeg static libraries:
`groovy build_ffmpeg.gr all`. Build trees are located in `/tmp/ffmpeg_builds`.

2. Build shared library:
`ndk-build -B (force rebuild) SABP_RELEASE=1 SABP_FULL=(0 or 1) RELEASE_LIB_NAME=(n plus last 5 digits of apk versionCode) APP_ABI=(one of armeabi-v7a arm64-v8a x86 x86_64)`
