--- /tmp/ffmpeg-4.0/libavformat/matroskadec.c	2018-04-20 13:02:57.000000000 +0300
+++ /code/ffmpeg-4.0/libavformat/matroskadec.c	2018-04-30 14:15:34.935375844 +0300
@@ -2689,6 +2689,7 @@
                 if ((res = av_new_packet(&st->attached_pic, attachments[j].bin.size)) < 0)
                     return res;
                 memcpy(st->attached_pic.data, attachments[j].bin.data, attachments[j].bin.size);
+                st->attached_pic.pos = attachments[j].bin.pos;
                 st->attached_pic.stream_index = st->index;
                 st->attached_pic.flags       |= AV_PKT_FLAG_KEY;
             } else {
