#ifndef __ANDROID_HARDWARE_LIBCAMERA_CAMERA_INFO_H__
#define __ANDROID_HARDWARE_LIBCAMERA_CAMERA_INFO_H__

#include <string.h>
#include <hardware/camera.h>

#include "EncoderInterface.h"
#include "TaggerInterface.h"

namespace android {

int camera_info_get_number_of_cameras(void);
int camera_info_get_camera_info(int camera_id, struct camera_info* info);

const char* camera_info_get_default_camera_param_str(int camera_id);

EncoderInterface*	get_encoder(void);
TaggerInterface*	get_tagger(void);

};

#endif
