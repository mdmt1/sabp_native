--- /tmp/ffmpeg-4.0/libavutil/log.c	2018-04-20 13:02:58.000000000 +0300
+++ /code/ffmpeg-4.0/libavutil/log.c	2018-04-30 15:30:11.101420631 +0300
@@ -288,6 +288,7 @@
 int av_log_format_line2(void *ptr, int level, const char *fmt, va_list vl,
                         char *line, int line_size, int *print_prefix)
 {
+#ifdef FFMPEG_ENABLE_LOGGING
     AVBPrint part[4];
     int ret;
 
@@ -295,10 +296,14 @@
     ret = snprintf(line, line_size, "%s%s%s%s", part[0].str, part[1].str, part[2].str, part[3].str);
     av_bprint_finalize(part+3, NULL);
     return ret;
+#else
+    return 0;
+#endif
 }
 
 void av_log_default_callback(void* ptr, int level, const char* fmt, va_list vl)
 {
+#ifdef FFMPEG_ENABLE_LOGGING
     static int print_prefix = 1;
     static int count;
     static char prev[LINE_SZ];
@@ -353,6 +358,7 @@
 end:
     av_bprint_finalize(part+3, NULL);
     ff_mutex_unlock(&mutex);
+#endif
 }
 
 static void (*av_log_callback)(void*, int, const char*, va_list) =
@@ -360,6 +366,7 @@
 
 void av_log(void* avcl, int level, const char *fmt, ...)
 {
+#ifdef FFMPEG_ENABLE_LOGGING
     AVClass* avc = avcl ? *(AVClass **) avcl : NULL;
     va_list vl;
     va_start(vl, fmt);
@@ -368,13 +375,16 @@
         level += *(int *) (((uint8_t *) avcl) + avc->log_level_offset_offset);
     av_vlog(avcl, level, fmt, vl);
     va_end(vl);
+#endif
 }
 
 void av_vlog(void* avcl, int level, const char *fmt, va_list vl)
 {
+#ifdef FFMPEG_ENABLE_LOGGING
     void (*log_callback)(void*, int, const char*, va_list) = av_log_callback;
     if (log_callback)
         log_callback(avcl, level, fmt, vl);
+#endif
 }
 
 int av_log_get_level(void)
@@ -389,7 +399,9 @@
 
 void av_log_set_flags(int arg)
 {
+#ifdef FFMPEG_ENABLE_LOGGING
     flags = arg;
+#endif
 }
 
 int av_log_get_flags(void)
@@ -399,7 +411,9 @@
 
 void av_log_set_callback(void (*callback)(void*, int, const char*, va_list))
 {
+#ifdef FFMPEG_ENABLE_LOGGING
     av_log_callback = callback;
+#endif
 }
 
 static void missing_feature_sample(int sample, void *avc, const char *msg,
@@ -418,18 +432,22 @@
 
 void avpriv_request_sample(void *avc, const char *msg, ...)
 {
+#ifdef FFMPEG_ENABLE_LOGGING
     va_list argument_list;
 
     va_start(argument_list, msg);
     missing_feature_sample(1, avc, msg, argument_list);
     va_end(argument_list);
+#endif
 }
 
 void avpriv_report_missing_feature(void *avc, const char *msg, ...)
 {
+#ifdef FFMPEG_ENABLE_LOGGING
     va_list argument_list;
 
     va_start(argument_list, msg);
     missing_feature_sample(0, avc, msg, argument_list);
     va_end(argument_list);
+#endif
 }
