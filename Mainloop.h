#ifndef MAINLOOP_H
#define MAINLOOP_H

#include <gst/gst.h>
#include "Pipeline.h"

class Mainloop
{
    struct CallbackData
    {
        GstElement *pipeline;
        GMainLoop *loop;
    } data;

    GstBus *bus;

    static void cb_message(GstBus *bus, GstMessage *msg, CallbackData *data);

public:
    Mainloop(Pipeline &pipeline);
    ~Mainloop();

    Mainloop(const Mainloop &) = delete;
    Mainloop &operator=(const Mainloop &) = delete;
    Mainloop(Mainloop &&) = delete;

    void run();
};

#endif
