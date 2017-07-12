/**
 * Datto
 */
#include <winpr/wlog.h>
#include <freerdp/log.h>
#include <freerdp/codec/h264.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

#define TAG FREERDP_TAG("codec")

struct _H264_CONTEXT_GSTREAMER
{
	GstPipeline *pipeline;
	GstAppSrc *appSrc;
};
typedef struct _H264_CONTEXT_GSTREAMER H264_CONTEXT_GSTREAMER;

static int gstreamer_decompress(H264_CONTEXT* h264, const BYTE* pSrcData, UINT32 SrcSize)
{
	H264_CONTEXT_GSTREAMER* sys = (H264_CONTEXT_GSTREAMER*) h264->pSystemData;
	WLog_INFO(TAG, "Gstreamer decompress: SrcSize %d", SrcSize);

	guint8 *ptr = (guint8*)g_malloc(SrcSize);
	memcpy(ptr, pSrcData, SrcSize);

	GstBuffer *buffer = gst_buffer_new_wrapped(ptr, SrcSize);
	gst_app_src_push_buffer(sys->appSrc, buffer);

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

	gst_init(NULL, NULL);
	GError* error;
	sys->pipeline = (GstPipeline *) gst_parse_launch("appsrc name=stream_src ! decodebin ! autovideosink", &error);
	sys->appSrc = (GstAppSrc *) gst_bin_get_by_name((GstBin *) sys->pipeline, "stream_src");
	if (!sys->appSrc)
	{
		WLog_ERR(TAG, "Failed to get appsrc");
		goto EXCEPTION;
	}

	gst_element_set_state((GstElement *) sys->pipeline, GST_STATE_PAUSED);
	gst_element_set_state((GstElement *) sys->pipeline, GST_STATE_PLAYING);

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
