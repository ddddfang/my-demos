//gst-launch-1.0 playbin uri=file:///home/fang/testvideo/Titanic.ts
//gst-launch-1.0  v4l2src ! video/x-raw,framerate=30/1 ! videoconvert ! autovideosink

#include <gst/gst.h>

int main(int argc, char *argv[]) {
    GstElement *pipeline;
    GstBus *bus;
    GstMessage *msg;

    gst_init(&argc, &argv);
    //pipeline = gst_parse_launch("playbin uri=file:///home/fang/testvideo/Titanic.ts", NULL);
    pipeline = gst_parse_launch("playbin uri=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm", NULL);
    //pipeline = gst_parse_launch("v4l2src ! video/x-raw,framerate=30/1 ! videoconvert ! autovideosink", NULL);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR|GST_MESSAGE_EOS);

    if (msg != NULL) {
        gst_message_unref(msg);
    }
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return 0;
}
