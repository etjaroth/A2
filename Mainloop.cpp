#include <gst/gst.h>
#include "Mainloop.h"

void Mainloop::cb_message(GstBus *bus, GstMessage *msg, Mainloop::CallbackData *data)
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

Mainloop::Mainloop(Pipeline &pipeline)
{
    data.pipeline = pipeline.get_pipeline();
    bus = gst_element_get_bus(data.pipeline);
    data.loop = g_main_loop_new(NULL, FALSE);
    gst_bus_add_signal_watch(bus);

    g_signal_connect(bus, "message", G_CALLBACK(Mainloop::cb_message), &data);
}

Mainloop::~Mainloop()
{
    gst_bus_remove_signal_watch(bus);
    g_main_loop_unref(data.loop);
    gst_object_unref(bus);
}

void Mainloop::run()
{
    g_main_loop_run(data.loop);
}
