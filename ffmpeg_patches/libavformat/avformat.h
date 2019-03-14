--- /tmp/ffmpeg-4.1_orig/libavformat/avformat.h	2018-11-06 01:22:26.000000000 +0200
+++ /tmp/ffmpeg-4.1/libavformat/avformat.h	2018-11-13 10:53:27.985274448 +0200
@@ -1296,6 +1296,8 @@
     AVRational time_base;   ///< time base in which the start/end timestamps are specified
     int64_t start, end;     ///< chapter start/end time in time_base units
     AVDictionary *metadata;
+    int64_t cover_off;
+    uint32_t cover_len;
 } AVChapter;
 
 
@@ -1944,6 +1946,8 @@
      * - decoding: set by user
      */
     int skip_estimate_duration_from_pts;
+
+    char has_id3;
 } AVFormatContext;
 
 #if FF_API_FORMAT_GET_SET
