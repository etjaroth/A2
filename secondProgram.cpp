#include <gst/gst.h>

typedef struct _PipelineData
{
    gboolean is_live;

    GstElement *pipeline;
    GMainLoop *loop;

    GstElement *source;
    GstElement *capsfilter;

    GstElement *payloader;
    GstElement *parser;
    GstElement *decoder;

    GstElement *video_convert;
    GstElement *video_sink;
} PipelineData;

void cb_message(GstBus *bus, GstMessage *msg, PipelineData *data)
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

        gst_element_set_state(data->pipeline, GST_STATE_READY);
        g_main_loop_quit(data->loop);
        break;
    case GST_MESSAGE_EOS:
        g_print("End-Of-Stream reached.\n");
        gst_element_set_state(data->pipeline, GST_STATE_READY);
        g_main_loop_quit(data->loop);
        break;
    case GST_MESSAGE_BUFFERING:
    {
        gint percent = 0;

        // If the stream is live, we do not care about buffering
        if (data->is_live)
        {
            g_print("Didn't buffer.\n");
            break;
        }

        gst_message_parse_buffering(msg, &percent);
        g_print("Buffering (%3d%%)\r", percent);
        // Wait until buffering is complete before start/resume playing
        if (percent < 100)
            gst_element_set_state(data->pipeline, GST_STATE_PAUSED);
        else
            gst_element_set_state(data->pipeline, GST_STATE_PLAYING);
        break;
    }
    case GST_MESSAGE_CLOCK_LOST:
        // Make new clock
        gst_element_set_state(data->pipeline, GST_STATE_PAUSED);
        gst_element_set_state(data->pipeline, GST_STATE_PLAYING);
        break;
    case GST_MESSAGE_STATE_CHANGED:
        // We are only interested in state-changed messages from the pipeline
        if (GST_MESSAGE_SRC(msg) == GST_OBJECT(data->pipeline))
        {
            GstState old_state, new_state, pending_state;
            gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
            g_print("Pipeline state changed from %s to %s:\n",
                    gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));
        }
        break;
    default:
        break;
    }
}

int main(int argc, char *argv[])
{
    // Initialize GStreamer
    gst_init(&argc, &argv);
    PipelineData data;
    data.is_live = false;
    GstStateChangeReturn ret;

    // Create the elements
    data.source = gst_element_factory_make("udpsrc", "source");
    data.capsfilter = gst_element_factory_make("capsfilter", "rtp_capsfilter");
    data.payloader = gst_element_factory_make("rtph264depay", "payloader");
    data.parser = gst_element_factory_make("h264parse", "parser");
    data.decoder = gst_element_factory_make("avdec_h264", "decoder");
    data.video_convert = gst_element_factory_make("videoconvert", "video_convert");
    data.video_sink = gst_element_factory_make("autovideosink", "video_sink");

    // Create the pipeline
    data.pipeline = gst_pipeline_new("data-pipeline");
    if (!data.pipeline ||
        !data.source ||
        !data.capsfilter ||
        !data.payloader ||
        !data.parser ||
        !data.decoder ||
        !data.video_convert ||
        !data.video_sink)
    {
        g_printerr("Not all elements could be created.\n");
        g_printerr("%d%d%d%d%d%d%d%d\n",
                   (int)(0 == data.pipeline),
                   (int)(0 == data.source),
                   (int)(0 == data.capsfilter),
                   (int)(0 == data.payloader),
                   (int)(0 == data.source),
                   (int)(0 == data.decoder),
                   (int)(0 == data.video_convert),
                   (int)(0 == data.video_sink));
        return -1;
    }

    GstCaps *caps = gst_caps_new_simple("application/x-rtp",
                                        "media", G_TYPE_STRING, "video",
                                        "clock-rate", G_TYPE_INT, 90000,
                                        "payload", G_TYPE_INT, 96, // Example payload type, change accordingly
                                        NULL);
    g_object_set(G_OBJECT(data.capsfilter), "caps", caps, NULL);
    gst_caps_unref(caps);

    // Add to the pipeline
    gst_bin_add_many(GST_BIN(data.pipeline), // Source
                     data.source,
                     data.capsfilter,
                     data.payloader,
                     data.parser,
                     data.decoder,
                     data.video_convert,
                     data.video_sink,
                     NULL);

    if (!gst_element_link_many(data.source, data.capsfilter, data.payloader, data.parser, data.decoder, data.video_convert, data.video_sink, NULL))
    {
        g_printerr("UDP elements could not be linked.\n");
        gst_object_unref(data.pipeline);
        return -1;
    }

    // Set the udpsrc to play
    g_object_set(data.source, "uri", "udp://localhost:5004", NULL);

    // Start playing pipeline
    ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(data.pipeline);
        return -1;
    }
    else if (ret == GST_STATE_CHANGE_NO_PREROLL)
    {
        data.is_live = TRUE;
    }

    // Set up main loop
    GstBus *bus = gst_element_get_bus(data.pipeline);
    data.loop = g_main_loop_new(NULL, FALSE);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(bus, "message", G_CALLBACK(cb_message), &data);
    g_main_loop_run(data.loop);

    // Release resources
    g_main_loop_unref(data.loop);
    gst_object_unref(bus);
    gst_element_set_state(data.pipeline, GST_STATE_NULL);
    gst_object_unref(data.pipeline);
    return 0;
}
