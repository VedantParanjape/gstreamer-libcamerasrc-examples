#include <gst/gst.h>

int main(int argc, char *argv[])
{
    GstElement *pipeline, *libcamera_src, *queue0, *queue1, *convert0, *convert1, *sink0, *sink1;
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;

    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Create the elements */
    libcamera_src = gst_element_factory_make("libcamerasrc", "libcamera");
    queue0 = gst_element_factory_make("queue", "camera_queue0");
    queue1 = gst_element_factory_make("queue", "camera_queue1");

    convert0 = gst_element_factory_make("videoconvert", "convert0");
    convert1 = gst_element_factory_make("videoconvert", "convert1");

    sink0 = gst_element_factory_make("autovideosink", "sink0");
    sink1 = gst_element_factory_make("autovideosink", "sink1");

    /* Create the empty pipeline */
    pipeline = gst_pipeline_new("test-pipeline");

    if (!pipeline || !queue0 || !queue1 || !convert0 || !convert1 || !sink0 || !sink1 || !libcamera_src)
    {
        g_printerr("Not all elements could be created.\n");
        return -1;
    }

    /* Build the pipeline */
    gst_bin_add_many(GST_BIN(pipeline), libcamera_src, queue0, queue1, convert0, convert1, sink0, sink1, NULL);
    if (gst_element_link_many(queue0, convert0, sink0, NULL) != TRUE || gst_element_link_many(queue1, convert1, sink1, NULL) != TRUE)
    {
        g_printerr("Elements could not be linked (1).\n");
        gst_object_unref(pipeline);
        return -1;
    }

    GstPad *src_pad = gst_element_get_static_pad(libcamera_src, "src");
    GstPad *request_pad = gst_element_get_request_pad(libcamera_src, "src_%u");
    GstPad *queue0_sink_pad = gst_element_get_static_pad(queue0, "sink");
    GstPad *queue1_sink_pad = gst_element_get_static_pad(queue1, "sink");
    g_object_set(src_pad, "stream-role", 3, NULL);
    g_object_set(request_pad, "stream-role", 3, NULL);

    if (gst_pad_link(src_pad, queue0_sink_pad) != GST_PAD_LINK_OK || gst_pad_link(request_pad, queue1_sink_pad) != GST_PAD_LINK_OK)
    {
        g_printerr("Tee could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }
    gst_object_unref(queue0_sink_pad);
    gst_object_unref(queue1_sink_pad);

    /* Start playing */
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    /* Wait until error or EOS */
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                                   GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    /* Parse message */
    if (msg != NULL)
    {
        GError *err;
        gchar *debug_info;

        switch (GST_MESSAGE_TYPE(msg))
        {
        case GST_MESSAGE_ERROR:
            gst_message_parse_error(msg, &err, &debug_info);
            g_printerr("Error received from element %s: %s\n",
                       GST_OBJECT_NAME(msg->src), err->message);
            g_printerr("Debugging information: %s\n",
                       debug_info ? debug_info : "none");
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

    /* Free resources */
    gst_element_release_request_pad(libcamera_src, request_pad);
    gst_object_unref(request_pad);
    gst_object_unref(src_pad);

    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return 0;
}
