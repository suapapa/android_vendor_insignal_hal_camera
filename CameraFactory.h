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

#ifndef __ANDROID_HARDWARE_LIBCAMERA_CAMERA_FACTORY_H__
#define __ANDROID_HARDWARE_LIBCAMERA_CAMERA_FACTORY_H__

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
