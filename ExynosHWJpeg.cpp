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
#define LOG_TAG "ExynosHWJpeg"
#include <utils/Log.h>

#include <linux/videodev2.h>
#include <videodev2_samsung.h>

#include "ExynosHWJpeg.h"

namespace android {

ExynosHWJpeg::ExynosHWJpeg() :
        _fd             (0),
	_outBuff	(NULL)
{
    memset(&_params, 0, sizeof(_params));
    LOGV("%s: Inited", __func__);

    //_reset();
}

ExynosHWJpeg::~ExynosHWJpeg()
{
    if(_fd > 0) {
        LOGE_IF(api_jpeg_encode_deinit(_fd) != JPEG_OK,
                "ERR(%s):Fail on api_jpeg_encode_deinit\n", __func__);
        _fd = 0;
    }
    LOGV("%s: Deinited", __func__);
}

int ExynosHWJpeg::setImgFormat(int w, int h, int f)
{
    jpeg_stream_format sf = _getOutFormat(f);

    if ((unsigned int)w == _params.width
            && (unsigned int)h == _params.height
            && sf == _params.out_fmt) {
        LOGV("%s: same as before. do nothing!", __func__);
        return 0;
    }

    _reset();

    _params.width = w;
    _params.height = h;
    _params.in_fmt = YUV_422; // YCBCR only
    _params.out_fmt = _getOutFormat(f);
    _params.quality = QUALITY_LEVEL_4;

    return 0;
}

int ExynosHWJpeg::doCompress(unsigned char *inBuff, int inBuffSize)
{
    //_reset();

    if (_fd == 0) {
        LOGE("%s: Jpeg device not opened!", __func__);
        return -1;
    }

    // Get input buffer
    unsigned char *buff = (unsigned char *)api_jpeg_get_encode_in_buf(_fd, inBuffSize);
    if (buff == NULL) {
        LOGE("%s: Failed to get input buffer!", __func__);
        return -1;
    }
    memcpy(buff, inBuff, inBuffSize);

    // Get output buffer
    _outBuff = (unsigned char *)api_jpeg_get_encode_out_buf(_fd);
    if (_outBuff == NULL) {
        LOGE("%s: Failed to get input buffer!", __func__);
        return -1;
    }

    // Set encode parameters
    api_jpeg_set_encode_param(&_params);

    // Finally do encode!
    enum jpeg_ret_type ret = api_jpeg_encode_exe(_fd, &_params);
    if (ret != JPEG_ENCODE_OK) {
        LOGE("%s: Failed encode to JPEG!", __func__);
        return -1;
    }

    LOGI("JPEG compress done! jpeg size is %d", _params.size);

    return _params.size;
}

int ExynosHWJpeg::copyOutput(unsigned char *outBuff, int outBuffSize)
{
    int jpegSize = _params.size;

    if (0 >= jpegSize) {
        LOGE("%s: seems no jpeg image readied!", __func__);
        return -1;
    }

    LOGW_IF(outBuffSize < jpegSize,
            "%s: outBuffsize(%d) and outSize(%d) not same!",
            __func__, outBuffSize, jpegSize);

    memcpy(outBuff, _outBuff, jpegSize);

    return jpegSize;
}

int ExynosHWJpeg::setQuality(int q)
{
    _params.quality = _getQualityEnum(q);

    return 0;
}

int ExynosHWJpeg::setThumbSize(int w, int h)
{
    // TODO: no code here!

    return 0;
}

int ExynosHWJpeg::setThumbQuality(int q)
{
    // TODO: no code here!

    return 0;
}

int ExynosHWJpeg::setRotate(int r)
{
    // TODO: no exif code here!
#if 0
    _rotation = r;
#endif

    return 0;
}

int ExynosHWJpeg::setGps(double latitude, double longitude,
            unsigned int timestamp, short altitude)
{
    // TODO: no exif code here!
#if 0
    _gps_latitude  = latitude;
    _gps_longitude = longitude;
    _gps_timestamp = timestamp;
    _gps_altitude  = altitude;
#endif

    return 0;
}

int ExynosHWJpeg::_reset(void)
{
    int ret;
    if(_fd > 0) {
        ret = api_jpeg_encode_deinit(_fd);
        if(ret != JPEG_OK) {
            LOGE("ERR(%s):Fail on api_jpeg_encode_deinit\n", __func__);
            _fd = 0;
            return -1;
        }
    }
    _fd = api_jpeg_encode_init();

    return _fd > 0 ? 0 : -1;
}

jpeg_stream_format ExynosHWJpeg::_getOutFormat(int inFmt)
{
    jpeg_stream_format outFmt = JPEG_RESERVED;

    switch(inFmt) {
        case V4L2_PIX_FMT_NV12:
        case V4L2_PIX_FMT_NV21:
        case V4L2_PIX_FMT_NV12T:
        case V4L2_PIX_FMT_YUV420:
            outFmt = JPEG_420;
            break;

        case V4L2_PIX_FMT_YUYV:
        case V4L2_PIX_FMT_UYVY:
        case V4L2_PIX_FMT_YUV422P:
            outFmt = JPEG_422;
            break;

        defalut:
            LOGE("%s: ExynosHWJpeg not support input format, %d", __func__, inFmt);
            break;
    }

    LOGV("%s: inFmt=%d, outFmt=%d", __func__, inFmt, outFmt);

    return outFmt;
}

jpeg_img_quality_level ExynosHWJpeg::_getQualityEnum(int quality)
{
    jpeg_img_quality_level enumQuality = QUALITY_LEVEL_4;

    if (quality > 100 || quality < 0) {
        LOGE("%s: quality should value in 0~100. given %d.", __func__, quality);
        enumQuality = QUALITY_LEVEL_1;
    } else if (85 >= quality) {
        enumQuality = QUALITY_LEVEL_1;
    } else if (75 >= quality) {
        enumQuality = QUALITY_LEVEL_2;
    } else if (65 >= quality) {
        enumQuality = QUALITY_LEVEL_3;
    }

    LOGV("%s: quality=%d, enumQuality=%d", __func__, quality, enumQuality);

    return enumQuality;
}

}
