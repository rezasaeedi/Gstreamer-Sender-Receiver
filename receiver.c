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

static void pad_added_handler(GstElement *src, GstPad *new_pad, GstElement *converter)
{
    GstPad *sink_pad = gst_element_get_static_pad(converter, "sink");
    GstPadLinkReturn ret;
    GstCaps *new_pad_caps = NULL;
    GstStructure *new_pad_struct = NULL;
    const gchar *new_pad_type = NULL;

    g_print("Received new pad '%s' from '%s':\n", GST_PAD_NAME(new_pad), GST_ELEMENT_NAME(src));

    /* If our converter is already linked, we have nothing to do here */
    if (gst_pad_is_linked(sink_pad))
    {
        g_print("We are already linked. Ignoring.\n");
        goto exit;
    }

    /* Check the new pad's type */
    new_pad_caps = gst_pad_get_current_caps(new_pad);
    new_pad_struct = gst_caps_get_structure(new_pad_caps, 0);
    new_pad_type = gst_structure_get_name(new_pad_struct);
    if (!g_str_has_prefix(new_pad_type, "video/x-raw"))
    {
        g_print("It has type '%s' which is not raw audio. Ignoring.\n", new_pad_type);
        goto exit;
    }

    /* Attempt the link */
    ret = gst_pad_link(new_pad, sink_pad);
    if (GST_PAD_LINK_FAILED(ret))
    {
        g_print("Type is '%s' but link failed.\n", new_pad_type);
    }
    else
    {
        g_print("Link succeeded (type '%s').\n", new_pad_type);
    }

exit:
    /* Unreference the new pad's caps, if we got them */
    if (new_pad_caps != NULL)
        gst_caps_unref(new_pad_caps);

    /* Unreference the sink pad */
    gst_object_unref(sink_pad);
}

int main(int argc, char *argv[])
{

    GstElement *source, *jitterbuffer, *rtpdepay, *decoder, *converter, *sink, *pipeline;
    GstCaps *capssrc;
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;
    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    //create source
    source = gst_element_factory_make("udpsrc", "source");
    //g_object_set(source, "port", 5004, NULL);

    capssrc = gst_caps_new_simple("application/x-rtp",
                                  "media", G_TYPE_STRING, "video",
                                  "encoding-name", G_TYPE_STRING, "H264",
                                  "payload", G_TYPE_INT, 96,
                                  NULL);

    //g_object_set(G_OBJECT(source), "caps", capssrc, NULL);
    g_object_set(G_OBJECT(source), "port", 5004, "caps", capssrc, NULL);
    gst_caps_unref(capssrc);

    jitterbuffer = gst_element_factory_make("rtpjitterbuffer", "buffer");

    rtpdepay = gst_element_factory_make("rtph264depay", "rtpdepay");

    //decoder = gst_element_factory_make("avdec_h264", "decoder");
    decoder = gst_element_factory_make("decodebin", "decoder");

    converter = gst_element_factory_make("videoconvert", "converter");

    sink = gst_element_factory_make(/*"autovideosink"*/ "ximagesink", "sink");

    //create pipeline
    pipeline = gst_pipeline_new("rtp-stream");

    if (!pipeline || !source || !jitterbuffer || !rtpdepay || !decoder || !converter || !sink)
    {
        g_printerr("One element could not be created. Exiting.\n");
        return -1;
    }

    gst_bin_add_many(GST_BIN(pipeline), source, jitterbuffer, rtpdepay, decoder, converter, sink, NULL);

    /*if (gst_element_link_many(source, jitterbuffer, rtpdepay, decoder, converter, sink, NULL) != TRUE)
    {
        g_printerr("Elements could not be linked.\n");
        gst_object_unref(pipeline);
        return 0;
    }*/
    if (gst_element_link_many(source, jitterbuffer, rtpdepay,  decoder, NULL) != TRUE)
    {
        g_printerr("Elements befor decoder could not be linked.\n");
        gst_object_unref(pipeline);
        return 0;
    }


    if (gst_element_link_many(converter, sink, NULL) != TRUE)
    {
        g_printerr("Elements after could not be linked.\n");
        gst_object_unref(pipeline);
        return 0;
    }

    /* Connect to the pad-added signal */
    g_signal_connect(decoder, "pad-added", G_CALLBACK(pad_added_handler), converter);
    g_print("Signal added.\n");


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