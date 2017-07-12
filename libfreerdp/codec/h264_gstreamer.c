/**
 * Datto
 */
#include <winpr/wlog.h>
#include <freerdp/log.h>
#include <freerdp/codec/h264.h>

#define TAG FREERDP_TAG("codec")

struct _H264_CONTEXT_GSTREAMER
{
};
typedef struct _H264_CONTEXT_GSTREAMER H264_CONTEXT_GSTREAMER;

static int gstreamer_decompress(H264_CONTEXT* h264, const BYTE* pSrcData, UINT32 SrcSize)
{
	H264_CONTEXT_GSTREAMER* sys = (H264_CONTEXT_GSTREAMER*) h264->pSystemData;
	WLog_INFO(TAG, "Gstreammmaahhh boiiii!!!!");
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
