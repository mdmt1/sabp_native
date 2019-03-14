--- /tmp/ffmpeg-4.0/libavformat/id3v2.c	2018-04-20 13:02:57.000000000 +0300
+++ /code/ffmpeg-4.0/libavformat/id3v2.c	2018-05-07 14:24:40.607617676 +0300
@@ -339,7 +339,8 @@
         genre <= ID3v1_GENRE_MAX) {
         av_freep(&dst);
         dst = av_strdup(ff_id3v1_genre_str[genre]);
-    } else if (!(strcmp(key, "TXXX") && strcmp(key, "TXX"))) {
+    } else if (!(strcmp(key, "TXXX") && strcmp(key, "TXX")
+                 && strcmp(key, "WXXX") && strcmp(key, "WXX"))) {
         /* dst now contains the key, need to get value */
         key = dst;
         if (decode_str(s, pb, encoding, &dst, &taglen) < 0) {         
@@ -624,11 +624,7 @@
         }
         mime++;
     }
-    if (id == AV_CODEC_ID_NONE) {
-        av_log(s, AV_LOG_WARNING,
-               "Unknown attached picture mimetype: %s, skipping.\n", mimetype);
-        goto fail;
-    }
+
     apic->id = id;
 
     /* picture type */
@@ -649,10 +650,13 @@
     }
 
     apic->buf = av_buffer_alloc(taglen + AV_INPUT_BUFFER_PADDING_SIZE);
+    apic->file_off = avio_tell(pb);
+
     if (!apic->buf || !taglen || avio_read(pb, apic->buf->data, taglen) != taglen)
         goto fail;
     memset(apic->buf->data + taglen, 0, AV_INPUT_BUFFER_PADDING_SIZE);
 
+
     new_extra->tag  = "APIC";
     new_extra->data = apic;
     new_extra->next = *extra_meta;
@@ -712,10 +716,55 @@
         len -= 10;
         if (taglen < 0 || taglen > len)
             goto fail;
-        if (tag[0] == 'T')
+        if (tag[0] == 'T' || tag[0] == 'W')
             read_ttag(s, pb, taglen, &chap->meta, tag);
-        else
-            avio_skip(pb, taglen);
+        else {
+            int64_t tag_start = avio_tell(pb);
+            int64_t tag_end = tag_start + taglen;
+
+            if (chap->cover_off == 0 && memcmp(tag, "APIC", 4) == 0) {
+                /*
+                 * APIC format:
+                 * 1 byte text encoding
+                 * img format string
+                 * 1 byte picture type
+                 * description string
+                 * bitmap
+                 */
+
+                int enc = avio_r8(pb);
+
+                // format
+                while (avio_r8(pb) != 0) {}
+
+                if (avio_tell(pb) >= tag_end) {
+                    goto apic_read_end;
+                }
+
+                // type
+                avio_r8(pb);
+
+                {
+                    int rem = taglen;
+                    uint8_t *desc = NULL;
+                    decode_str(s, pb, enc, &desc, &rem);
+                    av_free(desc);
+                }
+
+                int64_t cover_off = avio_tell(pb);
+
+                if (cover_off < tag_end) {
+                    uint32_t cover_len = (uint32_t) (tag_end - cover_off);
+                    chap->cover_off = cover_off;
+                    chap->cover_len = cover_len;
+                }
+            }
+
+            apic_read_end: {
+                avio_seek(pb, tag_end, SEEK_SET);
+            }
+        }
+
         len -= taglen;
     }
 
@@ -961,7 +1010,7 @@
             av_log(s, AV_LOG_WARNING, "Skipping %s ID3v2 frame %s.\n", type, tag);
             avio_skip(pb, tlen);
         /* check for text tag or supported special meta tag */
-        } else if (tag[0] == 'T' ||
+        } else if (tag[0] == 'T' || tag[0] == 'W' ||
                    !memcmp(tag, "USLT", 4) ||
                    !strcmp(tag, comm_frame) ||
                    (extra_meta &&
@@ -1028,7 +1077,7 @@
                     pbx = &pb_local; // read from sync buffer
                 }
 #endif
-            if (tag[0] == 'T')
+            if (tag[0] == 'T' || tag[0] == 'W')
                 /* parse text tag */
                 read_ttag(s, pbx, tlen, metadata, tag);
             else if (!memcmp(tag, "USLT", 4))
@@ -1166,6 +1215,7 @@
 
         av_init_packet(&st->attached_pic);
         st->attached_pic.buf          = apic->buf;
+        st->attached_pic.pos          = apic->file_off;
         st->attached_pic.data         = apic->buf->data;
         st->attached_pic.size         = apic->buf->size - AV_INPUT_BUFFER_PADDING_SIZE;
         st->attached_pic.stream_index = st->index;
@@ -1219,6 +1269,9 @@
         if (!chapter)
             continue;
 
+        chapter->cover_off = chap->cover_off;
+        chapter->cover_len = chap->cover_len;
+
         if ((ret = av_dict_copy(&chapter->metadata, chap->meta, 0)) < 0)
             goto end;
     }   

