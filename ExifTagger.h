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

#ifndef __ANDROID_HARDWARE_LIBCAMERA_EXIF_TAGGER_H__
#define __ANDROID_HARDWARE_LIBCAMERA_EXIF_TAGGER_H__

#include <utils/Errors.h>
extern "C" {
#include "jhead.h"
}

#include "TaggerInterface.h"

namespace android {

#define MAX_EXIF_TAGS_SUPPORTED 30

class ExifTagger : public TaggerInterface {
public:
    ExifTagger();
    ~ExifTagger();

    virtual int tagToJpeg(TaggerParams* p,
                          const uint8_t* srcBuff,
                          unsigned int srcSize);
    virtual int writeTaggedJpeg(uint8_t* destBuff, int destSize);

private:
    ExifElement_t _table[MAX_EXIF_TAGS_SUPPORTED];
    unsigned int _gpsTagCount;
    unsigned int _exifTagCount;
    bool _jpegOpened;

    status_t _fillExifTable(TaggerParams* p);

    status_t _insertTag(const char* tag, const char* value);
    status_t _insertTag(const char* tag, unsigned int value);

    status_t _insertDatetimeTag(void);
    status_t _insertOrientationTag(unsigned int degree);
    status_t _insertGpsTag(_gpsData* gps);

    status_t _convertGPSCoord(double coord,
                              int& deg,
                              int& min,
                              int& sec,
                              int& secDivisor);
};

}
#endif

