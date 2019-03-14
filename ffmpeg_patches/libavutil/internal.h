--- /tmp/ffmpeg-4.0/libavutil/internal.h	2018-04-20 13:02:58.000000000 +0300
+++ /code/ffmpeg-4.0/libavutil/internal.h	2018-04-30 15:24:15.707943304 +0300
@@ -180,11 +180,7 @@
  * without modification. Used to disable the definition of strings
  * (for example AVCodec long_names).
  */
-#if CONFIG_SMALL
-#   define NULL_IF_CONFIG_SMALL(x) NULL
-#else
-#   define NULL_IF_CONFIG_SMALL(x) x
-#endif
+#define NULL_IF_CONFIG_SMALL(x) NULL
 
 /**
  * Define a function with only the non-default version specified.
