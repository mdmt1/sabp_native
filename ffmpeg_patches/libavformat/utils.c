--- /tmp/ffmpeg-4.0/libavformat/utils.c	2018-04-20 13:02:58.000000000 +0300
+++ /code/ffmpeg-4.0/libavformat/utils.c	2018-05-07 12:22:27.551213599 +0300
@@ -626,6 +626,9 @@
     if (s->pb)
         ff_id3v2_read_dict(s->pb, &s->internal->id3v2_meta, ID3v2_DEFAULT_MAGIC, &id3v2_extra_meta);
 
+    if (s->internal->id3v2_meta) {
+        s->has_id3 = 1;
+    }
 
     if (!(s->flags&AVFMT_FLAG_PRIV_OPT) && s->iformat->read_header)
         if ((ret = s->iformat->read_header(s)) < 0)

@@ -2087,62 +2087,7 @@
     return m;
 }
 
-void ff_configure_buffers_for_index(AVFormatContext *s, int64_t time_tolerance)
-{
-    int ist1, ist2;
-    int64_t pos_delta = 0;
-    int64_t skip = 0;
-    //We could use URLProtocol flags here but as many user applications do not use URLProtocols this would be unreliable
-    const char *proto = avio_find_protocol_name(s->url);
-
-    if (!proto) {
-        av_log(s, AV_LOG_INFO,
-               "Protocol name not provided, cannot determine if input is local or "
-               "a network protocol, buffers and access patterns cannot be configured "
-               "optimally without knowing the protocol\n");
-    }
-
-    if (proto && !(strcmp(proto, "file") && strcmp(proto, "pipe") && strcmp(proto, "cache")))
-        return;
-
-    for (ist1 = 0; ist1 < s->nb_streams; ist1++) {
-        AVStream *st1 = s->streams[ist1];
-        for (ist2 = 0; ist2 < s->nb_streams; ist2++) {
-            AVStream *st2 = s->streams[ist2];
-            int i1, i2;
-
-            if (ist1 == ist2)
-                continue;
-
-            for (i1 = i2 = 0; i1 < st1->nb_index_entries; i1++) {
-                AVIndexEntry *e1 = &st1->index_entries[i1];
-                int64_t e1_pts = av_rescale_q(e1->timestamp, st1->time_base, AV_TIME_BASE_Q);
-
-                skip = FFMAX(skip, e1->size);
-                for (; i2 < st2->nb_index_entries; i2++) {
-                    AVIndexEntry *e2 = &st2->index_entries[i2];
-                    int64_t e2_pts = av_rescale_q(e2->timestamp, st2->time_base, AV_TIME_BASE_Q);
-                    if (e2_pts - e1_pts < time_tolerance)
-                        continue;
-                    pos_delta = FFMAX(pos_delta, e1->pos - e2->pos);
-                    break;
-                }
-            }
-        }
-    }
-
-    pos_delta *= 2;
-    /* XXX This could be adjusted depending on protocol*/
-    if (s->pb->buffer_size < pos_delta && pos_delta < (1<<24)) {
-        av_log(s, AV_LOG_VERBOSE, "Reconfiguring buffers to size %"PRId64"\n", pos_delta);
-        ffio_set_buf_size(s->pb, pos_delta);
-        s->pb->short_seek_threshold = FFMAX(s->pb->short_seek_threshold, pos_delta/2);
-    }
-
-    if (skip < (1<<23)) {
-        s->pb->short_seek_threshold = FFMAX(s->pb->short_seek_threshold, skip);
-    }
-}
+void ff_configure_buffers_for_index(AVFormatContext *s, int64_t time_tolerance) {}
 
 int av_index_search_timestamp(AVStream *st, int64_t wanted_timestamp, int flags)
 {

