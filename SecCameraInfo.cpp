/*
 * Copyright 2008, The Android Open Source Project
 * Copyright 2012, Insignal Co. LTD
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

#include <camera/Camera.h>
#include "SecCameraHardware.h"

namespace android
{

static CameraInfo sCameraInfo[] = {
    {
        CAMERA_FACING_BACK,
        0 /* orientation */
    }, {
        CAMERA_FACING_FRONT,
        270,  /* orientation */
    }
};

extern "C" int HAL_getNumberOfCameras()
{
    return sizeof(sCameraInfo) / sizeof(sCameraInfo[0]);
}

extern "C" void HAL_getCameraInfo(int cameraId, struct CameraInfo *cameraInfo)
{
    memcpy(cameraInfo, &sCameraInfo[cameraId], sizeof(CameraInfo));
}

extern "C" sp<CameraHardwareInterface> HAL_openCameraHardware(int cameraId)
{
    return SecCameraHardware::createInstance(cameraId);
}

}; // namespace android
