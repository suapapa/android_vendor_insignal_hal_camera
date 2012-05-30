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

#ifndef __LIBCAMERA_LIBJPEG_ENCODER_H__
#define __LIBCAMERA_LIBJPEG_ENCODER_H__

#include "EncoderInterface.h"

extern "C" {
#include "jpeglib.h"
#include "jerror.h"
}

namespace android {

class LibJpegEncoder : public EncoderInterface {
public:
    LibJpegEncoder();
    ~LibJpegEncoder();

    virtual int setImgFormat(int w, int h, int f, int q = -1);
    virtual int doCompress(uint8_t* inBuff, int inBuffSize);
    virtual int doCompress(EncoderParams* params, uint8_t* inBuff, int inBuffSize);

    virtual void getOutput(uint8_t** jpegBuff, int* jpegSize);
    virtual int copyOutput(uint8_t* outBuff, int outBuffSize,
                           bool skipSOI = false);

    virtual int setQuality(int q);

private:
    int		_outBuffSize;
    uint8_t*	_outBuff;
    int		_jpegSize;

    bool _checkParamsValid(EncoderParams* params);
    void _initOutBuff(EncoderParams* params);
    void _deinitOutBuff(void);
};

}

#endif
