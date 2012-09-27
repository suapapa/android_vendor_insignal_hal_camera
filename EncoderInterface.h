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

#ifndef __ANDROID_HARDWARE_LIBCAMERA_ENCODER_INTERFACE_H__
#define __ANDROID_HARDWARE_LIBCAMERA_ENCODER_INTERFACE_H__

namespace android {

struct EncoderParams {
    int width;
    int height;
    int format;
    int quality;
};

class EncoderInterface {
public:
    virtual ~EncoderInterface() { }
    virtual int setImgFormat(int w, int h, int f, int q = -1) = 0;

    virtual int doCompress(EncoderParams* parm, uint8_t* inBuff, int inBuffSize) = 0;
    virtual int doCompress(uint8_t* inBuff, int inBuffSize) = 0;

    virtual void getOutput(uint8_t** jpegBuff, int* jpegSize) = 0;
    virtual int copyOutput(uint8_t* outBuff, int outBuffSize,
                           bool skipSOI = false) = 0;

    virtual int setQuality(int q) = 0;
};

}

#endif
