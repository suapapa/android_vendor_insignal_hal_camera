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

struct gps_data {
    bool enable;
    int longDeg, longMin, longSec;
    int latDeg, latMin, latSec;
    int altitude, altitudeRef;
    char* longRef, *latRef;
    char* mapdatum, *versionId;
    char* procMethod;
    unsigned long timestamp;
    char datestamp[11];
};

// FYI. refer exif-entry.c
struct TaggerParams {
    int width, height;
    int rotation;
#define EXIF_CENTER     0x0
#define EXIF_AVERAGE    0x1
    int metering_mode;
#define EXIF_ISO_AUTO   0x0
#define EXIF_ISO_100    0x1
#define EXIF_ISO_200    0x2
#define EXIF_ISO_400    0x3
#define EXIF_ISO_800    0x4
#define EXIF_ISO_1000   0x5
#define EXIF_ISO_1200   0x6
#define EXIF_ISO_1600   0x7
    int iso;
    float zoom;
#define EXIF_WB_AUTO    0xA
#define EXIF_WB_MANUAL  0xB
    int wb;
    int exposure;
#define EXIF_FLASH_NOT_FIRED            0x00
#define EXIF_FLASH_FIRED                0x01
#define EXIF_FLASH_AUTO_NOT_FIRED       0x18
#define EXIF_FLASH_AUTO_FIRED           0x19
    int flash;
    char* maker;
    char* model;
    uint8_t* thumb_data;
    unsigned int thumb_size;
    gps_data gps_location;
};

class TaggerInterface {
public:
    virtual ~TaggerInterface() { }
    virtual int createTaggedJpeg(TaggerParams* p,
                                 const uint8_t* jpegData,
                                 unsigned int jpegSize) = 0;
    virtual int copyJpegWithExif(uint8_t* targetBuff, int targetBuffSize) = 0;
};

}
#endif
