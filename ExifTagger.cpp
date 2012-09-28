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
#define LOG_TAG "ExifTagger"
#include <utils/Log.h>

#include <math.h>

#include "ExifTagger.h"

#define RET_IF_ERR(R) \
    if (NO_ERROR != ret) {\
        LOGE("%s:%d somthing wrong! ret = %d", __func__, __LINE__, ret); \
        return ret; \
    }

namespace android {

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
static const char TAG_FLASH[] = "Flash";
static const char TAG_DIGITALZOOMRATIO[] = "DigitalZoomRatio";
static const char TAG_EXPOSURETIME[] = "ExposureTime";
static const char TAG_APERTURE[] = "ApertureValue";
static const char TAG_ISO_EQUIVALENT[] = "ISOSpeedRatings";
static const char TAG_WHITEBALANCE[] = "WhiteBalance";
static const char TAG_LIGHT_SOURCE[] = "LightSource";
static const char TAG_METERING_MODE[] = "MeteringMode";
static const char TAG_EXPOSURE_PROGRAM[] = "ExposureProgram";
static const char TAG_COLOR_SPACE[] = "ColorSpace";
static const char TAG_CPRS_BITS_PER_PIXEL[] = "CompressedBitsPerPixel";
static const char TAG_FNUMBER[] = "FNumber";
static const char TAG_SHUTTERSPEED[] = "ShutterSpeedValue";
static const char TAG_SENSING_METHOD[] = "SensingMethod";
static const char TAG_CUSTOM_RENDERED[] = "CustomRendered";

ExifTagger::ExifTagger() :
    _gpsTagCount(0),
    _exifTagCount(0),
    _jpegOpened(false)
{
    memset(&_table, 0, sizeof(ExifElement_t) * MAX_EXIF_TAGS_SUPPORTED);
}

ExifTagger::~ExifTagger()
{
    _release();
}

int ExifTagger::tagToJpeg(TaggerParams* p,
                          const uint8_t* srcBuff,
                          unsigned int srcSize)
{
    if (_jpegOpened) {
        LOGE("Previous tagging not completed!");
        return 0;
    }

    if (p == NULL) {
        LOGE("TaggerParams is NULL!");
        return 0;
    }

    _gpsTagCount = 0;
    _exifTagCount = 0;

    status_t ret;
    ret = _fillExifTable(p);

    ResetJpgfile();

    ReadMode_t rMode = (ReadMode_t)(READ_METADATA | READ_IMAGE);
    if (ReadJpegSectionsFromBuffer((unsigned char*)srcBuff, srcSize, rMode)) {
        _jpegOpened = true;
        create_EXIF(_table, _exifTagCount, _gpsTagCount);

        const char* td = (const char*)(p->thumbData);
        unsigned int ts = p->thumbSize;
        if (td && ts) {
            status_t ret = ReplaceThumbnailFromBuffer(td, ts);
            LOGV("insertExifThumbnailImage. ReplaceThumbnail(). ret=%d", ret);
        } else {
            LOGI("no thumbnail!");
        }
    }

    Section_t* exif_section = FindSection(M_EXIF);

    return srcSize + exif_section->Size;
}

int ExifTagger::writeTaggedJpeg(uint8_t* destBuff,
                                int destBuffSize)
{
    if (_jpegOpened == false) {
        LOGE("%s: Jpeg not opened!", __func__);
        return -1;
    }

    WriteJpegToBuffer(destBuff, destBuffSize);
    _release();

    return destBuffSize;
}

bool ExifTagger::readyToWrite(void)
{
    return _jpegOpened;
}

void ExifTagger::_release(void)
{
    for (int i = 0; i < MAX_EXIF_TAGS_SUPPORTED; i++) {
        if (_table[i].Value) {
            free(_table[i].Value);
            _table[i].Value = NULL;
        }
    }

    if (_jpegOpened) {
        DiscardData();
    }
    _jpegOpened = false;
}
status_t ExifTagger::_fillExifTable(TaggerParams* p)
{
    status_t ret = NO_ERROR;
    char tempStr[256];

    /*
    if (p->maker) {
        ret = _insertTag(TAG_MAKE, p->maker);
        RET_IF_ERR(ret);
    }

    if (p->model) {
        ret = _insertTag(TAG_MODEL, p->model);
        RET_IF_ERR(ret);
    }
    */

    ret = _insertTag(TAG_MAKE, "Insignal Co,. Ltd");
    RET_IF_ERR(ret);

    ret = _insertTag(TAG_MODEL, "ORIGEN_quad board");
    RET_IF_ERR(ret);

    if (p->focalNum || p->focalDen) {
        sprintf(tempStr, "%u/%u", p->focalNum, p->focalDen);
        ret = _insertTag(TAG_FOCALLENGTH, tempStr);
        RET_IF_ERR(ret);
    }

    ret = _insertDatetimeTag();
    RET_IF_ERR(ret);

    ret = _insertOrientationTag(p->rotation);
    RET_IF_ERR(ret);

    ret = _insertTag(TAG_IMAGE_WIDTH, p->width);
    RET_IF_ERR(ret);

    ret = _insertTag(TAG_IMAGE_LENGTH, p->height);
    RET_IF_ERR(ret);

    ret = _insertGpsTag(&(p->gps));
    RET_IF_ERR(ret);

    return ret;
}


status_t ExifTagger::_insertTag(const char* tag, const char* value)
{
    int value_length = 0;
    status_t ret = NO_ERROR;

    if (!value || !tag) {
        return -EINVAL;
    }

    LOGV("t:v = %s:%s", tag, value);

    unsigned int pos = _gpsTagCount + _exifTagCount;
    if (pos >= MAX_EXIF_TAGS_SUPPORTED) {
        LOGE("Max number of EXIF elements already inserted");
        return NO_MEMORY;
    }

    if (0 == strncmp(value, ExifAsciiPrefix, sizeof(ExifAsciiPrefix))) {
        value_length = sizeof(ExifAsciiPrefix) + strlen(value + sizeof(ExifAsciiPrefix));
    } else {
        value_length = strlen(value);
    }

    ExifElement_t* e = &(_table[pos]);
    if (IsGpsTag(tag)) {
        e->GpsTag = TRUE;
        e->Tag = GpsTagNameToValue(tag);
        _gpsTagCount++;
    } else {
        e->GpsTag = FALSE;
        e->Tag = TagNameToValue(tag);
        _exifTagCount++;
    }

    e->DataLength = 0;
    e->Value = (char*) malloc(sizeof(char) * (value_length + 1));

    if (e->Value) {
        memcpy(e->Value, value, value_length + 1);
        e->DataLength = value_length + 1;
    }

    return ret;
}

status_t ExifTagger::_insertTag(const char* tag, unsigned int value)
{
    char valueStr[5];
    sprintf(valueStr, "%d", value);

    return _insertTag(tag, valueStr);
}

status_t ExifTagger::_insertOrientationTag(unsigned int degree)
{
    unsigned int lut = 0;
    switch (degree) {
    case 0:
        lut = 1;
        break;
    case 90:
        lut = 6;
        break;
    case 180:
        lut = 3;
        break;
    case 270:
        lut = 8;
        break;
    }

    if (lut == 0) {
        LOGW("Skip tag orientation! degree, %d not supported!", degree);
        return NO_ERROR;
    }

    return _insertTag(TAG_ORIENTATION, lut);
}

status_t ExifTagger::_insertDatetimeTag(void)
{
    struct timeval sTv;
    struct tm* pTime;
    int status = gettimeofday(&sTv, NULL);
    if (status) {
        return UNKNOWN_ERROR;
    }
    pTime = localtime(&sTv.tv_sec);
    if (NULL == pTime) {
        return UNKNOWN_ERROR;
    }
    char tempStr[256];
    sprintf(tempStr, "%04d:%02d:%02d %02d:%02d:%02d",
            pTime->tm_year + 1900,
            pTime->tm_mon + 1,
            pTime->tm_mday,
            pTime->tm_hour,
            pTime->tm_min,
            pTime->tm_sec);
    return _insertTag(TAG_DATETIME, tempStr);
}

status_t ExifTagger::_insertGpsTag(_gpsData* gps)
{
    int d, m, s, sd;
    char tempStr[256];
    status_t ret;

    // latitude
    ret = _convertGPSCoord(gps->latitude, d, m, s, sd);
    RET_IF_ERR(ret);
    sprintf(tempStr, "%d/%d,%d/%d,%d/%d",
            abs(d), 1, abs(m), 1, abs(s), abs(sd));
    ret = _insertTag(TAG_GPS_LAT, tempStr);
    RET_IF_ERR(ret);
    ret = _insertTag(TAG_GPS_LAT_REF,
                     gps->latitude > 0 ? "N" : "S");
    RET_IF_ERR(ret);

    // longitute
    ret = _convertGPSCoord(gps->longitude, d, m, s, sd);
    RET_IF_ERR(ret);
    ret = _insertTag(TAG_GPS_LONG, tempStr);
    RET_IF_ERR(ret);
    ret = _insertTag(TAG_GPS_LONG_REF,
                     gps->longitude > 0 ? "E" : "W");
    RET_IF_ERR(ret);

    // altitude
    sprintf(tempStr, "%d/%d",
            abs(floor(fabs(gps->altitude))), 1);
    ret = _insertTag(TAG_GPS_ALT, tempStr);
    RET_IF_ERR(ret);
    ret = _insertTag(TAG_GPS_ALT_REF,
                     gps->altitude > 0 ? 0 : 1);
    RET_IF_ERR(ret);

    // procMethod
    if (0 /* gps->procMethod */) {
        memcpy(tempStr, ExifAsciiPrefix, sizeof(ExifAsciiPrefix));
        strcpy(tempStr + sizeof(ExifAsciiPrefix), gps->procMethod);
        ret = _insertTag(TAG_GPS_PROCESSING_METHOD, tempStr);
        RET_IF_ERR(ret);
    }

    // timestamp
    struct tm* timeinfo = gmtime((time_t*) & (gps->timestamp));
    sprintf(tempStr, "%d/%d,%d/%d,%d/%d",
            timeinfo->tm_hour, 1,
            timeinfo->tm_min, 1,
            timeinfo->tm_sec, 1);
    ret = _insertTag(TAG_GPS_TIMESTAMP, tempStr);
    RET_IF_ERR(ret);

    return NO_ERROR;
}

status_t ExifTagger::_convertGPSCoord(double coord,
                                      int& deg,
                                      int& min,
                                      int& sec,
                                      int& secDivisor)
{
    if (coord == 0) {
        LOGE("Invalid GPS coordinate");
        return -EINVAL;
    }

#define GPS_MIN_DIV                 60
#define GPS_SEC_DIV                 60
#define GPS_SEC_ACCURACY            1000

    double tmp;

    deg = (int) floor(fabs(coord));
    tmp = (fabs(coord) - floor(fabs(coord))) * GPS_MIN_DIV;
    min = (int) floor(tmp);
    tmp = (tmp - floor(tmp)) * (GPS_SEC_DIV * GPS_SEC_ACCURACY);
    sec = (int) floor(tmp);
    secDivisor = GPS_SEC_ACCURACY;

    if (sec >= (GPS_SEC_DIV * GPS_SEC_ACCURACY)) {
        sec = 0;
        min += 1;
    }

    if (min >= 60) {
        min = 0;
        deg += 1;
    }

    return NO_ERROR;
}

}

