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

#ifndef __LIBCAMERA_S5PJPEG_ENCODER_H__
#define __LIBCAMERA_S5PJPEG_ENCODER_H__

#include "EncoderInterface.h"
#include "jpeg_api.h"

namespace android {

class S5PJpegEncoder : public EncoderInterface {
public:
    S5PJpegEncoder();
    ~S5PJpegEncoder();

    virtual int setImgFormat(int w, int h, int f, int q = -1);
    virtual int doCompress(uint8_t* inBuff, int inBuffSize);
    virtual int doCompress(EncoderParams* parm, uint8_t* inBuff, int inBuffSize);

    virtual void getOutput(uint8_t** jpegBuff, int* jpegSize);
    virtual int copyOutput(uint8_t* outBuff, int outBuffSize,
                           bool skipSOI = false);

    virtual int setQuality(int q);

private:
    int _fd;

    int _outSize;
    uint8_t* _outBuff;

    struct jpeg_enc_param _params;

    int _reset(void);
    jpeg_stream_format _getOutFormat(int inFmt);
    jpeg_img_quality_level _getQualityEnum(int quality);
};

}

#endif
