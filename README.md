# sabp_native
Native component of [Simple ABP](https://play.google.com/store/apps/details?id=mdmt.sabp&hl=en)

# Build instructions
Prerequisites: Linux, Android NDK 16 (should be installed in its default location: $ANDROID_HOME/ndk-bundle), gradle, gpg, tar, patch, make.

1. Set $ANDROID_NDK_TOOLCHAINS to a path where [standalone toolchains](https://developer.android.com/ndk/guides/standalone_toolchain.html) will be created. They will take up ~4 GB of space.

2. Build FFmpeg static libraries:
`gradle -b build_ffmpeg.gradle all`
Build trees are located in `/tmp/ffmpeg_builds`

3. Build shared library:
`ndk-build -B (force rebuild) NDK_DEBUG=0 RELEASE_LIB_NAME=(n plus apk versionCode) APP_ABI=(one of armeabi-v7a arm64-v8a x86 x86_64) SABP_FULL=(0 or 1)`
