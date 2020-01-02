# Gstreamer Readme

## Install GStreamer on Ubuntu or Debian
Run the following command:
```
apt-get install libgstreamer1.0-0 gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-doc gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio
```

## Compile

For compile se this specific command on Linux:
```
gcc sender.c -o sender `pkg-config --cflags --libs gstreamer-1.0`
gcc receiver.c -o receiver `pkg-config --cflags --libs gstreamer-1.0`
```

## Initialize gstreamer

Using the following code to initialize gstreamer:

```c
gst_init (&argc, &argv);
```

In a gstreamer we need used a pipeline for running and transfering video streams. each pipeline is contain of many sources and sinks. source is the producer side of data and sink is the consumer side of data.

## Element and Pipeline creation

with the following code can create source and sink elements and pipeline:

```c
GstElement *source, *sink, *pipeline;
source = gst_element_factory_make ("videotestsrc", "source");
sink = gst_element_factory_make ("ximagesink", "sink");
pipeline = gst_pipeline_new ("test-pipeline");
```

A GstElementFactory is in charge of instantiating a particular type of element, identified by its factory name.
You can use `gst_element_factory_find()` to create a factory of type “videotestsrc”, and then use it to instantiate multiple “videotestsrc” elements using `gst_element_factory_create()`. 
`gst_element_factory_make() is really a shortcut for gst_element_factory_find()+ gst_element_factory_create().`

```c
/* Create the element factories */
source_factory = gst_element_factory_find ("audiotestsrc");
sink_factory = gst_element_factory_find ("autoaudiosink");
if (!source_factory || !sink_factory) {
  g_printerr ("Not all element factories could be created.\n");
  return -1;
}

/* Ask the factories to instantiate actual elements */
source = gst_element_factory_create (source_factory, "source");
sink = gst_element_factory_create (sink_factory, "sink");
```

Each element in gstreamer is a gst_element that can be added to pipeline by the following code:

```c
gst_bin_add_many (GST_BIN (pipeline), source, sink, NULL);
```

`Notic:` A bin is a `container element`. You can add elements to a bin. Since a bin is an element itself, a bin can be handled in the same way as any other element. Therefore, the whole Elements applies to bins as well. `Bins allow you to combine a group of linked elements into one logical element.`

![bin-element](img/bin-element.png)

## Properties

Properties of gstreamer elements are read from with g_object_get() and written to with g_object_set():

```c
g_object_set (source, "pattern", 0, NULL);
```

`Set multiple properties:`

```c
g_object_set(G_OBJECT(source), "port", 5004, "caps", capssrc, NULL);
```

## caps

The `Capabilities` (or Caps, for short) of a Pad, then, `specify what kind of information can travel through the Pad`. For example, “RGB video with a resolution of 320x200 pixels and 30 frames per second”, or “16-bits per sample audio, 5.1 channels at 44100 samples per second”, or even compressed formats like mp3 or h264.

`set caps:`

```c
GstCaps capssrc = gst_caps_new_simple("application/x-rtp",
                "media", G_TYPE_STRING, "video",
                "encoding-name", G_TYPE_STRING, "H264",
                "payload", G_TYPE_INT, 96);

g_object_set(source, "caps", capssrc, NULL);
gst_caps_unref(capssrc);
```

or

```c
GstCaps filter = gst_element_factory_make("capsfilter", "filter");
g_assert(filter != NULL); /* should always exist */
filtercaps = gst_caps_new_simple("video/x-raw",
                                 "format", G_TYPE_STRING, "RGB16",
                                 "width", G_TYPE_INT, 384,
                                 "height", G_TYPE_INT, 288,
                                 "framerate", GST_TYPE_FRACTION, 20, 1,
                                 NULL);

g_object_set(G_OBJECT(filter), "caps", filtercaps, NULL);
```

## Linking elements

By linking a source element with zero or more filter-like elements and finally a sink element, you set up a media pipeline. Data will flow through the elements. This is the basic concept of media handling in GStreamer.

![pipeline](img/pipeline.png)

```c
gst_element_link (source, sink)
```

`Check for linking error:`

```c
if (gst_element_link (source, sink) != TRUE) {
  g_printerr ("Elements could not be linked.\n");
  gst_object_unref (pipeline);
  return -1;
}
```

## Change state of the pipeline

`Check for error in changing state is showned:`

```c
ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
if (ret == GST_STATE_CHANGE_FAILURE) {
  g_printerr ("Unable to set the pipeline to the playing state.\n");
  gst_object_unref (pipeline);
  return -1;
}
```

## Error checking

`gst_bus_timed_pop_filtered()` waits for execution to end and returns with a `GstMessage`. We asked `gst_bus_timed_pop_filtered()` to return when GStreamer encountered either an error condition or an EOS, so we need to check which one happened, and print a message on screen (Your application will probably want to undertake more complex actions).

```c
/* Wait until error or EOS */
bus = gst_element_get_bus (pipeline);
msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

/* Parse message */
if (msg != NULL) {
  GError *err;
  gchar *debug_info;

  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_ERROR:
      gst_message_parse_error (msg, &err, &debug_info);
      g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
      g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
      g_clear_error (&err);
      g_free (debug_info);
      break;
    case GST_MESSAGE_EOS:
      g_print ("End-Of-Stream reached.\n");
      break;
    default:
      /* We should not reach here because we only asked for ERRORs and EOS */
      g_printerr ("Unexpected message received.\n");
      break;
  }
  gst_message_unref (msg);
}
```

## The GStreamer bus

It is the object `responsible for delivering to the application the GstMessages generated by the elements`, in order and to the application thread. This last point is important, because the actual streaming of media is done in `another thread` than the application.
Messages can be extracted from the bus `synchronously` with `gst_bus_timed_pop_filtered()` and its siblings, or asynchronously, using signals. Your application should always keep an eye on the bus to be notified of errors and other playback-related issues.

## Pads

The ports through which GStreamer elements communicate with each other are called pads `(GstPad)`.
There exists `sink pads`, through which `data enters an element`, and `source pads`, through which `data exits an element`. It follows naturally that source elements only contain source pads, sink elements only contain sink pads, and `filter elements contain both`.

![src-element](img/src-element.png)   ![ilter-element](img/filter-element.png)   ![sink-element](img/sink-element.png)

### demuxer

A demuxer contains one sink pad, through which the muxed data arrives, and multiple source pads, one for each stream found in the container:

![demuxer](img/filter-element-multi.png)

`Example:`

For completeness, here you have a simplified pipeline containing a demuxer and two branches, one for audio and one for video. This is NOT the pipeline that will be built in this example:

![simple-player](img/simple-player.png)

### linking pads

`gst_pad_link()` tries to link two pads. the link must be specified from source to sink, and both pads must be owned by elements residing in the same bin (or pipeline).

```c
/* Attempt the link */
ret = gst_pad_link (new_pad, sink_pad);
```

## Signals

GSignals are a crucial point in GStreamer. They allow you to be notified (by means of a callback) when something interesting has happened. Signals are identified by a name, and each GObject has its own signals.

a data pointer forwards to the callback so we can share information with it.

`The signals that a GstElement generates can be found in its documentation or using the gst-inspect-1.0 tool.`

```c
/* Connect to the pad-added signal */
g_signal_connect (data.source, "pad-added", G_CALLBACK (pad_added_handler), &data);
```

## GStreamer States

| State   | Description                                                                                                                 |
|---------|-----------------------------------------------------------------------------------------------------------------------------|
| NULL    | the NULL state or initial state of an element.                                                                              |
| READY   | the element is ready to go to PAUSED.                                                                                       |
| PAUSED  | the element is PAUSED, it is ready to accept and process data. Sink elements however only accept one buffer and then block. |
| PLAYING | the element is PLAYING, the clock is running and the data is flowing.                                                       |

## GstQuery

Queries can be performed on `pads` (gst_pad_query) and `elements` (gst_element_query). Please note that `some queries might need a running pipeline to work`.

Queries can be created using the `gst_query_new_xxx()` functions. Query values can be set using `gst_query_set_xxx()`, and parsed using `gst_query_parse_xxx()` helpers.

The following example shows how to query the duration of a pipeline:

```c
GstQuery *query;
gboolean res;
query = gst_query_new_duration (GST_FORMAT_TIME);
res = gst_element_query (pipeline, query);
if (res) {
  gint64 duration;
  gst_query_parse_duration (query, NULL, &amp;duration);
  g_print ("duration = %"GST_TIME_FORMAT, GST_TIME_ARGS (duration));
} else {
  g_print ("duration query failed...");
}
gst_query_unref (query);
```

## Installing gstreamer-video-1.0

For isntalling `gstreamer-video-1.0` we need to install `libgstreamer-plugins-base1.0-dev`.
