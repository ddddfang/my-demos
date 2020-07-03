//gst-launch-1.0 -v uridecodebin uri=file:///home/fang/testvideo/Titanic.ts name=decoder \
//  decoder. ! videoconvert ! autovideosink \
//  decoder. ! audioconvert ! autoaudiosink

#include <gst/gst.h>

typedef struct _PipelineBins {
    GstElement *pipeline;
    GstElement *source;
    GstElement *aconvert;
    GstElement *asink;
    GstElement *vconvert;
    GstElement *vsink;
} PipelineBins;

static void pad_added_handler(GstElement *src, GstPad *pad, PipelineBins *data);

int main(int argc, char *argv[]) {
    PipelineBins mybins;
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;
    gboolean terminate = FALSE;

    gst_init(&argc, &argv);

    mybins.source = gst_element_factory_make("uridecodebin", "source");
    mybins.aconvert = gst_element_factory_make("audioconvert", "aconvert");
    mybins.asink = gst_element_factory_make("autoaudiosink", "asink");
    mybins.vconvert = gst_element_factory_make("videoconvert", "vconvert");
    mybins.vsink = gst_element_factory_make("autovideosink", "vsink");

    mybins.pipeline = gst_pipeline_new("test-pipeline");                   //
    if (!mybins.source || !mybins.aconvert || !mybins.asink || !mybins.vconvert || !mybins.vsink || !mybins.pipeline) {
        g_printerr("Not all element could be created.\n");
        return -1;
    }

    gst_bin_add_many(GST_BIN(mybins.pipeline), mybins.source, mybins.aconvert, mybins.asink, mybins.vconvert, mybins.vsink, NULL);
    if (gst_element_link(mybins.aconvert, mybins.asink) != TRUE || gst_element_link(mybins.vconvert, mybins.vsink) != TRUE) {
        g_printerr("Element could not be linked.\n");
        gst_object_unref(mybins.pipeline);
        return -1;
    }

    g_object_set(mybins.source, "uri", "file:///home/fang/testvideo/Titanic.ts", NULL);

    g_signal_connect(mybins.source, "pad-added", G_CALLBACK(pad_added_handler), &mybins);

    ret = gst_element_set_state(mybins.pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state.");
        gst_object_unref(mybins.pipeline);
        return -1;
    }

    bus = gst_element_get_bus(mybins.pipeline);
    do {
        //
        msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_STATE_CHANGED);

        if (msg != NULL) {
            GError *err;
            gchar *debug_info;

            switch(GST_MESSAGE_TYPE(msg)) {
                case GST_MESSAGE_ERROR:
                    gst_message_parse_error(msg, &err, &debug_info);
                    g_printerr("Error recved from element %s:%s\n", GST_OBJECT_NAME(msg->src), err->message);
                    g_printerr("Debug info: %s\n", debug_info ? debug_info : "none");
                    g_clear_error(&err);
                    g_free(debug_info);
                    terminate = TRUE;
                    break;
                case GST_MESSAGE_EOS:
                    g_printerr("End-of-Stream reached.\n");
                    terminate = TRUE;
                    break;
                case GST_MESSAGE_STATE_CHANGED:
                    if (GST_MESSAGE_SRC(msg) == GST_OBJECT(mybins.pipeline)) {
                        GstState old_state, new_state, pending_state;
                        gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
                        g_print("Pipeline state changed from %s to %s.\n",
                            gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));
                    }
                    break;
                default:
                    g_printerr("Unpected message received.\n");
                    break;
            }
            gst_message_unref(msg);
        }
    } while (terminate == FALSE);
    gst_object_unref(bus);
    gst_element_set_state(mybins.pipeline, GST_STATE_NULL);
    gst_object_unref(mybins.pipeline);
    return 0;
}


static void pad_added_handler(GstElement *src, GstPad *new_pad, PipelineBins *data) {
    GstPad *sink_pad = NULL;
    GstPadLinkReturn ret;
    GstCaps *new_pad_caps = NULL;
    GstStructure *new_pad_struct = NULL;
    const gchar *new_pad_type = NULL;

    g_print("Received new pad '%s' from '%s':\n", GST_PAD_NAME(new_pad), GST_ELEMENT_NAME(src));

    //new_pad_caps = gst_pad_get_caps(new_pad);
    new_pad_caps = gst_pad_get_current_caps(new_pad);
    new_pad_struct = gst_caps_get_structure(new_pad_caps, 0);
    new_pad_type = gst_structure_get_name(new_pad_struct);

    if (g_str_has_prefix(new_pad_type, "audio/")) {
        sink_pad = gst_element_get_static_pad(data->aconvert, "sink");
        if (gst_pad_is_linked(sink_pad)) {
            g_print("--audio pad already linked, Ignoring.\n");
            goto EXIT;
        }
        ret = gst_pad_link(new_pad, sink_pad);
        if (GST_PAD_LINK_FAILED(ret)) {
            g_printerr("--Type is '%s' but link failed.\n", new_pad_type);
        } else {
            g_print("--Link successed(type '%s').\n", new_pad_type);
        }
    } else if (g_str_has_prefix(new_pad_type, "video/")) {
        sink_pad = gst_element_get_static_pad(data->vconvert, "sink");
        if (gst_pad_is_linked(sink_pad)) {
            g_print("--video pad already linked, Ignoring.\n");
            goto EXIT;
        }
        ret = gst_pad_link(new_pad, sink_pad);
        if (GST_PAD_LINK_FAILED(ret)) {
            g_printerr("--Type is '%s' but link failed.\n", new_pad_type);
        } else {
            g_print("--Link successed(type '%s').\n", new_pad_type);
        }
    }

EXIT:
    if (new_pad_caps != NULL) {
        gst_caps_unref(new_pad_caps);
    }
    gst_object_unref(sink_pad);
}

