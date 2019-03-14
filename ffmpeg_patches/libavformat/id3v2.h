--- /tmp/ffmpeg-4.0/libavformat/id3v2.h	2018-04-20 13:02:57.000000000 +0300
+++ /code/ffmpeg-4.0/libavformat/id3v2.h	2018-04-30 14:55:59.468307090 +0300
@@ -70,6 +70,7 @@
 
 typedef struct ID3v2ExtraMetaAPIC {
     AVBufferRef *buf;
+    int64_t file_off;
     const char  *type;
     uint8_t     *description;
     enum AVCodecID id;
@@ -85,6 +86,8 @@
     uint8_t *element_id;
     uint32_t start, end;
     AVDictionary *meta;
+    int64_t cover_off;
+    uint32_t cover_len;
 } ID3v2ExtraMetaCHAP;
 
 /**
