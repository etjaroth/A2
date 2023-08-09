#ifndef PIPELINE_H
#define PIPELINE_H

#include <gst/gst.h>
#include <string>
#include <vector>

class Pipeline
{
    std::string pipeline_name;
    GstElement *pipeline;

    std::vector<GstElement *> elements;
    bool linked = false;

public:
    Pipeline(std::string pipeline_name);
    ~Pipeline();

    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;

    // Creates an elemnt and returns it. If elememt creation is successful, adds it to the pipeline.
    GstElement *add(std::string element, std::string name = "");
    bool link();

    GstStateChangeReturn play();
    
    GstElement *get_pipeline();
};

#endif
