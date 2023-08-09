#include <gst/gst.h>
#include "Mainloop.h"
#include "Pipeline.h"

int main(int argc, char *argv[])
{
    // Initialize GStreamer
    gst_init(&argc, &argv);

    Pipeline pipeline{"pipe"};

    GstElement *source = pipeline.add("udpsrc", "source");
    GstElement *capfilter = pipeline.add("capsfilter", "rtp_capsfilter");

    if (!(source && capfilter && pipeline.add("rtph264depay", "payloader") &&
          pipeline.add("h264parse", "parser") &&
          pipeline.add("avdec_h264", "decoder") &&
          pipeline.add("videoconvert", "video_convert") &&
          pipeline.add("autovideosink", "video_sink")))
    {
        g_printerr("Not all elements could be created.\n");
        return -1;
    }

    // Set caps
    g_object_set(source, "uri", "udp://localhost:5004", NULL);
    GstCaps *caps = gst_caps_new_simple("application/x-rtp",
                                        "media", G_TYPE_STRING, "video",
                                        "clock-rate", G_TYPE_INT, 90000,
                                        "payload", G_TYPE_INT, 96, // Example payload type, change accordingly
                                        NULL);
    g_object_set(G_OBJECT(capfilter), "caps", caps, NULL);
    gst_caps_unref(caps);

    // Link
    if (!pipeline.link())
    {
        g_printerr("Elements could not be linked.\n");
        return -1;
    }

    // Play
    if (pipeline.play() == GST_STATE_CHANGE_FAILURE)
    {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        return -1;
    }

    Mainloop mainloop{pipeline};
    mainloop.run();

    return 0;
}
