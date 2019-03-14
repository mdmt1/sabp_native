--- /tmp/ffmpeg-4.0/libavformat/flac_picture.c	2018-04-20 13:02:57.000000000 +0300
+++ /code/ffmpeg-4.0/libavformat/flac_picture.c	2018-04-30 14:15:34.985376320 +0300
@@ -112,6 +112,7 @@
         RETURN_ERROR(AVERROR(ENOMEM));
     }
     memset(data->data + len, 0, AV_INPUT_BUFFER_PADDING_SIZE);
+    int64_t attached_pic_pos = avio_tell(pb);
     if (avio_read(pb, data->data, len) != len) {
         av_log(s, AV_LOG_ERROR, "Error reading attached picture data.\n");
         if (s->error_recognition & AV_EF_EXPLODE)
@@ -127,6 +128,7 @@
     av_init_packet(&st->attached_pic);
     st->attached_pic.buf          = data;
     st->attached_pic.data         = data->data;
+    st->attached_pic.pos          = attached_pic_pos;
     st->attached_pic.size         = len;
     st->attached_pic.stream_index = st->index;
     st->attached_pic.flags       |= AV_PKT_FLAG_KEY;
