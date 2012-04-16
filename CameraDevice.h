#ifndef __ANDROID_HARDWARE_LIBCAMERA_CAMERA_DEVICE_H__
#define __ANDROID_HARDWARE_LIBCAMERA_CAMERA_DEVICE_H__

#include <hardware/camera.h>

namespace android {

camera_device_t* cam_dev_get_instance(int id);

};
#endif
