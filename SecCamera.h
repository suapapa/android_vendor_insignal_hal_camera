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

#ifndef __ANDROID_HARDWARE_LIBCAMERA_SEC_CAMERA_H__
#define __ANDROID_HARDWARE_LIBCAMERA_SEC_CAMERA_H__

#include "SecV4L2Adapter.h"
#include "EncoderInterface.h"
#include "TaggerInterface.h"

#define DUAL_PORT_RECORDING

namespace android {

class SecCamera {
public:
    SecCamera(int ch);
    ~SecCamera();

    int                 flagCreate(void) const;

    int                 startPreview(void);
    int                 stopPreview(void);
    void                pausePreview();
    int                 dqPreviewBuffer(int* index, unsigned int* addrY, unsigned int* addrC);
    void                qPreviewBuffer(int index);

#ifdef DUAL_PORT_RECORDING
    int                 startRecord(void);
    int                 stopRecord(void);
    int                 dqRecordBuffer(int* index, unsigned int* addrY, unsigned int* addrC);
    void                qRecordBuffer(int index);
#endif

    int                 setPreviewFormat(int width, int height, const char* strPixfmt);
    unsigned int        getPreviewFrameSize(void);
    void                getPreviewFrameSize(int* width, int* height, int* frameSize);

    int                 setSnapshotFormat(int width, int height, const char* strPixfmt);

    int                 startAutoFocus(void);
    int                 abortAutoFocus(void);
    bool                getAutoFocusResult(void);

    int                 setSceneMode(const char* strScenemode);
    int                 setWhiteBalance(const char* strWhitebalance);
    int                 setEffect(const char* strEffect);
    int                 setFlashMode(const char* strFlashMode);
    int                 setBrightness(int brightness);
    int                 setFocusMode(const char* strFocusMode);

    int                 setRotate(int angle);
    void                setFrameRate(int frame_rate);
    int                 setZoom(int zoom);

    int                 setPictureQuality(int q);
    int                 setThumbQuality(int q);
    int                 setJpegThumbnailSize(int width, int height);

    int                 startSnapshot(size_t* captureSize);
    int                 getSnapshot(int xth = 0);
    int                 getRawSnapshot(uint8_t* buffer, size_t size);
    int                 getJpegSnapshot(uint8_t* buffer, size_t size);
    int                 endSnapshot(void);

    int                 compress2Jpeg(unsigned char* rawData, size_t rawSize);
    int                 getJpeg(unsigned char* outBuff, int buffSize);

    int                 setGpsInfo(const char* strLatitude,
                                   const char* strLongitude,
                                   const char* strAltitude,
                                   const char* strTimestamp = NULL,
                                   const char* strProcessMethod = NULL,
                                   int nALtitudeRef = 0,
                                   const char* strMapDatum = NULL,
                                   const char* strGpsVersion = NULL);

    int                 getFd(void);

private:
    struct sec_cam_parm _v4l2Params;

    bool                _isInited;

    int                 _previewWidth;
    int                 _previewHeight;
    int                 _previewPixfmt;
    unsigned int        _previewFrameSize;

    int                 _snapshotWidth;
    int                 _snapshotHeight;
    int                 _snapshotPixfmt;

    bool                _isPreviewOn;
    bool                _isRecordOn;

    SecV4L2Adapter*     _v4l2Cam;
    SecV4L2Adapter*     _v4l2Rec;

    EncoderInterface*   _encoder;
    EncoderParams       _pictureParams;
    EncoderParams       _thumbParams;

    void                _release(void);
    void                _initParms(void);
    int                 _getPhyAddr(int index, unsigned int* addrY, unsigned int* addrC);

    TaggerInterface*    _tagger;
    TaggerParams        _exifParams;
    void                _initExifParams(void);

    int                 _convertGPSCoord(double coord, int* deg, int* min, int* sec);
    int                 _scaleDownYuv422(uint8_t* srcBuf, uint32_t srcWidth, uint32_t srcHight,
                                         uint8_t* dstBuf, uint32_t dstWidth, uint32_t dstHight);
    int                 _createThumbnail(uint8_t* rawData, int rawSize);
};

}; // namespace android

#endif // ANDROID_HARDWARE_CAMERA_SEC_H
