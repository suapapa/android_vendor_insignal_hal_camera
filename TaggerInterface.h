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

#ifndef __ANDROID_HARDWARE_LIBCAMERA_TAGGER_IMTERFACE_H__
#define __ANDROID_HARDWARE_LIBCAMERA_TAGGER_IMTERFACE_H__

namespace android {

// FYI. refer exif-entry.c

struct _gpsData {
    double latitude, longitude, altitude;
    long timestamp;
    char* procMethod;
};

struct TaggerParams {
    char* maker;
    char* model;
    unsigned int width, height;
    int focalNum, focalDen;

    int rotation;

#define EXIF_CENTER     0x0
#define EXIF_AVERAGE    0x1
    int metering;

#define EXIF_ISO_AUTO   0x0
#define EXIF_ISO_100    0x1
#define EXIF_ISO_200    0x2
#define EXIF_ISO_400    0x3
#define EXIF_ISO_800    0x4
#define EXIF_ISO_1000   0x5
#define EXIF_ISO_1200   0x6
#define EXIF_ISO_1600   0x7
    int iso;

#define EXIF_WB_AUTO    0xA
#define EXIF_WB_MANUAL  0xB
    int wb;
    int exposure;

#define EXIF_FLASH_NOT_FIRED            0x00
#define EXIF_FLASH_FIRED                0x01
#define EXIF_FLASH_AUTO_NOT_FIRED       0x18
#define EXIF_FLASH_AUTO_FIRED           0x19
    int flash;

    float zoom;
    uint8_t* thumbData;
    unsigned int thumbSize;
    _gpsData gps;
};

class TaggerInterface {
public:
    virtual ~TaggerInterface() { }
    virtual int tagToJpeg(TaggerParams* p,
                          const uint8_t* srcBuff,
                          unsigned int srcSize) = 0;
    virtual int writeTaggedJpeg(uint8_t* destBuff, int destSize) = 0;
};

}
#endif
