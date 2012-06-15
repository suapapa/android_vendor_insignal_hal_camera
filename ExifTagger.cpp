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

#define ARRAY_SIZE(array) (sizeof((array)) / sizeof((array)[0]))

namespace android {

struct string_pair {
    const char* string1;
    const char* string2;
};

static string_pair degress_to_exif_lut [] = {
    // degrees, exif_orientation
    {"0",   "1"},
    {"90",  "6"},
    {"180", "3"},
    {"270", "8"},
};

const char* ExifTagger::_degreesToExifOrientation(const char* degrees)
{
    for (unsigned int i = 0; i < ARRAY_SIZE(degress_to_exif_lut); i++) {
        if (!strcmp(degrees, degress_to_exif_lut[i].string1)) {
            return degress_to_exif_lut[i].string2;
        }
    }
    return NULL;
}

void ExifTagger::stringToRational(const char* str, unsigned int* num, unsigned int* den)
{
    int len;
    char* tempVal = NULL;

    if (str != NULL) {
        len = strlen(str);
        tempVal = (char*) malloc(sizeof(char) * (len + 1));
    }

    if (tempVal != NULL) {
        // convert the decimal string into a rational
        size_t den_len;
        char* ctx;
        unsigned int numerator = 0;
        unsigned int denominator = 0;
        char* temp = NULL;

        memset(tempVal, '\0', len + 1);
        strncpy(tempVal, str, len);
        temp = strtok_r(tempVal, ".", &ctx);

        if (temp != NULL)
            numerator = atoi(temp);

        if (!numerator)
            numerator = 1;

        temp = strtok_r(NULL, ".", &ctx);
        if (temp != NULL) {
            den_len = strlen(temp);
            if (HUGE_VAL == den_len) {
                den_len = 0;
            }

            denominator = static_cast<unsigned int>(pow(10, den_len));
            numerator = numerator * denominator + atoi(temp);
        } else {
            denominator = 1;
        }

        free(tempVal);

        *num = numerator;
        *den = denominator;
    }
}

bool ExifTagger::_isAsciiTag(const char* tag)
{
    // TODO(XXX): Add tags as necessary
    return (strcmp(tag, TAG_GPS_PROCESSING_METHOD) == 0);
}

void ExifTagger::insertExifToJpeg(unsigned char* jpeg, size_t jpeg_size)
{
    ReadMode_t read_mode = (ReadMode_t)(READ_METADATA | READ_IMAGE);

    ResetJpgfile();
    if (ReadJpegSectionsFromBuffer(jpeg, jpeg_size, read_mode)) {
        _jpegOpened = true;
        create_EXIF(_table, _exifTagCount, _gpsTagCount);
    }
}

status_t ExifTagger::insertExifThumbnailImage(const char* thumb, int len)
{
    status_t ret = NO_ERROR;

    if ((len > 0) && _jpegOpened) {
        ret = ReplaceThumbnailFromBuffer(thumb, len);
        LOGV("insertExifThumbnailImage. ReplaceThumbnail(). ret=%d", ret);
    }

    return ret;
}

void ExifTagger::saveJpeg(unsigned char* jpeg, size_t jpeg_size)
{
    if (_jpegOpened) {
        WriteJpegToBuffer(jpeg, jpeg_size);
        DiscardData();
        _jpegOpened = false;
    }
}

/* public functions */
ExifTagger::~ExifTagger()
{
    int num_elements = _gpsTagCount + _exifTagCount;

    for (int i = 0; i < num_elements; i++) {
        if (_table[i].Value) {
            free(_table[i].Value);
        }
    }

    if (_jpegOpened) {
        DiscardData();
    }
}

status_t ExifTagger::insertElement(const char* tag, const char* value)
{
    int value_length = 0;
    status_t ret = NO_ERROR;

    if (!value || !tag) {
        return -EINVAL;
    }

    if (_position >= MAX_EXIF_TAGS_SUPPORTED) {
        LOGE("Max number of EXIF elements already inserted");
        return NO_MEMORY;
    }

    if (_isAsciiTag(tag)) {
        value_length = sizeof(ExifAsciiPrefix) + strlen(value + sizeof(ExifAsciiPrefix));
    } else {
        value_length = strlen(value);
    }

    ExifElement_t *e = &(_table[_position]);
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

    _position++;
    return ret;
}

int ExifTagger::createTaggedJpeg(TaggerParams* p,
                                 const uint8_t* jpegData,
                                 unsigned int jpegSize)
{
    // TODO: Implement TaggerInterface

    return 0;
}

int ExifTagger::copyJpegWithExif(uint8_t* targetBuff,
                                 int targetBuffSize)
{
    // TODO: Implement TaggerInterface

    return 0;
}

}

