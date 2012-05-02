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

//#define LOG_NDEBUG 0
#define LOG_TAG "ExifTagger"
#include <utils/Log.h>

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <libexif/exif-entry.h>
#include <libexif/exif-ifd.h>
#include <libexif/exif-loader.h>

#include "ExifTagger.h"

namespace android {

static void exif_entry_set_string(ExifData* pEdata, ExifIfd eEifd, ExifTag eEtag,
                                  const char* data);

static void exif_entry_set_undefined(ExifData* pEdata, ExifIfd eEifd, ExifTag eEtag,
                                     void* data = NULL, unsigned int size = 0);

static void exif_entry_set_byte(ExifData* pEdata, ExifIfd eEifd, ExifTag eEtag,
                                ExifByte n);
static void exif_entry_set_short(ExifData* pEdata, ExifIfd eEifd, ExifTag eEtag,
                                 ExifShort n);
static void exif_entry_set_long(ExifData* pEdata, ExifIfd eEifd, ExifTag eEtag,
                                ExifLong n);
static void exif_entry_set_rational(ExifData* pEdata, ExifIfd eEifd, ExifTag eEtag,
                                    ExifRational r);

static void exif_entry_set_sbyte(ExifData* pEdata, ExifIfd eEifd, ExifTag eEtag,
                                 ExifSByte n);
static void exif_entry_set_sshort(ExifData* pEdata, ExifIfd eEifd, ExifTag eEtag,
                                  ExifSShort n);
static void exif_entry_set_slong(ExifData* pEdata, ExifIfd eEifd, ExifTag eEtag,
                                 ExifSLong n);
static void exif_entry_set_srational(ExifData* pEdata, ExifIfd eEifd, ExifTag eEtag,
                                     ExifSRational r);


static void exif_entry_set_gps_coord(ExifData* pEdata, ExifIfd eEifd, ExifTag eEtag,
                                     ExifRational r1, ExifRational r2, ExifRational r3);
static void exif_entry_set_gps_altitude(ExifData* pEdata, ExifIfd eEifd, ExifTag eEtag,
                                        ExifRational r1);
static void exif_entry_set_gps_version(ExifData* pEdata, ExifIfd eEifd, ExifTag eEtag,
                                       ExifByte r1, ExifByte r2, ExifByte r3, ExifByte r4);

static const char ExifAsciiPrefix[] = { 0x41, 0x53, 0x43, 0x49, 0x49, 0x0, 0x0, 0x0 };


ExifTagger::ExifTagger() :
    _exif(NULL),
    _jpeg(NULL),
    _taggedJpegBuff(NULL),
    _taggedJpegSize(0)
{
    LOGV("%s", __func__);
}

ExifTagger::~ExifTagger()
{
    LOGV("%s", __func__);

    exif_data_free(_exif);
    jpeg_data_free(_jpeg);
    if (_taggedJpegBuff)
        free(_taggedJpegBuff);
}

int ExifTagger::createTaggedJpeg(TaggerParams* p,
                                 const uint8_t* jpegData,
                                 unsigned int jpegSize)
{
    LOGV("building EXIF...");
    _buildExifData(p);

    if (_jpeg) {
        LOGW("Free previous alloc for ExifData!");
        jpeg_data_free(_jpeg);
    }

    LOGV("parsing JPEG...");
    _jpeg = jpeg_data_new_from_data(jpegData, jpegSize);

    LOGV("joining EXIF to JPEG...");
    jpeg_data_set_exif_data(_jpeg, _exif);

    if (_taggedJpegBuff) {
        LOGW("cleaning up previous buffer for taggedJPEG."
             "size was %d", _taggedJpegSize);
        free(_taggedJpegBuff);
        _taggedJpegBuff = NULL;
        _taggedJpegSize = 0;
    }

    jpeg_data_save_data(_jpeg, &_taggedJpegBuff, &_taggedJpegSize);
    LOGV("tagged JPEG generated. size = %d", _taggedJpegSize);

    LOGV("freeing _exif and _jpeg...");
    exif_data_free(_exif);
    _exif = NULL;
    jpeg_data_free(_jpeg);
    _jpeg = NULL;

    return _taggedJpegSize;
}

int ExifTagger::copyJpegWithExif(uint8_t* targetBuff,
                                 int targetBuffSize)
{
    if (targetBuffSize < _taggedJpegSize) {
        LOGE("%s: Target buffer(%d) is too small for JPEG(%d)!",
             __func__, targetBuffSize, _taggedJpegSize);
        return -1;
    }

    int writtenSize = _taggedJpegSize;
    memcpy(targetBuff, _taggedJpegBuff, writtenSize);

    free(_taggedJpegBuff);
    _taggedJpegBuff = NULL;

    return writtenSize;
}

int ExifTagger::_buildExifData(TaggerParams* par)
{
    struct tm* sTime;
    char* TimeStr = NULL;
    int res;

    if (par == NULL) {
        LOGE("%s: NULL given for TaggerParams*", __func__);
        return -1;
    }

    if (_exif != NULL) {
        LOGW("Free previous alloc for ExifData!");
        exif_data_free(_exif);
    }

    _exif = exif_data_new();
    if (_exif == NULL) {
        LOGE("%s: Fail to alloc memory for ExifData!", __func__);
        return -1;
    }

    exif_entry_set_string(_exif, EXIF_IFD_0, EXIF_TAG_MAKE, par->maker);
    exif_entry_set_string(_exif, EXIF_IFD_0, EXIF_TAG_MODEL, par->model);

    exif_entry_set_short(_exif, EXIF_IFD_0, EXIF_TAG_IMAGE_LENGTH, par->width);
    exif_entry_set_short(_exif, EXIF_IFD_0, EXIF_TAG_IMAGE_WIDTH, par->height);

    switch (par->rotation) {
    case 0:
        exif_entry_set_short(_exif, EXIF_IFD_0, EXIF_TAG_ORIENTATION, 1);
        break;
    case 90:
        exif_entry_set_short(_exif, EXIF_IFD_0, EXIF_TAG_ORIENTATION, 6);
        break;
    case 180:
        exif_entry_set_short(_exif, EXIF_IFD_0, EXIF_TAG_ORIENTATION, 3);
        break;
    case 270:
        exif_entry_set_short(_exif, EXIF_IFD_0, EXIF_TAG_ORIENTATION, 8);
        break;
    };

    ExifRational sR;
    sR.numerator = 4 * 100 + 68;
    sR.denominator = 100;
    exif_entry_set_rational(_exif, EXIF_IFD_EXIF, EXIF_TAG_FOCAL_LENGTH, sR);

    exif_entry_set_short(_exif, EXIF_IFD_EXIF, EXIF_TAG_FLASH, 0);

    switch (par->metering_mode) {
    case EXIF_CENTER:
        exif_entry_set_short(_exif, EXIF_IFD_EXIF, EXIF_TAG_METERING_MODE, 1);
        break;
    case EXIF_AVERAGE:
        exif_entry_set_short(_exif, EXIF_IFD_EXIF, EXIF_TAG_METERING_MODE, 2);
        break;
    };

    switch (par->iso) {
    case EXIF_ISO_AUTO:
        exif_entry_set_short(_exif, EXIF_IFD_EXIF, EXIF_TAG_ISO_SPEED_RATINGS, 0);
        break;
    case EXIF_ISO_100:
        exif_entry_set_short(_exif, EXIF_IFD_EXIF, EXIF_TAG_ISO_SPEED_RATINGS, 100);
        break;
    case EXIF_ISO_200:
        exif_entry_set_short(_exif, EXIF_IFD_EXIF, EXIF_TAG_ISO_SPEED_RATINGS, 200);
        break;
    case EXIF_ISO_400:
        exif_entry_set_short(_exif, EXIF_IFD_EXIF, EXIF_TAG_ISO_SPEED_RATINGS, 400);
        break;
    case EXIF_ISO_800:
        exif_entry_set_short(_exif, EXIF_IFD_EXIF, EXIF_TAG_ISO_SPEED_RATINGS, 800);
        break;
    case EXIF_ISO_1000:
        exif_entry_set_short(_exif, EXIF_IFD_EXIF, EXIF_TAG_ISO_SPEED_RATINGS, 1000);
        break;
    case EXIF_ISO_1200:
        exif_entry_set_short(_exif, EXIF_IFD_EXIF, EXIF_TAG_ISO_SPEED_RATINGS, 1200);
        break;
    case EXIF_ISO_1600:
        exif_entry_set_short(_exif, EXIF_IFD_EXIF, EXIF_TAG_ISO_SPEED_RATINGS, 1600);
        break;
    };

    sR.numerator = par->zoom * 100;
    sR.denominator = 100;
    exif_entry_set_rational(_exif, EXIF_IFD_EXIF, EXIF_TAG_DIGITAL_ZOOM_RATIO, sR);

    if (EXIF_WB_AUTO == par->wb)
        exif_entry_set_short(_exif, EXIF_IFD_EXIF, EXIF_TAG_WHITE_BALANCE, 0);
    else
        exif_entry_set_short(_exif, EXIF_IFD_EXIF, EXIF_TAG_WHITE_BALANCE, 1);

    exif_entry_set_short(_exif, EXIF_IFD_EXIF, EXIF_TAG_FLASH, par->flash);

    sR.numerator = par->exposure;
    sR.denominator = 1000000;
    exif_entry_set_rational(_exif, EXIF_IFD_EXIF, EXIF_TAG_EXPOSURE_TIME, sR);

    /* resolution */
    sR.numerator = 72;
    sR.denominator = 1;
    exif_entry_set_rational(_exif, EXIF_IFD_0, EXIF_TAG_X_RESOLUTION, sR);
    exif_entry_set_rational(_exif, EXIF_IFD_0, EXIF_TAG_Y_RESOLUTION, sR);
    exif_entry_set_short(_exif, EXIF_IFD_0, EXIF_TAG_RESOLUTION_UNIT, 2);  /* inches */
    exif_entry_set_short(_exif, EXIF_IFD_0, EXIF_TAG_YCBCR_POSITIONING, 1);  /* centered */
    exif_entry_set_short(_exif, EXIF_IFD_0, EXIF_TAG_YCBCR_SUB_SAMPLING, 0);

    /* Exif version */
    exif_entry_set_undefined(_exif, EXIF_IFD_EXIF, EXIF_TAG_EXIF_VERSION, NULL);

    /* flashpix version */
    exif_entry_set_undefined(_exif, EXIF_IFD_EXIF, EXIF_TAG_FLASH_PIX_VERSION, NULL);

    /* file source */
    exif_entry_set_undefined(_exif, EXIF_IFD_EXIF, EXIF_TAG_FILE_SOURCE, NULL);

    /* file name */
    exif_entry_set_undefined(_exif, EXIF_IFD_EXIF, EXIF_TAG_DOCUMENT_NAME, NULL);

    /* scene type */
    exif_entry_set_undefined(_exif, EXIF_IFD_EXIF, EXIF_TAG_SCENE_TYPE, NULL);

    /* Color Components */
    exif_entry_set_undefined(_exif, EXIF_IFD_EXIF, EXIF_TAG_COMPONENTS_CONFIGURATION, NULL);

    /* Bits per sample */
    exif_entry_set_undefined(_exif, EXIF_IFD_0, EXIF_TAG_BITS_PER_SAMPLE, NULL);

    /* Color space */
    exif_entry_set_short(_exif, EXIF_IFD_EXIF, EXIF_TAG_COLOR_SPACE, 1);

    /* Interoperability index */
    exif_entry_set_string(_exif, EXIF_IFD_INTEROPERABILITY, EXIF_TAG_INTEROPERABILITY_INDEX, "R98");

    /* time */
    /* this sould be last resort */
    struct timeval sTv;
    res = gettimeofday(&sTv, NULL);
    sTime = localtime(&sTv.tv_sec);
    if (res == 0 && sTime != NULL) {
        TimeStr = (char*) malloc(20); /* No data for secondary sensor */

        if (TimeStr != NULL) {
            snprintf(TimeStr, 20, "%04d:%02d:%02d %02d:%02d:%02d",
                     sTime->tm_year + 1900,
                     sTime->tm_mon + 1,
                     sTime->tm_mday,
                     sTime->tm_hour,
                     sTime->tm_min,
                     sTime->tm_sec
                    );
            exif_entry_set_string(_exif, EXIF_IFD_0, EXIF_TAG_DATE_TIME, TimeStr);
            exif_entry_set_string(_exif, EXIF_IFD_EXIF, EXIF_TAG_DATE_TIME_ORIGINAL, TimeStr);
            exif_entry_set_string(_exif, EXIF_IFD_EXIF, EXIF_TAG_DATE_TIME_DIGITIZED, TimeStr);
            snprintf(TimeStr, 20, "%06d", (int) sTv.tv_usec);
            exif_entry_set_string(_exif, EXIF_IFD_EXIF, EXIF_TAG_SUB_SEC_TIME, TimeStr);
            exif_entry_set_string(_exif, EXIF_IFD_EXIF, EXIF_TAG_SUB_SEC_TIME_ORIGINAL, TimeStr);
            exif_entry_set_string(_exif, EXIF_IFD_EXIF, EXIF_TAG_SUB_SEC_TIME_DIGITIZED, TimeStr);
            free(TimeStr);
            TimeStr = NULL;
        } else {
            LOGE("%s():%d:!!!!!ERROR:  malloc Failed.\n", __FUNCTION__, __LINE__);
        }
    } else {
        LOGE("Error in time recognition. res: %d sTime: %p\n%s\n", res, sTime, strerror(errno));
    }

    exif_entry_set_short(_exif, EXIF_IFD_1, EXIF_TAG_COMPRESSION, 6);  /* JPEG */
    exif_entry_set_long(_exif, EXIF_IFD_1, EXIF_TAG_JPEG_INTERCHANGE_FORMAT, 0xFFFFFFFF);
    exif_entry_set_long(_exif, EXIF_IFD_1, EXIF_TAG_JPEG_INTERCHANGE_FORMAT_LENGTH, 0xFFFFFFFF);

    if (par->gps_location.enable) {
        ExifRational r1, r2, r3;
        gps_data* gps = &par->gps_location;

        /* gps data */
        r1.denominator = 1;
        r2.denominator = 1;
        r3.denominator = 1;

        r1.numerator = gps->longDeg;
        r2.numerator = gps->longMin;
        r3.numerator = gps->longSec;

        exif_entry_set_gps_coord(_exif, EXIF_IFD_GPS, (ExifTag) EXIF_TAG_GPS_LONGITUDE, r1, r2, r3);

        r1.numerator = gps->latDeg;
        r2.numerator = gps->latMin;
        r3.numerator = gps->latSec;

        exif_entry_set_gps_coord(_exif, EXIF_IFD_GPS, (ExifTag) EXIF_TAG_GPS_LATITUDE, r1, r2, r3);

        r1.numerator = gps->altitude;
        exif_entry_set_gps_altitude(_exif, EXIF_IFD_GPS, (ExifTag) EXIF_TAG_GPS_ALTITUDE, r1);

        sTime = localtime((const time_t*) &gps->timestamp);
        if (NULL != sTime) {
            r1.numerator = sTime->tm_hour;
            r2.numerator = sTime->tm_min;
            r3.numerator = sTime->tm_sec;

            exif_entry_set_gps_coord(_exif, EXIF_IFD_GPS, (ExifTag) EXIF_TAG_GPS_TIME_STAMP, r1, r2, r3);
        }

        exif_entry_set_byte(_exif, EXIF_IFD_GPS, (ExifTag) EXIF_TAG_GPS_ALTITUDE_REF, (ExifByte) gps->altitudeRef);

        if (NULL != gps->latRef)
            exif_entry_set_string(_exif, EXIF_IFD_GPS, (ExifTag) EXIF_TAG_GPS_LATITUDE_REF, gps->latRef);

        if (NULL != gps->longRef)
            exif_entry_set_string(_exif, EXIF_IFD_GPS, (ExifTag) EXIF_TAG_GPS_LONGITUDE_REF, gps->longRef);

        if (NULL != gps->procMethod) {
            //using strlen since i don't want the terminating null
            unsigned char* data = (unsigned char*)malloc(strlen(gps->procMethod) + sizeof(ExifAsciiPrefix));
            if (data != NULL) {
                memcpy(data, ExifAsciiPrefix, sizeof(ExifAsciiPrefix));
                memcpy(data + sizeof(ExifAsciiPrefix), gps->procMethod, strlen(gps->procMethod));
                exif_entry_set_undefined(_exif, EXIF_IFD_GPS, (ExifTag) EXIF_TAG_GPS_PROCESSING_METHOD,
                                         data, (strlen(gps->procMethod) + sizeof(ExifAsciiPrefix)));
                free(data);
            }
        }

        if (NULL != gps->mapdatum)
            exif_entry_set_string(_exif, EXIF_IFD_GPS, (ExifTag) EXIF_TAG_GPS_MAP_DATUM, gps->mapdatum);

        if (strlen(gps->datestamp) == 10)
            exif_entry_set_string(_exif, EXIF_IFD_GPS, (ExifTag) EXIF_TAG_GPS_DATE_STAMP, gps->datestamp);

        if (NULL != gps->versionId)
            exif_entry_set_gps_version(_exif, EXIF_IFD_GPS, (ExifTag) EXIF_TAG_GPS_VERSION_ID,
                                       gps->versionId[0], gps->versionId[1], gps->versionId[2], gps->versionId[3]);
    }

    /* thumbnail */
    if (par->thumb_data && par->thumb_size) {
        _exif->data = par->thumb_data;
        _exif->size = par->thumb_size;
    }

    LOGV("%s: all done!", __func__);

    return 0;
}


static void exif_entry_set_string(ExifData* pEdata, ExifIfd eEifd, ExifTag eEtag, const char* s)
{
    ExifEntry* pE;

    pE = exif_entry_new();
    exif_content_add_entry(pEdata->ifd[eEifd], pE);
    exif_entry_initialize(pE, eEtag);
    if (pE->data)
        free(pE->data);
    pE->components = strlen(s) + 1;
    pE->size = sizeof(char) * pE->components;
    pE->data = (unsigned char*) malloc(pE->size);
    if (!pE->data) {
        printf("Cannot allocate %d bytes.\nTerminating.\n", (int) pE->size);
        exit(1);
    }
    strcpy((char*) pE->data, (char*) s);
    exif_entry_fix(pE);
    exif_entry_unref(pE);
}


static void exif_entry_set_undefined(ExifData* pEdata, ExifIfd eEifd, ExifTag eEtag,
                                     void* data, unsigned int size)
{
    ExifEntry* pE;

    pE = exif_entry_new();
    exif_content_add_entry(pEdata->ifd[eEifd], pE);
    exif_entry_initialize(pE, eEtag);
    if (data != NULL && size != 0) {
        if (pE->data)
            free(pE->data);
        pE->components = (long unsigned int)data;
        pE->size = size;
        pE->data = (unsigned char*) malloc(pE->size);
        if (!pE->data) {
            printf("Cannot allocate %d bytes.\nTerminating.\n", (int) pE->size);
            exit(1);
        }
        memcpy((void*) pE->data, (void*) data, size);
    }
    exif_entry_fix(pE);
    exif_entry_unref(pE);
}

static void  exif_entry_set_byte(ExifData* pEdata, ExifIfd eEifd, ExifTag eEtag,
                                 ExifByte n)
{
    ExifEntry* pE;
    unsigned char* pData;

    pE = exif_entry_new();
    exif_content_add_entry(pEdata->ifd[eEifd], pE);
    exif_entry_initialize(pE, eEtag);

    pData = (unsigned char*)(pE->data);
    if (pData) {
        *pData = n;
    } else {
        printf("ERROR: unallocated e->data Tag %d\n", eEtag);
    }
    exif_entry_fix(pE);
    exif_entry_unref(pE);
}

static void exif_entry_set_short(ExifData* pEdata, ExifIfd eEifd, ExifTag eEtag,
                                 ExifShort n)
{
    ExifEntry* pE;

    ExifByteOrder eO;

    pE = exif_entry_new();
    exif_content_add_entry(pEdata->ifd[eEifd], pE);
    exif_entry_initialize(pE, eEtag);
    eO = exif_data_get_byte_order(pE->parent->parent);
    if (pE->data) {
        exif_set_short(pE->data, eO, n);
    } else {
        printf("ERROR: unallocated e->data Tag %d\n", eEtag);
    }
    exif_entry_fix(pE);
    exif_entry_unref(pE);
}

static void exif_entry_set_long(ExifData* pEdata, ExifIfd eEifd, ExifTag eEtag,
                                ExifLong n)
{
    ExifEntry* pE;

    ExifByteOrder eO;

    pE = exif_entry_new();
    exif_content_add_entry(pEdata->ifd[eEifd], pE);
    exif_entry_initialize(pE, eEtag);
    eO = exif_data_get_byte_order(pE->parent->parent);
    if (pE->data) {
        exif_set_long(pE->data, eO, n);
    } else {
        printf("ERROR: unallocated e->data Tag %d\n", eEtag);
    }
    exif_entry_fix(pE);
    exif_entry_unref(pE);
}

static void exif_entry_set_rational(ExifData* pEdata, ExifIfd eEifd, ExifTag eEtag,
                                    ExifRational r)
{
    ExifEntry* pE;

    ExifByteOrder eO;

    pE = exif_entry_new();
    exif_content_add_entry(pEdata->ifd[eEifd], pE);
    exif_entry_initialize(pE, eEtag);
    eO = exif_data_get_byte_order(pE->parent->parent);
    if (pE->data) {
        exif_set_rational(pE->data, eO, r);
    } else {
        printf("ERROR: unallocated e->data Tag %d\n", eEtag);
    }
    exif_entry_fix(pE);
    exif_entry_unref(pE);
}

static void exif_entry_set_sbyte(ExifData* pEdata, ExifIfd eEifd, ExifTag eEtag,
                                 ExifSByte n)
{
    ExifEntry* pE;

    char* pData;

    pE = exif_entry_new();
    exif_content_add_entry(pEdata->ifd[eEifd], pE);
    exif_entry_initialize(pE, eEtag);
    pData = (char*)(pE->data);
    if (pData) {
        *pData = n;
    } else {
        printf("ERROR: unallocated e->data Tag %d\n", eEtag);
    }
    exif_entry_fix(pE);
    exif_entry_unref(pE);
}

static void exif_entry_set_sshort(ExifData* pEdata, ExifIfd eEifd, ExifTag eEtag,
                                  ExifSShort n)
{
    ExifEntry* pE;

    ExifByteOrder eO;

    pE = exif_entry_new();
    exif_content_add_entry(pEdata->ifd[eEifd], pE);
    exif_entry_initialize(pE, eEtag);
    eO = exif_data_get_byte_order(pE->parent->parent);
    if (pE->data) {
        exif_set_sshort(pE->data, eO, n);
    } else {
        printf("ERROR: unallocated e->data Tag %d\n", eEtag);
    }
    exif_entry_fix(pE);
    exif_entry_unref(pE);
}

static void exif_entry_set_slong(ExifData* pEdata, ExifIfd eEifd, ExifTag eEtag,
                                 ExifSLong n)
{
    ExifEntry* pE;

    ExifByteOrder eO;

    pE = exif_entry_new();
    exif_content_add_entry(pEdata->ifd[eEifd], pE);
    exif_entry_initialize(pE, eEtag);
    eO = exif_data_get_byte_order(pE->parent->parent);
    if (pE->data) {
        exif_set_slong(pE->data, eO, n);
    } else {
        printf("ERROR: unallocated e->data Tag %d\n", eEtag);
    }
    exif_entry_fix(pE);
    exif_entry_unref(pE);
}

static void exif_entry_set_srational(ExifData* pEdata, ExifIfd eEifd, ExifTag eEtag,
                                     ExifSRational r)
{
    ExifEntry* pE;
    ExifByteOrder eO;

    pE = exif_entry_new();
    exif_content_add_entry(pEdata->ifd[eEifd], pE);
    exif_entry_initialize(pE, eEtag);
    eO = exif_data_get_byte_order(pE->parent->parent);
    if (pE->data) {
        exif_set_srational(pE->data, eO, r);
    } else {
        printf("ERROR: unallocated e->data Tag %d\n", eEtag);
    }
    exif_entry_fix(pE);
    exif_entry_unref(pE);
}

static void exif_entry_set_gps_coord(ExifData* pEdata, ExifIfd eEifd, ExifTag eEtag,
                                     ExifRational r1, ExifRational r2, ExifRational r3)
{
    ExifEntry* pE;
    ExifByteOrder eO;

    pE = exif_entry_new();
    exif_content_add_entry(pEdata->ifd[eEifd], pE);
    exif_entry_gps_initialize(pE, eEtag);
    eO = exif_data_get_byte_order(pE->parent->parent);
    if (pE->data) {
        exif_set_rational(pE->data, eO, r1);
        exif_set_rational(pE->data + exif_format_get_size(pE->format), eO, r2);
        exif_set_rational(pE->data + 2 * exif_format_get_size(pE->format), eO,
                          r3);
    } else {
        printf("ERROR: unallocated e->data Tag %d\n", eEtag);
    }
    exif_entry_fix(pE);
    exif_entry_unref(pE);
}

static void exif_entry_set_gps_altitude(ExifData* pEdata, ExifIfd eEifd, ExifTag eEtag, ExifRational r1)
{
    ExifEntry* pE;
    ExifByteOrder eO;

    pE = exif_entry_new();
    exif_content_add_entry(pEdata->ifd[eEifd], pE);
    exif_entry_gps_initialize(pE, eEtag);
    eO = exif_data_get_byte_order(pE->parent->parent);
    if (pE->data) {
        exif_set_rational(pE->data, eO, r1);
    } else {
        printf("ERROR: unallocated e->data Tag %d\n", eEtag);
    }
    exif_entry_fix(pE);
    exif_entry_unref(pE);
}

static void exif_entry_set_gps_version(ExifData* pEdata, ExifIfd eEifd, ExifTag eEtag, ExifByte r1, ExifByte r2, ExifByte r3, ExifByte r4)
{
    ExifEntry* pE;
    ExifByteOrder eO;

    pE = exif_entry_new();
    exif_content_add_entry(pEdata->ifd[eEifd], pE);
    exif_entry_gps_initialize(pE, eEtag);
    eO = exif_data_get_byte_order(pE->parent->parent);
    if (pE->data) {
        pE->data[0] = r1;
        pE->data[1] = r2;
        pE->data[2] = r3;
        pE->data[3] = r4;
    } else {
        printf("ERROR: unallocated e->data Tag %d\n", eEtag);
    }
    exif_entry_fix(pE);
    exif_entry_unref(pE);
}

} // namespace android
