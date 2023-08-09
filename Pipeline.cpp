#include "Pipeline.h"

Pipeline::Pipeline(std::string pipeline_name) : pipeline_name{pipeline_name}
{
    pipeline = gst_pipeline_new(pipeline_name.c_str());
}

Pipeline::~Pipeline()
{
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline); // Releases all added elements as well
}

// Creates an elemnt and returns it. If elememt creation is successful, adds it to the pipeline.
GstElement *Pipeline::add(std::string element, std::string name)
{
    GstElement *new_element = nullptr;
    name = "";
    if (name == "")
    {
        new_element = gst_element_factory_make(element.c_str(), NULL);
    }
    else
    {
        new_element = gst_element_factory_make(element.c_str(), name.c_str());
    }

    if (new_element)
    {
        elements.push_back(new_element);
        gst_bin_add(GST_BIN(pipeline), new_element);
    }
    else
    {
        g_print("Could not create element!");
    }

    return new_element;
}

bool Pipeline::link()
{
    if (elements.size() < 2)
    {
        return false;
    }

    bool success = true;
    for (std::vector<GstElement *>::iterator itr = elements.begin() + 1; itr != elements.end(); ++itr)
    {
        success &= gst_element_link(*(itr - 1), *itr);
    }
    return success;
}

GstStateChangeReturn Pipeline::play()
{
    return gst_element_set_state(pipeline, GST_STATE_PLAYING);
}

GstElement *Pipeline::get_pipeline()
{
    return pipeline;
}
