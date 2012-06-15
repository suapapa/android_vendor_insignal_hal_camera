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

static const char TAG_MODEL[] = "Model";
static const char TAG_MAKE[] = "Make";
static const char TAG_FOCALLENGTH[] = "FocalLength";
static const char TAG_DATETIME[] = "DateTime";
static const char TAG_IMAGE_WIDTH[] = "ImageWidth";
static const char TAG_IMAGE_LENGTH[] = "ImageLength";
static const char TAG_GPS_LAT[] = "GPSLatitude";
static const char TAG_GPS_LAT_REF[] = "GPSLatitudeRef";
static const char TAG_GPS_LONG[] = "GPSLongitude";
static const char TAG_GPS_LONG_REF[] = "GPSLongitudeRef";
static const char TAG_GPS_ALT[] = "GPSAltitude";
static const char TAG_GPS_ALT_REF[] = "GPSAltitudeRef";
static const char TAG_GPS_MAP_DATUM[] = "GPSMapDatum";
static const char TAG_GPS_PROCESSING_METHOD[] = "GPSProcessingMethod";
static const char TAG_GPS_VERSION_ID[] = "GPSVersionID";
static const char TAG_GPS_TIMESTAMP[] = "GPSTimeStamp";
static const char TAG_GPS_DATESTAMP[] = "GPSDateStamp";
static const char TAG_ORIENTATION[] = "Orientation";

class ExifTagger : public TaggerInterface {
public:
    ExifTagger() :
        _gpsTagCount(0), _exifTagCount(0), _position(0),
        _jpegOpened(false) { }
    ~ExifTagger();

    virtual int createTaggedJpeg(TaggerParams* p,
                                 const uint8_t* jpegData,
                                 unsigned int jpegSize);
    virtual int copyJpegWithExif(uint8_t* targetBuff, int targetBuffSize);

    status_t insertElement(const char* tag, const char* value);
    void insertExifToJpeg(unsigned char* jpeg, size_t jpeg_size);
    status_t insertExifThumbnailImage(const char*, int);
    void saveJpeg(unsigned char* picture, size_t jpeg_size);
    static void stringToRational(const char*, unsigned int*, unsigned int*);
private:
    status_t _createExifElementTable(TaggerParams* p);

    ExifElement_t _table[MAX_EXIF_TAGS_SUPPORTED];
    unsigned int _gpsTagCount;
    unsigned int _exifTagCount;
    unsigned int _position;	// last idx of _table
    bool _jpegOpened;

    bool _isAsciiTag(const char* tag);
    const char* _degreesToExifOrientation(const char*);
};

}
#endif

