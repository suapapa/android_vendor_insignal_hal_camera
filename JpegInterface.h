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

#ifndef __ANDROID_HARDWARE_LIBCAMERA_JPEG_INTERFACE_H__
#define __ANDROID_HARDWARE_LIBCAMERA_JPEG_INTERFACE_H__

namespace android {

class JpegInterface {
public:
    virtual ~JpegInterface() { }
    virtual int setImgFormat(int w, int h, int f) = 0;
    virtual int doCompress(unsigned char *inBuff, int inBuffSize) = 0;
    virtual int copyOutput(unsigned char *outBuff, int outBuffSize) = 0;

    virtual int setQuality(int q) = 0;
    virtual int setThumbSize(int w, int h) = 0;
    virtual int setThumbQuality(int q) = 0;

    // For informastions of EXIF
    virtual int setRotate(int r) = 0;
    virtual int setGps(double lat, double lon, unsigned int ts, short alt) = 0;
};

}

#endif
