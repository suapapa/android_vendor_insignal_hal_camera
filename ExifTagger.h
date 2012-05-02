/*
 * Copyright (C) 2012 Insignal Co, Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __LIBCAMERA_EXIF_TAGGER_H__
#define __LIBCAMERA_EXIF_TAGGER_H__

#include <libexif/exif-data.h>
#include <libjpeg/jpeg-data.h>

#include "TaggerInterface.h"

namespace android {

class ExifTagger : public TaggerInterface {
public:
    ExifTagger();
    ~ExifTagger();
    virtual int createTaggedJpeg(TaggerParams* p,
                                 const uint8_t* jpegData,
                                 unsigned int jpegSize);
    virtual int copyJpegWithExif(uint8_t* targetBuff, int targetBuffSize);

private:
    ExifData* _exif;
    JPEGData* _jpeg;

    uint8_t* _taggedJpegBuff;
    unsigned int _taggedJpegSize;

    int _buildExifData(TaggerParams* p);

};

}

#endif