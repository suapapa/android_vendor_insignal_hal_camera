/*
 * Copyright (C) 2012 Homin Lee <suapapa@insignal.co.kr>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "CameraModule"
#include <utils/Log.h>

#include <stdlib.h>
#include <hardware/camera.h>

#include "CameraFactory.h"
#include "CameraDevice.h"

namespace android {

static int camera_module_open(const struct hw_module_t* module,
                              const char* id,
                              struct hw_device_t** device)
{
    int camera_id = atoi(id);
    LOGI("Opening camera device...");
    camera_device_t *camera_device = cam_dev_get_instance(camera_id);
    if (camera_device == NULL) {
        LOGE("%s: failed to get camera_device for id, %d",
              __func__, camera_id);
        return -1;
    }

    LOGI("Connecting module with device...");
    camera_device->common.module = const_cast<hw_module_t *>(module);

    *device = (hw_device_t*)camera_device;

    LOGI("Camera module opened");
    return 0;
}

static hw_module_methods_t camera_module_methods = {
    open : camera_module_open,
};

extern "C" {
    struct camera_module HAL_MODULE_INFO_SYM = {
        common : {
            tag           : HARDWARE_MODULE_TAG,
            version_major : 1,
            version_minor : 0,
            id            : CAMERA_HARDWARE_MODULE_ID,
            name          : "Insignal Camera HAL",
            author        : "Homin Lee <suapapa@insignal.co.kr>",
            methods       : &camera_module_methods,
            dso           : NULL,
            reserved      : {0},
        },
        get_number_of_cameras : camera_info_get_number_of_cameras,
        get_camera_info       : camera_info_get_camera_info,
    };
}

};
