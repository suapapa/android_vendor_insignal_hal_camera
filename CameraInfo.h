#ifndef __ANDROID_HARDWARE_LIBCAMERA_CAMERA_INFO_H__
#define __ANDROID_HARDWARE_LIBCAMERA_CAMERA_INFO_H__

#include <hardware/camera.h>

namespace android {

const char* camera_info_get_default_camera_param_str(int camera_id);

int camera_info_get_number_of_cameras(void);
int camera_info_get_camera_info(int camera_id, struct camera_info* info);

};

#endif
