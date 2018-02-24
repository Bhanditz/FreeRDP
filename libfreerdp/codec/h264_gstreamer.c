/**
 * Datto
 */
#include <winpr/wlog.h>
#include <freerdp/log.h>
#include <freerdp/codec/h264.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>

#define TAG FREERDP_TAG("codec")

struct _H264_CONTEXT_GSTREAMER
{
	GstPipeline *pipeline;
	GstAppSrc *appSrc;
	GstAppSink *appSink;
	BOOL ready;
	BOOL isPreroll;
	BYTE *yBuf;
	BYTE *uBuf;
	BYTE *vBuf;
};
typedef struct _H264_CONTEXT_GSTREAMER H264_CONTEXT_GSTREAMER;

static int gstreamer_decompress(H264_CONTEXT* h264, const BYTE* pSrcData, UINT32 SrcSize)
{
	H264_CONTEXT_GSTREAMER* sys = (H264_CONTEXT_GSTREAMER*) h264->pSystemData;
	WLog_INFO(TAG, "Gstreamer decompress: SrcSize %d", SrcSize);

	guint8 *ptr = (guint8*)g_malloc(SrcSize);
	memcpy(ptr, pSrcData, SrcSize);

	GstBuffer *buffer = gst_buffer_new_wrapped(ptr, SrcSize);
	GstFlowReturn flowReturn = gst_app_src_push_buffer(GST_APP_SRC(sys->appSrc), buffer);
	if (flowReturn != GST_FLOW_OK)
	{
		WLog_ERR(TAG, "flow returned non-ok");
	}

	if (sys->ready)
	{
		if (sys->isPreroll)
		{
			GstSample *preroll = gst_app_sink_pull_sample(sys->appSink);
			if (preroll == NULL)
			{
				WLog_ERR(TAG, "Failed to pull sample");
				exit(1);
			}
			gst_sample_unref(preroll);
			sys->isPreroll = FALSE;
			WLog_INFO(TAG, "pulled preroll frame");
		}

		GstSample *sample = gst_app_sink_pull_sample(sys->appSink);
		if (sample == NULL)
		{
			WLog_ERR(TAG, "Failed to pull sample");
			exit(1);
		}
		else
		{
			WLog_INFO(TAG, "Successfully pulled sample");
			GstBuffer *outbuf = gst_sample_get_buffer(sample);
			if (outbuf == NULL)
			{
				WLog_ERR(TAG, "failed to get buffer from sample");
				exit(1);
			}

			GstCaps *caps = gst_sample_get_caps(sample);
			if (caps == NULL)
			{
				WLog_ERR(TAG, "failed to get caps from sample");
				exit(1);
			}

			GstVideoInfo videoInfo;
			if (!gst_video_info_from_caps(&videoInfo, caps))
			{
				WLog_ERR(TAG, "failed to get video info from caps");
				exit(1);
			}


			GstVideoFrame videoFrame;
			if (!gst_video_frame_map(&videoFrame, &videoInfo, outbuf, GST_MAP_READ))
			{
				WLog_ERR(TAG, "failed to map video frame");
				exit(1);
			}

			BYTE** pYUVData = h264->pYUVData;
			UINT32* iStride = h264->iStride;

			pYUVData[0] = GST_VIDEO_FRAME_PLANE_DATA(&videoFrame, 0);
			pYUVData[1] = GST_VIDEO_FRAME_PLANE_DATA(&videoFrame, 1);
			pYUVData[2] = GST_VIDEO_FRAME_PLANE_DATA(&videoFrame, 2);
			iStride[0] = (UINT32) GST_VIDEO_FRAME_PLANE_STRIDE(&videoFrame, 0);
			iStride[1] = (UINT32) GST_VIDEO_FRAME_PLANE_STRIDE(&videoFrame, 1);
			iStride[2] = (UINT32) GST_VIDEO_FRAME_PLANE_STRIDE(&videoFrame, 2);
			h264->width = (UINT32) GST_VIDEO_FRAME_WIDTH(&videoFrame);
			h264->height = (UINT32) GST_VIDEO_FRAME_HEIGHT(&videoFrame);

			WLog_INFO(TAG, "stride 0: %d, stride 1: %d, stride 2: %d", iStride[0], iStride[1], iStride[2]);
			WLog_INFO(TAG, "frame width: %d", h264->width);
			WLog_INFO(TAG, "frame height: %d", h264->height);

			gst_video_frame_unmap(&videoFrame);
			gst_sample_unref(sample);
			return 1;
		}
	}

	return -1;
}

static int gstreamer_compress(H264_CONTEXT* h264, BYTE** ppDstData, UINT32* pDstSize)
{
	WLog_INFO(TAG, "Gtreamer compress");
	return -1;
}

static void gstreamer_uninit(H264_CONTEXT* h264)
{
	H264_CONTEXT_GSTREAMER* sys = (H264_CONTEXT_GSTREAMER*) h264->pSystemData;
	WLog_INFO(TAG, "Gtreamer uninit");

	if (!sys)
	{
		return;
	}

	free(sys);
	h264->pSystemData = NULL;
}

static GstFlowReturn preroll_ready(GstAppSink *appSink, gpointer userData)
{
	WLog_ERR(TAG, "PREROLL READY");
	H264_CONTEXT_GSTREAMER* sys = (H264_CONTEXT_GSTREAMER*) userData;
	sys->ready = TRUE;
	sys->isPreroll = TRUE;
	return GST_FLOW_OK;
}

static BOOL gstreamer_init(H264_CONTEXT* h264)
{
	H264_CONTEXT_GSTREAMER* sys;
	sys = (H264_CONTEXT_GSTREAMER*) calloc(1, sizeof(H264_CONTEXT_GSTREAMER));

	WLog_INFO(TAG, "Gtreamer init");
	if (!sys)
	{
		goto EXCEPTION;
	}
	h264->pSystemData = (void*) sys;

	GError *error;
	gst_init(NULL, NULL);

	sys->pipeline = (GstPipeline *) gst_parse_launch("appsrc name=src ! queue max-size-buffers=1 ! decodebin ! queue max-size-buffers=1 ! ximagesink", &error);
	sys->appSrc = (GstAppSrc *) gst_bin_get_by_name(GST_BIN(sys->pipeline), "src");
	sys->appSink = (GstAppSink *) gst_bin_get_by_name(GST_BIN(sys->pipeline), "sink");

	if (!sys->appSrc)
	{
		WLog_ERR(TAG, "Failed to get appsrc");
		goto EXCEPTION;
	}
	if (!sys->appSink)
	{
		WLog_ERR(TAG, "Failed to get appsink");
//		goto EXCEPTION;
	}

	gst_app_src_set_latency(sys->appSrc, 0, 0);


//	GstAppSinkCallbacks *appSinkCallbacks = calloc(1, sizeof(GstAppSinkCallbacks));
//	appSinkCallbacks->eos = NULL;
//	appSinkCallbacks->new_preroll = &preroll_ready;
//	appSinkCallbacks->new_sample = NULL;
//
//	gst_app_sink_set_callbacks(GST_APP_SINK(sys->appSink), appSinkCallbacks, sys, NULL);
//	gst_app_sink_set_max_buffers(GST_APP_SINK(sys->appSink), 0);
//	gst_app_sink_set_drop(GST_APP_SINK(sys->appSink), TRUE);

	gst_element_set_state(GST_ELEMENT(sys->pipeline), GST_STATE_PLAYING);

	sys->ready = FALSE;
	sys->isPreroll = FALSE;

	sys->yBuf = calloc(1, 1920 * 1080);
	sys->uBuf = calloc(1, 1920 * 1080);
	sys->vBuf = calloc(1, 1920 * 1080);

	return TRUE;
EXCEPTION:
	return FALSE;
}

H264_CONTEXT_SUBSYSTEM g_Subsystem_gstreamer =
{
	"gstreamer",
	gstreamer_init,
	gstreamer_uninit,
	gstreamer_decompress,
	gstreamer_compress
};
