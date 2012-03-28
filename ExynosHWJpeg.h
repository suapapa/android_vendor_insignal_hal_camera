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

#ifndef __ANDROID_HARDWARE_LIBCAMERA_EXYNOS_HW_JPEG_H__
#define __ANDROID_HARDWARE_LIBCAMERA_EXYNOS_HW_JPEG_H__

#include "jpeg_api.h"
#include "JpegInterface.h"

namespace android {

class ExynosHWJpeg : public JpegInterface
{
public:
    ExynosHWJpeg();
    ~ExynosHWJpeg();

    virtual int setImgFormat(int w, int h, int f);
    virtual int doCompress(unsigned char *inBuff, int inBuffSize);
    virtual int copyOutput(unsigned char *outBuff, int outBuffSize);

    virtual int setQuality(int q);
    virtual int setThumbSize(int w, int h);
    virtual int setThumbQuality(int q);

    virtual int setRotate(int r);
    virtual int setGps(double lat, double lon, unsigned int ts, short alt);

private:
    int _fd;

    int _outSize;
    unsigned char *_outBuff;

    struct jpeg_enc_param _params;

    int _reset(void);
    jpeg_stream_format _getOutFormat(int inFmt);
    jpeg_img_quality_level _getQualityEnum(int quality);
};

}

#endif
