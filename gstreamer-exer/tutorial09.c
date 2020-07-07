#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <string.h>

#define CHUNK_SIZE 1024   /* Amount of bytes we are sending in each buffer */
#define SAMPLE_RATE 44100 /* Samples per second we are sending */

/* Structure to contain all our information, so we can pass it to callbacks */
typedef struct _CustomData {
    GstElement *pipeline, *app_source, *tee;
    GstElement *audio_queue, *audio_convert1, *audio_resample, *audio_sink;
    GstElement *video_queue, *audio_convert2, *visual, *video_convert, *video_sink;
    GstElement *app_queue, *app_sink;

    guint64 num_samples;   /* Number of samples generated so far (for timestamp generation) */
    gfloat a, b, c, d;     /* For waveform generation */

    guint sourceid;        /* To control the GSource */

    GMainLoop *main_loop;  /* GLib's Main Loop */
} CustomData;

/* This method is called by the idle GSource in the mainloop, to feed CHUNK_SIZE bytes into appsrc.
* The idle handler is added to the mainloop when appsrc requests us to start sending data (need-data signal)
* and is removed when appsrc has enough data (enough-data signal).
*/
static gboolean push_data (CustomData *data) {
    GstBuffer *buffer;
    GstFlowReturn ret;
    int i;
    GstMapInfo map;
    gint16 *raw;
    gint num_samples = CHUNK_SIZE / 2; /* Because each sample is 16 bits */
    gfloat freq;

    /* Create a new empty buffer */
    buffer = gst_buffer_new_and_alloc (CHUNK_SIZE);

    /* Set its timestamp and duration */
    GST_BUFFER_TIMESTAMP (buffer) = gst_util_uint64_scale (data->num_samples, GST_SECOND, SAMPLE_RATE); //(p1*p2)/p3
    GST_BUFFER_DURATION (buffer) = gst_util_uint64_scale (num_samples, GST_SECOND, SAMPLE_RATE);    //duration 是 事件长度

    /* Generate some psychodelic waveforms */
    gst_buffer_map (buffer, &map, GST_MAP_WRITE);
    raw = (gint16 *)map.data;

    //生成audio波形
    data->c += data->d;
    data->d -= data->c / 1000;
    freq = 1100 + 1000 * data->d;
    for (i = 0; i < num_samples; i++) {
        data->a += data->b;
        data->b -= data->a / freq;
        raw[i] = (gint16)(500 * data->a);
    } 

    gst_buffer_unmap (buffer, &map);
    data->num_samples += num_samples;

    /* Push the buffer into the appsrc */
    //另一种方式为通过调用 gst_app_src_push_buffer() 向appsrc填充数据,这种方式就需要在编译时链接 gstreamer-app-1.0库,
    //同时 gst_app_src_push_buffer() 会接管 GstBuffer 的所有权,调用者无需释放buffer
    g_signal_emit_by_name (data->app_source, "push-buffer", buffer, &ret);

    /* Free the buffer now that we are done with it */
    gst_buffer_unref (buffer);

    if (ret != GST_FLOW_OK) {
        /* We got some error, stop sending data */
        return FALSE;
    }

    //在所有数据都发送完成后，我们可以调用 gst_app_src_end_of_stream() 向Pipeline写入EOS事件

    return TRUE;
}

/* This signal callback triggers when appsrc needs data. Here, we add an idle handler
* to the mainloop to start pushing data into the appsrc */
static void start_feed (GstElement *source, guint size, CustomData *data) {
    if (data->sourceid == 0) {  //启动一个线程向appsrc写数据,这里给出的是 glib 的一种方式
        g_print ("Start feeding\n");
        data->sourceid = g_idle_add ((GSourceFunc) push_data, data);
    }
}

/* This callback triggers when appsrc has enough data and we can stop sending.
* We remove the idle handler from the mainloop */
static void stop_feed (GstElement *source, CustomData *data) {
    if (data->sourceid != 0) {
        g_print ("Stop feeding\n");
        g_source_remove (data->sourceid);
        data->sourceid = 0;
    }
}

/* The appsink has received a buffer */
static GstFlowReturn new_sample (GstElement *sink, CustomData *data) {
    GstSample *sample;

    /* Retrieve the buffer */
    g_signal_emit_by_name (sink, "pull-sample", &sample); //使用 "pull-sample" 信号提取sample
    if (sample) {
        //得到GstSample之后,我们可以通过gst_sample_get_buffer()得到Sample中所包含的GstBuffer,
        //再使用 GST_BUFFER_DATA,GST_BUFFER_SIZE 等接口访问其中的数据.
        g_print ("*");
        //使用完后，得到的GstSample同样需要通过gst_sample_unref()进行释放
        gst_sample_unref (sample);
        return GST_FLOW_OK;
    }

    return GST_FLOW_ERROR;
}

/* This function is called when an error message is posted on the bus */
static void error_cb (GstBus *bus, GstMessage *msg, CustomData *data) {
    GError *err;
    gchar *debug_info;

    /* Print error details on the screen */
    gst_message_parse_error (msg, &err, &debug_info);
    g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
    g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
    g_clear_error (&err);
    g_free (debug_info);

    g_main_loop_quit (data->main_loop);
}

int main(int argc, char *argv[]) {
    CustomData data;
    GstPad *tee_audio_pad, *tee_video_pad, *tee_app_pad;
    GstPad *queue_audio_pad, *queue_video_pad, *queue_app_pad;
    GstAudioInfo info;
    GstCaps *audio_caps;
    GstBus *bus;

    /* Initialize cumstom data structure */
    memset (&data, 0, sizeof (data));
    data.b = 1; /* For waveform generation */
    data.d = 1;

    /* Initialize GStreamer */
    gst_init (&argc, &argv);

    /* Create the elements */
    data.app_source = gst_element_factory_make ("appsrc", "audio_source");
    data.tee = gst_element_factory_make ("tee", "tee");
    data.audio_queue = gst_element_factory_make ("queue", "audio_queue");
    data.audio_convert1 = gst_element_factory_make ("audioconvert", "audio_convert1");
    data.audio_resample = gst_element_factory_make ("audioresample", "audio_resample");
    data.audio_sink = gst_element_factory_make ("autoaudiosink", "audio_sink");
    data.video_queue = gst_element_factory_make ("queue", "video_queue");
    data.audio_convert2 = gst_element_factory_make ("audioconvert", "audio_convert2");
    data.visual = gst_element_factory_make ("wavescope", "visual");
    data.video_convert = gst_element_factory_make ("videoconvert", "video_convert");
    data.video_sink = gst_element_factory_make ("autovideosink", "video_sink");
    data.app_queue = gst_element_factory_make ("queue", "app_queue");
    data.app_sink = gst_element_factory_make ("appsink", "app_sink");

    /* Create the empty pipeline */
    data.pipeline = gst_pipeline_new ("test-pipeline");

    if (!data.pipeline || !data.app_source || !data.tee || !data.audio_queue || !data.audio_convert1 ||
        !data.audio_resample || !data.audio_sink || !data.video_queue || !data.audio_convert2 || !data.visual ||
        !data.video_convert || !data.video_sink || !data.app_queue || !data.app_sink) {
        g_printerr ("Not all elements could be created.\n");
        return -1;
    }

    /* Configure wavescope */
    g_object_set (data.visual, "shader", 0, "style", 0, NULL);

    /* Configure appsrc */
    gst_audio_info_set_format (&info, GST_AUDIO_FORMAT_S16, SAMPLE_RATE, 1, NULL);
    audio_caps = gst_audio_info_to_caps (&info);    //或者使用 gst_caps_from_string() 得到 caps
    g_object_set (data.app_source, "caps", audio_caps, "format", GST_FORMAT_TIME, NULL);
    g_signal_connect (data.app_source, "need-data", G_CALLBACK (start_feed), &data);    //同时监听"need-data"与"enough-data"事件,
    g_signal_connect (data.app_source, "enough-data", G_CALLBACK (stop_feed), &data);   //这2个事件由appsrc在需要数据和缓冲区满时触发

    /* Configure appsink */
    g_object_set (data.app_sink, "emit-signals", TRUE, "caps", audio_caps, NULL);
    g_signal_connect (data.app_sink, "new-sample", G_CALLBACK (new_sample), &data);     //监听"new-sample"事件,用于appsink在收到数据时的处理
    gst_caps_unref (audio_caps);

    /* Link all elements that can be automatically linked because they have "Always" pads */
    gst_bin_add_many (GST_BIN (data.pipeline), data.app_source, data.tee,
        data.audio_queue, data.audio_convert1, data.audio_resample, data.audio_sink,
        data.video_queue, data.audio_convert2, data.visual, data.video_convert, data.video_sink,
        data.app_queue, data.app_sink, NULL);
    if (gst_element_link_many (data.app_source, data.tee, NULL) != TRUE ||
        gst_element_link_many (data.audio_queue, data.audio_convert1, data.audio_resample, data.audio_sink, NULL) != TRUE ||
        gst_element_link_many (data.video_queue, data.audio_convert2, data.visual, data.video_convert, data.video_sink, NULL) != TRUE ||
        gst_element_link_many (data.app_queue, data.app_sink, NULL) != TRUE) {
        g_printerr ("Elements could not be linked.\n");
        gst_object_unref (data.pipeline);
        return -1;
    }

    /* Manually link the Tee, which has "Request" pads */
    tee_audio_pad = gst_element_get_request_pad (data.tee, "src_%u");
    g_print ("Obtained request pad %s for audio branch.\n", gst_pad_get_name (tee_audio_pad));
    queue_audio_pad = gst_element_get_static_pad (data.audio_queue, "sink");

    tee_video_pad = gst_element_get_request_pad (data.tee, "src_%u");
    g_print ("Obtained request pad %s for video branch.\n", gst_pad_get_name (tee_video_pad));
    queue_video_pad = gst_element_get_static_pad (data.video_queue, "sink");
    
    tee_app_pad = gst_element_get_request_pad (data.tee, "src_%u");
    g_print ("Obtained request pad %s for app branch.\n", gst_pad_get_name (tee_app_pad));
    queue_app_pad = gst_element_get_static_pad (data.app_queue, "sink");
    if (gst_pad_link (tee_audio_pad, queue_audio_pad) != GST_PAD_LINK_OK ||
        gst_pad_link (tee_video_pad, queue_video_pad) != GST_PAD_LINK_OK ||
        gst_pad_link (tee_app_pad, queue_app_pad) != GST_PAD_LINK_OK) {
        g_printerr ("Tee could not be linked\n");
        gst_object_unref (data.pipeline);
        return -1;
    }
    gst_object_unref (queue_audio_pad);
    gst_object_unref (queue_video_pad);
    gst_object_unref (queue_app_pad);

    /* Instruct the bus to emit signals for each received message, and connect to the interesting signals */
    bus = gst_element_get_bus (data.pipeline);
    gst_bus_add_signal_watch (bus);
    g_signal_connect (G_OBJECT (bus), "message::error", (GCallback)error_cb, &data);
    gst_object_unref (bus);

    /* Start playing the pipeline */
    gst_element_set_state (data.pipeline, GST_STATE_PLAYING);

    /* Create a GLib Main Loop and set it to run */
    data.main_loop = g_main_loop_new (NULL, FALSE);
    g_main_loop_run (data.main_loop);

    /* Release the request pads from the Tee, and unref them */
    gst_element_release_request_pad (data.tee, tee_audio_pad);
    gst_element_release_request_pad (data.tee, tee_video_pad);
    gst_element_release_request_pad (data.tee, tee_app_pad);
    gst_object_unref (tee_audio_pad);
    gst_object_unref (tee_video_pad);
    gst_object_unref (tee_app_pad);

    /* Free resources */
    gst_element_set_state (data.pipeline, GST_STATE_NULL);
    gst_object_unref (data.pipeline);
    return 0;
}

