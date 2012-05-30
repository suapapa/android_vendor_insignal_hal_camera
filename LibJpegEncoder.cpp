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
#define LOG_TAG "LibJpegEncoder"
#include <utils/Log.h>

#include <linux/videodev2.h>
#include "LibJpegEncoder.h"

namespace android {

struct libjpeg_destination_mgr : jpeg_destination_mgr {
    libjpeg_destination_mgr(uint8_t* input, int size);

    uint8_t* buf;
    int bufsize;
    size_t jpegsize;
};

static void libjpeg_init_destination(j_compress_ptr cinfo)
{
    libjpeg_destination_mgr* dest = (libjpeg_destination_mgr*)cinfo->dest;

    dest->next_output_byte = dest->buf;
    dest->free_in_buffer = dest->bufsize;
    dest->jpegsize = 0;
}

static boolean libjpeg_empty_output_buffer(j_compress_ptr cinfo)
{
    libjpeg_destination_mgr* dest = (libjpeg_destination_mgr*)cinfo->dest;

    dest->next_output_byte = dest->buf;
    dest->free_in_buffer = dest->bufsize;
    return TRUE; // ?
}

static void libjpeg_term_destination(j_compress_ptr cinfo)
{
    libjpeg_destination_mgr* dest = (libjpeg_destination_mgr*)cinfo->dest;
    dest->jpegsize = dest->bufsize - dest->free_in_buffer;
}

static void yuyv_to_yuv(uint8_t* dst, uint32_t* src, int width)
{
    if (!dst || !src) {
        return;
    }

    if (width % 2) {
        return; // not supporting odd widths
    }

    while ((width -= 2) >= 0) {
        uint8_t y0 = (src[0] >> 0) & 0xFF;
        uint8_t u0 = (src[0] >> 8) & 0xFF;
        uint8_t y1 = (src[0] >> 16) & 0xFF;
        uint8_t v0 = (src[0] >> 24) & 0xFF;
        dst[0] = y0;
        dst[1] = u0;
        dst[2] = v0;
        dst[3] = y1;
        dst[4] = u0;
        dst[5] = v0;
        dst += 6;
        src++;
    }
}

libjpeg_destination_mgr::libjpeg_destination_mgr(uint8_t* input, int size)
{
    this->init_destination = libjpeg_init_destination;
    this->empty_output_buffer = libjpeg_empty_output_buffer;
    this->term_destination = libjpeg_term_destination;

    this->buf = input;
    this->bufsize = size;

    jpegsize = 0;
}

LibJpegEncoder::LibJpegEncoder() :
    _outBuffSize(0),
    _outBuff(NULL),
    _jpegSize(0)
{
    LOGV("%s: Inited", __func__);
}

LibJpegEncoder::~LibJpegEncoder()
{
    _deinitOutBuff();

    LOGV("%s: Deinited", __func__);
}

int LibJpegEncoder::doCompress(EncoderParams* params, uint8_t* inBuff, int inBuffSize)
{
    if (inBuff == NULL || inBuffSize == 0) {
        LOGE("%s: null input!", __func__);
        return 0;
    }

    if (!_checkParamsValid(params))
        return 0;

    LOGI("encoding %dx%d with q%d...",
         params->width, params->height, params->quality);

    LOGV("preparing buffer...");
    _initOutBuff(params);
    libjpeg_destination_mgr dest_mgr(_outBuff, _outBuffSize);

    LOGV("initing jpeg structures...");
    jpeg_compress_struct        cinfo;
    jpeg_error_mgr              jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    cinfo.dest = &dest_mgr;
    cinfo.image_width = params->width;
    cinfo.image_height = params->height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_YCbCr;
    cinfo.input_gamma = 1;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, params->quality, TRUE);
    cinfo.dct_method = JDCT_IFAST;

    LOGV("starting compress...");
    jpeg_start_compress(&cinfo, TRUE);
    int out_width = params->width;
    uint8_t* row_tmp = (uint8_t*)malloc(out_width * 3);
    uint8_t* row_src = inBuff;
    int src_stride = out_width * 2;

    while (cinfo.next_scanline < cinfo.image_height) {
        JSAMPROW row[1];    /* pointer to JSAMPLE row[s] */
        yuyv_to_yuv(row_tmp, (uint32_t*)row_src, out_width);
        row[0] = row_tmp;
        jpeg_write_scanlines(&cinfo, row, 1);
        row_src += src_stride;
    }

    LOGV("finishing compress...");
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    if (row_tmp)
        free(row_tmp);

    _jpegSize = dest_mgr.jpegsize;
    LOGV("jpeg compressed. _jpegSize = %d", _jpegSize);

    return _jpegSize;
}

void LibJpegEncoder::getOutput(uint8_t** jpegBuff, int* jpegSize)
{
    LOGE_IF(_outBuff == NULL || _jpegSize == 0,
            "%s: seems no jpeg readied! _outBuff = %p, _jpegSize = %d",
            __func__, _outBuff, _jpegSize);

    if (jpegBuff != NULL)
        *jpegBuff = _outBuff;

    if (jpegSize != NULL)
        *jpegSize = _jpegSize;
}

int LibJpegEncoder::copyOutput(uint8_t* outBuff, int outBuffSize, bool skipSOI)
{
    if (_outBuff == NULL || _jpegSize == 0) {
        LOGE("%s: seems JPEG not readied. yet?", __func__);
        return 0;
    }

    if (outBuffSize < _jpegSize) {
        LOGE("%s: too small buffer for JPEG output. outBuffSize = %d, _jpegSize = %d",
             __func__, outBuffSize, _jpegSize);
        return 0;
    }

    memcpy(outBuff, _outBuff, _jpegSize);
    return _jpegSize;
}

int LibJpegEncoder::setQuality(int q)
{
    LOGE("%s: deprecated!!", __func__);

    return 0;
}

bool LibJpegEncoder::_checkParamsValid(EncoderParams* params)
{
    if (params->width < 2 || params->height < 2) {
        LOGE("%s: Not support too small input %dx%d!", __func__,
             params->width, params->height);
        return false;
    }

    if (params->quality < 1) {
        LOGE("%s: Not support too small quality, %d!", __func__,
             params->quality);
        return false;
    }

    if (params->format != V4L2_PIX_FMT_YUYV) {
        LOGE("%s: Not supported format, %d", __func__,
             params->format);
        return false;
    }

    return true;
}

void LibJpegEncoder::_deinitOutBuff(void)
{
    if (_outBuff) {
        LOGV("%s: freeing _outBuff, %dbytes", __func__, _outBuffSize);
        free(_outBuff);
        _outBuff = NULL;
    }
    _outBuffSize = 0;
    _jpegSize = 0;
}

void LibJpegEncoder::_initOutBuff(EncoderParams* params)
{
    int buffSize = params->width * params->height * 2;
    if (buffSize == 0) {
        LOGE("%s: input size not determined. yet?", __func__);
        return;
    }

    if (_outBuff && _outBuffSize < buffSize) {
        LOGV("%s: will realloc _outBuff for %dbytes", __func__, buffSize);
        _deinitOutBuff();
    }

    if (_outBuff == NULL) {
        LOGV("%s: alloc %dbytes for jpeg out", __func__, buffSize);
        _outBuff = (uint8_t*)malloc(buffSize);
        _outBuffSize = buffSize;
        _jpegSize = 0;
    }
}

int LibJpegEncoder::setImgFormat(int w, int h, int f, int q)
{
    LOGE("%s: deprecated!!", __func__);

    return 0;
}

int LibJpegEncoder::doCompress(uint8_t* inBuff, int inBuffSize)
{
    LOGE("%s: deprecated!!", __func__);

    return 0;
}

}
