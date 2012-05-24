//#define LOG_NDEBUG 0
#define LOG_TAG "CameraInfo"
#include <utils/Log.h>

#include "CameraInfo.h"

#ifdef SAMSUNG_S5P_JPEG_ENCODER
#include "S5PJpegEncoder.h"
#endif
//#include "ExifTagger.h"

namespace android {

const char* camera_info_get_default_camera_param_str(int camera_id)
{
    static const char* back_cam_init_params_str =
        "preview-size=720x480;"
        "preview-size-values=1280x720,720x480,720x480,640x480,320x240,176x144;"
        "preview-format=yuv420sp;"
        "preview-format-values=yuv420sp,yuv420p;"
        "preview-frame-rate=30;"
        "preview-frame-rate-values=7,15,30;"
        "preview-fps-range=15000,30000;"
        "preview-fps-range-values=(15000,30000);"
// TODO: 5MP is too much for raw-snapshot! It needs 10M bytes. dirty fix here not to capture too large
#if 0
        "picture-size=2560x1920;"
        "picture-size-values=3264x2448,3264x1968,"
        "2560x1920,2048x1536,2048x1536,2048x1232,"
        "1600x1200,1600x960,"
        "800x480,640x480;"
#endif
        "picture-size=1600x1200;"
        "picture-size-values="
        "1600x1200,1600x960,"
        "800x480,640x480;"
        "picture-format=jpeg;"
        "picture-format-values=jpeg;"
        "jpeg-thumbnail-width=320;"
        "jpeg-thumbnail-height=240;"
        "jpeg-thumbnail-size-values=320x240,0x0;"
        "jpeg-thumbnail-quality=100;"
        "jpeg-quality=100;"
        "rotation=0;"
        "min-exposure-compensation=-2;"
        "max-exposure-compensation=2;"
        "exposure-compensation-step=1;"
        "exposure-compensation=0;"
        "whitebalance=auto;"
        "whitebalance-values=auto,fluorescent,warm-flurescent,daylight,cloudy-daylight;"
        "effect=none;"
        "effect-values=none,mono,negative,sepia;"
        "antibanding=auto;"
        "antibanding-values=auto,50hz,60hz,off;"
        "scene-mode=auto;"
        "scene-mode-values=auto,portrait,night,landscape,sports,party,snow,sunset,fireworks,candlelight;"
        "focus-mode=auto;"
        "focus-mode-values=auto,macro,infinity;"
        "video-frame-format=yuv420sp;"
        "focal-length=343";

    return back_cam_init_params_str;
}

int camera_info_get_number_of_cameras(void)
{
    return 1;
}

int camera_info_get_camera_info(int camera_id, struct camera_info* info)
{
    info->facing = CAMERA_FACING_BACK;
    info->orientation = 0;

    return 0;
}

EncoderInterface* get_encoder(void)
{
    EncoderInterface* encoder = NULL;

#ifdef SAMSUNG_S5P_JPEG_ENCODER
    LOGI("creating S5PJpegEncoder...");
    encoder = new S5PJpegEncoder();
#endif

    LOGW_IF(encoder == NULL, "No jpeg encoder specified!");

    return encoder;
}

TaggerInterface* get_tagger(void)
{
    LOGW("No tagger exists!");
    TaggerInterface* tagger = NULL; //new ExifTagger();
    return tagger;
}

};
