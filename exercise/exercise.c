#include <gst/gst.h>
#include <stdio.h>

typedef struct _CustomData {
	GstElement *pipeline;
	GstElement *source;
	GstElement *demuxer;
	GstElement *audioqueue;
	GstElement *videoqueue;
	GstElement *audio_decoder;
	GstElement *video_decoder;
	GstElement *video_convert;
	GstElement *audio_convert;
	GstElement *video_sink;
	GstElement *audio_sink;
} CustomData;


static void pad_added_handler (GstElement *src, GstPad *pad, CustomData *data);
static void pad_added_handler_src (GstElement *src, GstPad *pad, CustomData *data);
int main(int argc, char *argv[]) {
	CustomData data;
	GstBus *bus;
	GstMessage *msg;
	GstPad *pad;
	GstStateChangeReturn ret;
	gboolean terminate = FALSE;
	if (argc < 2) {
		printf("Invalid Command \nUsage-> ./exe uri for mp4 file\n");
		exit(1);
	}
	/* Initialize GStreamer */
	gst_init (&argc, &argv);
	/* Create the elements */
	data.source = gst_element_factory_make ("urisourcebin", "source");
	data.demuxer = gst_element_factory_make ("qtdemux", "demuxer");
	data.audioqueue = gst_element_factory_make("queue","audioqueue");
	data.videoqueue = gst_element_factory_make("queue","videoqueue");
	data.audio_decoder = gst_element_factory_make ("avdec_aac", "audio_decoder");
	data.audio_convert = gst_element_factory_make ("audioconvert", "audio_convert");
	data.audio_sink = gst_element_factory_make ("autoaudiosink", "audio_sink");
	data.video_decoder = gst_element_factory_make("avdec_h264","video_decoder");
	data.video_convert = gst_element_factory_make("autovideoconvert","video_convert");
	data.video_sink = gst_element_factory_make("autovideosink","video_sink");

	data.pipeline = gst_pipeline_new ("test-pipeline");
	if (!data.pipeline || !data.source || !data.demuxer || !data.audioqueue ||!data.audio_decoder ||!data.audio_convert ||
	!data.audio_sink || !data.videoqueue || !data.video_decoder || !data.video_convert || !data.video_sink) {
		g_printerr ("Not all elements could be created.\n");
		return -1;
	}

	gst_bin_add_many (GST_BIN (data.pipeline), data.source,data.demuxer,data.audioqueue,data.audio_decoder,data.audio_convert,data.audio_sink,data.videoqueue,data.video_decoder,data.video_convert,data.video_sink, NULL);

	if (!gst_element_link_many (data.audioqueue,data.audio_decoder,data.audio_convert, data.audio_sink,NULL)) {
		g_printerr (" audio Elements could not be linked.\n");
		gst_object_unref (data.pipeline);
		return -1;
	}
	if (!gst_element_link_many(data.videoqueue,data.video_decoder,data.video_convert, data.video_sink,NULL)) {
		g_printerr("video Elements could not be linked.\n");
		gst_object_unref(data.pipeline);
		return -1;
	} 
	/* Set the file to play */
	g_object_set (data.source, "uri", argv[1], NULL);
		
	g_signal_connect (data.source, "pad-added", G_CALLBACK (pad_added_handler_src), &data);   
	g_signal_connect (data.demuxer, "pad-added", G_CALLBACK (pad_added_handler), &data);
	/* Start playing */
	ret = gst_element_set_state (data.pipeline, GST_STATE_PLAYING);
	if (ret == GST_STATE_CHANGE_FAILURE) {
		g_printerr ("Unable to set the pipeline to the playing state.\n");
		gst_object_unref (data.pipeline);
		return -1;
	}
	/* Listen to the bus */
	bus = gst_element_get_bus (data.pipeline);
	do {
		msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
		GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

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
				terminate = TRUE;
				break;
			case GST_MESSAGE_EOS:
				g_print ("End-Of-Stream reached.\n");
				terminate = TRUE;
				break;
			case GST_MESSAGE_STATE_CHANGED:

				if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data.pipeline)) {
					GstState old_state, new_state, pending_state;
					gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
					g_print ("Pipeline state changed from %s to %s:\n",
					gst_element_state_get_name (old_state), gst_element_state_get_name (new_state));
				}
				break;
			default:

				g_printerr ("Unexpected message received.\n");
				break;
			}
			gst_message_unref (msg);
		}
	} while (!terminate); 

	gst_object_unref (bus);
	gst_element_set_state (data.pipeline, GST_STATE_NULL);
	gst_object_unref (data.pipeline);
	return 0;
	}
static void pad_added_handler_src (GstElement *src, GstPad *pad, CustomData *data) {
	GstCaps *new_pad_caps = NULL;
	GstStructure *new_pad_struct = NULL;
	const gchar *new_pad_type = NULL;
	new_pad_caps = gst_pad_get_current_caps (pad);
	g_print("Inside the pad_added_handler_src method \n");
	g_print ("Received new pad '%s' from '%s':\n", GST_PAD_NAME (pad), GST_ELEMENT_NAME (src));
	GstPad *sink_pad_demux = gst_element_get_static_pad (data->demuxer, "sink");
	gst_pad_link (pad, sink_pad_demux);
}
/* This function will be called by the pad-added signal */
static void pad_added_handler (GstElement *src, GstPad *new_pad, CustomData *data) {
	g_print("Inside the pad_added_handler method \n");
	GstPad *sink_pad_audio = gst_element_get_static_pad (data->audioqueue, "sink");
	GstPad *sink_pad_video = gst_element_get_static_pad (data->videoqueue, "sink");
	GstPadLinkReturn ret;
	GstCaps *new_pad_caps = NULL;
	GstStructure *new_pad_struct = NULL;
	const gchar *new_pad_type = NULL;
	g_print ("Received new pad '%s' from '%s':\n", GST_PAD_NAME (new_pad), GST_ELEMENT_NAME (src));
	new_pad_caps = gst_pad_get_current_caps (new_pad);
	new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
	new_pad_type = gst_structure_get_name (new_pad_struct);
	if (g_str_has_prefix (new_pad_type,"audio/mpeg")) 
	{
		  ret = gst_pad_link (new_pad, sink_pad_audio);
		  if (GST_PAD_LINK_FAILED (ret)) 
		   { 
			g_print (" Type is '%s' but link failed.\n", new_pad_type);
		   } 
		   else 
		  {
			g_print (" Link succeeded (type '%s').\n", new_pad_type);
		  }
	}
	else if (g_str_has_prefix (new_pad_type, "video/x-h264")) 
	{
		ret = gst_pad_link (new_pad, sink_pad_video);

		if (GST_PAD_LINK_FAILED (ret)) 
		{
				g_print (" Type is '%s' but link failed.\n", new_pad_type);
		} 
		else 
		{
				g_print (" Link succeeded (type '%s').\n", new_pad_type);
		}
	} 


	else {
		g_print (" It has type '%s' which is not raw audio. Ignoring.\n", new_pad_type);
		goto exit;
	}
	exit:
		if (new_pad_caps != NULL)
			gst_caps_unref (new_pad_caps);
			gst_object_unref (sink_pad_audio);
			gst_object_unref (sink_pad_video);
}
