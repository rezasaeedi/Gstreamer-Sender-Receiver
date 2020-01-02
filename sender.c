#include <stdio.h>
#include <gst/gst.h>

static void handle_message(GstMessage *msg)
{
    GError *err;
    gchar *debug_info;
    switch (GST_MESSAGE_TYPE(msg))
    {
    case GST_MESSAGE_ERROR:
        gst_message_parse_error(msg, &err, &debug_info);
        g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
        g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
        g_clear_error(&err);
        g_free(debug_info);
        break;
    case GST_MESSAGE_EOS:
        g_print("End-Of-Stream reached.\n");
        break;
    default:
        /* We should not reach here because we only asked for ERRORs and EOS */
        g_printerr("Unexpected message received.\n");
        break;
    }
    gst_message_unref(msg);
}

int main(int argc, char *argv[])
{

    GstElement *source, *filter, *videoscale, *videoconvert, *encoder, *rtppay, *sink, *pipeline;
    GstCaps *filtercaps;
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;
    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    //create source
    source = gst_element_factory_make("videotestsrc", "source");
    g_object_set(source, "pattern", 1, NULL);

    //create filter
    filter = gst_element_factory_make("capsfilter", "filter");
    g_assert(filter != NULL); /* should always exist */
    filtercaps = gst_caps_new_simple("video/x-raw",
                                     //"format", G_TYPE_STRING, "RGB16",
                                     //"width", G_TYPE_INT, 384,
                                     //"height", G_TYPE_INT, 288,
                                     "framerate", GST_TYPE_FRACTION, 20, 1,
                                     NULL);
    g_object_set(G_OBJECT(filter), "caps", filtercaps, NULL);

    videoscale = gst_element_factory_make("videoscale", "videoscale");

    videoconvert = gst_element_factory_make("videoconvert", "videoconvert");

    encoder = gst_element_factory_make("x264enc", "encoder");
    //g_object_set(G_OBJECT(encoder), "tune", "zerolatency", NULL);
    gst_util_set_object_arg(G_OBJECT(encoder), "tune", "zerolatency");
    g_object_set(encoder, "bitrate", 500, NULL);
    //g_object_set(encoder, "speed-preset", "superfast", NULL);
    gst_util_set_object_arg(G_OBJECT(encoder), "speed-preset", "superfast");

    rtppay = gst_element_factory_make("rtph264pay", "rtppay");

    sink = gst_element_factory_make("udpsink", "udpsink");
    g_object_set(sink, "host", "127.0.0.1", NULL);
    g_object_set(sink, "port", 5004, NULL);

    //create pipeline
    pipeline = gst_pipeline_new("rtp-stream");

    gst_bin_add_many(GST_BIN(pipeline), source, filter, videoscale, videoconvert, encoder, rtppay, sink, NULL);

    if (gst_element_link_many(source,
                              filter, videoscale, videoconvert, encoder, rtppay, sink, NULL) != TRUE)
    {
        g_printerr("Elements could not be linked.\n");
        gst_object_unref(pipeline);
        return 0;
    }

    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    /* Listen to the bus */
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    if (msg != NULL)
    {
        handle_message(msg);
    }
    g_print("Ok\n");
    return 0;
}