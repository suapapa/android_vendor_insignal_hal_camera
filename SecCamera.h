/*
 *
 * Copyright 2008, The Android Open Source Project
 * Copyright 2010, Samsung Electronics Co. LTD
 * Copyright (C) 2012 Insignal Co, Ltd.
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
#include "JpegInterface.h"

namespace android
{

#define DUAL_PORT_RECORDING //Define this if 2 fimc ports are needed for recording.

#if defined(LOG_NDEBUG) && LOG_NDEBUG == 0
#define LOG_CAMERA LOGD
#define LOG_CAMERA_PREVIEW LOGD

#define LOG_TIME_START(n) \
    struct timeval time_start_##n, time_stop_##n; \
    unsigned long log_time_##n = 0; \
    gettimeofday(&time_start_##n, NULL)

#define LOG_TIME_END(n) \
    gettimeofday(&time_stop_##n, NULL); \
    log_time_##n = measure_time(&time_start_##n, &time_stop_##n)

#define LOG_TIME(n) \
    log_time_##n

#else
#define LOG_CAMERA(...)
#define LOG_CAMERA_PREVIEW(...)
#define LOG_TIME_START(n)
#define LOG_TIME_END(n)
#define LOG_TIME(n)
#endif


#define CAMERA_DEV_NAME         "/dev/video0"

#ifdef DUAL_PORT_RECORDING
#define CAMERA_DEV_NAME2        "/dev/video2"
#endif


class SecCamera
{
public:
    SecCamera(const char *camPath, const char *recPath, int ch);
    ~SecCamera();

    int                 flagCreate(void) const;

    int                 startPreview(void);
    int                 stopPreview(void);
    void                pausePreview();
#ifdef DUAL_PORT_RECORDING
    int                 startRecord(void);
    int                 stopRecord(void);
    int                 getRecordBuffer(int *index, unsigned int *addrY, unsigned int *addrC);
    void                releaseRecordFrame(int i);
#endif

    int                 getPreviewBuffer(int *index, unsigned int *addrY, unsigned int *addrC);

    int                 setPreviewFormat(int width, int height, const char *strPixfmt);
    unsigned int        getPreviewFrameSize(void);

    int                 setSnapshotFormat(int width, int height, const char *strPixfmt);
    unsigned int        getSnapshotFrameSize(void);

    int                 startAutoFocus(void);
    int                 abortAutoFocus(void);
    bool                getAutoFocusResult(void);

    int                 setSceneMode(const char *strScenemode);
    int                 setWhiteBalance(const char *strWhitebalance);
    int                 setEffect(const char *strEffect);
    int                 setFlashMode(const char *strFlashMode);
    int                 setBrightness(int brightness);
    int                 setFocusMode(const char *strFocusMode);

    int                 setRotate(int angle);
    void                setFrameRate(int frame_rate);
    int                 setZoom(int zoom);


    int                 startSnapshot(void);
    int                 getSnapshot(int xth = 0);
    int                 getRawSnapshot(unsigned char *buffer, unsigned int size);
    int                 getJpegSnapshot(unsigned char *buffer, unsigned int size);
    int                 endSnapshot(void);

    int                 setJpegThumbnailSize(int width, int height);
    int                 setJpegQuality(int quality);
    int                 setGpsInfo(double latitude, double longitude,
                                   unsigned int timestamp, int altitude);


    int                 getFd(void);

private:
    struct sec_cam_parm _parms;

    bool                _isInited;

    int                 _previewWidth;
    int                 _previewHeight;
    int                 _previewPixfmt;
    unsigned int        _previewFrameSize;

    int                 _snapshotWidth;
    int                 _snapshotHeight;
    int                 _snapshotPixfmt;
    unsigned int        _snapshotFrameSize;

    bool                _isPreviewOn;

    struct v4l2Buffer   _previewBufs[MAX_BUFFERS];
    struct v4l2Buffer   _captureBuf;

#ifdef DUAL_PORT_RECORDING
    bool                _isRecordOn;
    struct v4l2Buffer   _recordBufs[MAX_BUFFERS];
#endif

    SecV4L2Adapter     *_v4l2Cam;
    SecV4L2Adapter     *_v4l2Rec;

    JpegInterface      *_jpeg;

    void                _release(void);
    void                _initParms(void);
    int                 _getPhyAddr(int index, unsigned int *addrY, unsigned int *addrC);
};

extern unsigned long measure_time(struct timeval *start, struct timeval *stop);

}; // namespace android

#endif // ANDROID_HARDWARE_CAMERA_SEC_H
