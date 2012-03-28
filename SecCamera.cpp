/*
 * Copyright@ Samsung Electronics Co. LTD
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
#define LOG_TAG "SecCamera"
#include <utils/Log.h>

#include "SecCamera.h"
#include "ExynosHWJpeg.h"

using namespace android;

#define CHECK(T)                                \
    if (!(T)) {                                 \
        LOGE("%s+%d: Expected %s but failed!!", \
                __func__, __LINE__, #T);        \
    }

namespace android {

// ======================================================================
// Camera controls
static struct timeval time_start;
static struct timeval time_stop;

unsigned long measure_time(struct timeval *start, struct timeval *stop)
{
    unsigned long sec, usec, time;

    sec = stop->tv_sec - start->tv_sec;

    if (stop->tv_usec >= start->tv_usec) {
        usec = stop->tv_usec - start->tv_usec;
    } else {
        usec = stop->tv_usec + 1000000 - start->tv_usec;
        sec--;
    }

    time = (sec * 1000000) + usec;

    return time;
}

void SecCamera::releaseRecordFrame(int i)
{
    LOGV("releaseframe : (%d)",i);

#ifdef DUAL_PORT_RECORDING
    int ret = _v4l2Rec->qbuf(i);
    //CHECK(ret == 0);
#endif

}

// ======================================================================
// Constructor & Destructor

SecCamera::SecCamera(const char *camPath, const char *recPath, int ch) :
        _isInited               (false),
        _previewWidth           (0),
        _previewHeight          (0),
        _previewPixfmt          (-1),
        _previewFrameSize       (0),
        _snapshotWidth          (0),
        _snapshotHeight         (0),
        _snapshotPixfmt         (-1),
        _snapshotFrameSize      (0),
        _isPreviewOn            (false),
        _isRecordOn             (false),
        _v4l2Cam		(NULL),
        _v4l2Rec                (NULL),
        _jpeg                   (NULL)
{
    LOGV("%s()", __func__);

    _v4l2Cam = new SecV4L2Adapter(camPath, ch);
#ifdef DUAL_PORT_RECORDING
    _v4l2Rec = new SecV4L2Adapter(recPath, ch);
#endif
    _jpeg = new ExynosHWJpeg();

    if (_v4l2Cam->getFd() == 0 || _v4l2Rec->getFd() == 0) {
        _release();
        LOGE("!! %s: Failed to create SecCamera !!", __func__);
        return;
    }

    _initParms();

    _isInited = true;
}

SecCamera::~SecCamera()
{
    LOGV("%s()", __func__);

    if(!_isInited) {
        LOGV("%s: Can't destroy! SecCamera not inited!", __func__);
        return;
    }

    LOGE_IF(this->stopPreview() < 0,
            "ERR(%s):Fail on stopPreview()\n", __func__);

#ifdef DUAL_PORT_RECORDING
    stopRecord();
#endif

    _release();

    _isInited = false;
}

void SecCamera::_release(void)
{
    if(_jpeg)
        delete _jpeg;

    if(_v4l2Cam)
        delete _v4l2Cam;

#ifdef DUAL_PORT_RECORDING
    if(_v4l2Rec)
        delete _v4l2Rec;
#endif

}

int SecCamera::flagCreate(void) const
{
    LOGV("%s() : %d", __func__, _isInited);

    return _isInited == true ? 1 : 0;
}

int SecCamera::getFd(void)
{
    return _v4l2Cam->getFd();
}

// ======================================================================
// Preview

int SecCamera::startPreview(void)
{
    // aleady started
    if(_isPreviewOn == true) {
        LOGE("ERR(%s):Preview was already started\n", __func__);
        return 0;
    }

    int ret = 0;

    ret = _v4l2Cam->setFmt(_previewWidth, _previewHeight, _previewPixfmt, 0);
    CHECK(ret == 0);

    _v4l2Cam->initBufs(_previewBufs, _previewWidth, _previewHeight, _previewPixfmt);
    ret = _v4l2Cam->reqBufs(V4L2_BUF_TYPE_VIDEO_CAPTURE, MAX_BUFFERS);
    CHECK(ret == MAX_BUFFERS);

    ret = _v4l2Cam->queryBufs(_previewBufs, V4L2_BUF_TYPE_VIDEO_CAPTURE, MAX_BUFFERS);
    CHECK(ret == 0);

    /* start with all buffers in queue */
    for (int i = 0; i < MAX_BUFFERS; i++) {
        ret = _v4l2Cam->qbuf(i);
        CHECK(ret == 0);
    }

    ret = _v4l2Cam->startStream(true);
    CHECK(ret == 0);

    _isPreviewOn = true;

    ret = _v4l2Cam->setParm(&_parms);
    CHECK(ret == 0);

    ret = _v4l2Cam->waitFrame();
    CHECK(ret > 0);

    int index = _v4l2Cam->dqbuf();
    ret = _v4l2Cam->qbuf(index);
    CHECK(ret == 0);

    return 0;
}

int SecCamera::stopPreview(void)
{
    LOGV("++%s() \n", __func__);

    if(_isPreviewOn == false)
        return 0;

    _v4l2Cam->closeBufs(_previewBufs);

    int ret = _v4l2Cam->startStream(false);
    CHECK(ret == 0);

    _isPreviewOn = false; //Kamat check

    LOGV("--%s() \n", __func__);
    return 0;
}

//Recording
#ifdef DUAL_PORT_RECORDING
int SecCamera::startRecord(void)
{
    LOGI("++%s() \n", __func__);
    int ret = 0;
    // aleady started
    if(_isRecordOn == true) {
        LOGE("ERR(%s):Preview was already started\n", __func__);
        return 0;
    }

    int color_format = V4L2_PIX_FMT_NV12;

    ret = _v4l2Rec->setFmt(_previewWidth, _previewHeight, color_format, 0);
    CHECK(ret == 0);

    _v4l2Rec->initBufs(_recordBufs, _previewWidth, _previewHeight, _previewPixfmt);
    ret = _v4l2Rec->reqBufs(V4L2_BUF_TYPE_VIDEO_CAPTURE, MAX_BUFFERS);
    CHECK(ret == 0);

    ret = _v4l2Rec->queryBufs(_recordBufs, V4L2_BUF_TYPE_VIDEO_CAPTURE, MAX_BUFFERS);
    CHECK(ret == 0);

    /* start with all buffers in queue */
    for (int i = 0; i < MAX_BUFFERS; i++) {
        ret = _v4l2Rec->qbuf(i);
        CHECK(ret == 0);
    }

    ret = _v4l2Rec->startStream(true);
    CHECK(ret == 0);

    // It is a delay for a new frame, not to show the previous bigger ugly picture frame.
    ret = _v4l2Rec->waitFrame();
    CHECK(ret > 0);

    int index = _v4l2Rec->dqbuf();
    ret = _v4l2Rec->qbuf(index);
    CHECK(ret == 0);

    _isRecordOn = true;
    LOGI("Recording started!");

    return 0;
}

int SecCamera::stopRecord(void)
{
    LOGV("%s :", __func__);

    if(_isRecordOn == false)
        return 0;

    _v4l2Rec->closeBufs(_recordBufs);

    int ret = _v4l2Rec->startStream(false);
    CHECK(ret == 0);

    _isRecordOn = false;
    LOGI("Recording stopped!");

    return 0;
}

int SecCamera::getRecordBuffer(int *index, unsigned int *addrY, unsigned int *addrC)
{
#ifdef PERFORMANCE
    LOG_TIME_START(0);
    _v4l2Rec->waitFrame();
    LOG_TIME_END(0);
    LOG_CAMERA("fimc_poll interval: %lu us", LOG_TIME(0));

    LOG_TIME_START(1);
    *index = _v4l2Rec->dqbuf();
    LOG_TIME_END(1);
    LOG_CAMERA("fimc_dqbuf interval: %lu us", LOG_TIME(1));
#else
    _v4l2Rec->waitFrame();
    *index = _v4l2Rec->dqbuf();
#endif
    if(!(0 <= *index && *index < MAX_BUFFERS)) {
        LOGE("ERR(%s):wrong index = %d\n", __func__, *index);
        return -1;
    }

    _v4l2Rec->getAddr(*index, addrY, addrC);

    return 0;
}
#endif //DUAL_PORT_RECORDING

int SecCamera::_getPhyAddr(int index, unsigned int *addrY, unsigned int *addrC)
{
    int ret = _v4l2Cam->qbuf(index);
    CHECK(ret == 0);

    _v4l2Cam->getAddr(index, addrY, addrC);

    return 0;
}

int SecCamera::getPreviewBuffer(int *index, unsigned int *addrY, unsigned int *addrC)
{
#ifdef PERFORMANCE
    LOG_TIME_START(0);
    //_v4l2Cam->waitFrame();
    LOG_TIME_END(0);
    LOG_CAMERA("fimc_poll interval: %lu us", LOG_TIME(0));

    LOG_TIME_START(1);
    *index = _v4l2Cam->dqbuf();
    LOG_TIME_END(1);
    LOG_CAMERA("fimc_dqbuf interval: %lu us", LOG_TIME(1));
#else
    *index = _v4l2Cam->blk_dqbuf();
#endif
    if(!(0 <= *index && *index < MAX_BUFFERS)) {
        LOGE("ERR(%s):wrong index = %d\n", __func__, *index);
        return -1;
    }

    return _getPhyAddr(*index, addrY, addrC);
}

void SecCamera::pausePreview()
{
    _v4l2Cam->setCtrl(V4L2_CID_STREAM_PAUSE, 0);
}

int SecCamera::setPreviewFormat(int width, int height, const char *strPixfmt)
{
    _previewWidth = width;
    _previewHeight = height;
    _previewPixfmt = _v4l2Cam->nPixfmt(strPixfmt);
    _previewFrameSize = _v4l2Cam->frameSize(_previewWidth, _previewHeight,
            _previewPixfmt);

    LOGI("previewFormat=%dx%d(%s), frameSize=%d",
            width, height, strPixfmt, _previewFrameSize);

    return _previewFrameSize > 0 ? 0 : -1;
}

unsigned int SecCamera::getPreviewFrameSize(void)
{
    return _previewFrameSize;
}

// ======================================================================
// Snapshot
int SecCamera::endSnapshot(void)
{
    return _v4l2Cam->closeBuf(&_captureBuf);
}

int SecCamera::startSnapshot(void)
{
    LOG_TIME_START(0);
    stopPreview();
    LOG_TIME_END(0);

    int ret = _v4l2Cam->setFmt(_snapshotWidth, _snapshotHeight, _snapshotPixfmt, 1);
    CHECK(ret == 0);

    LOG_TIME_START(1); // prepare
    _v4l2Cam->initBuf(&_captureBuf, _snapshotWidth, _snapshotHeight, _snapshotPixfmt);
    _v4l2Cam->reqBufs(V4L2_BUF_TYPE_VIDEO_CAPTURE, 1);
    _v4l2Cam->queryBuf(&_captureBuf, V4L2_BUF_TYPE_VIDEO_CAPTURE);
    _v4l2Cam->qbuf(0);
    _v4l2Cam->startStream(true);
    LOG_TIME_END(1);

    LOG_CAMERA("%s: stopPreview(%lu), prepare(%lu) us",
            __func__, LOG_TIME(0), LOG_TIME(1));

    return 0;
}
    // ------------------------------------------------------------------

int SecCamera::getSnapshot(int xth)
{

    int index;
    int skipFirstNFrames = xth;

    LOG_TIME_START(0); // skip frames
    while (skipFirstNFrames) {
        LOGV("skipFrames %d", skipFirstNFrames);
        _v4l2Cam->waitFrame();
        index = _v4l2Cam->dqbuf();
        _v4l2Cam->qbuf(index);
        skipFirstNFrames--;
    }

    _v4l2Cam->waitFrame();
    index = _v4l2Cam->dqbuf();
    LOG_TIME_END(0);

    LOG_TIME_START(1);
    _v4l2Cam->startStream(false);
    LOG_TIME_END(1);

    LOG_CAMERA("%s: get frame after skip %d(%lu), stopStream(%lu)",
            __func__, xth, LOG_TIME(0), LOG_TIME(1));

    return 0;
}

int SecCamera::getRawSnapshot(unsigned char *buffer, unsigned int size)
{
    if (buffer == NULL) {
        LOGE("%s: got null pointer!", __func__);
        return -1;
    }

    if (size < _captureBuf.length) {
        LOGE("%s: buffer size, %d is too small! for snapshot size, %d",
                __func__, size, _captureBuf.length);
    } else {
        size = _captureBuf.length;
    }

    memcpy(buffer, _captureBuf.start, size);

    return 0;
}

int SecCamera::setSnapshotFormat(int width, int height, const char *strPixfmt)
{
    _snapshotWidth  = width;
    _snapshotHeight = height;
    _snapshotPixfmt = _v4l2Cam->nPixfmt(strPixfmt);

    /* TODO: send snapshot format to dirver which have context change */

    _snapshotFrameSize = _v4l2Cam->frameSize(_snapshotWidth, _snapshotHeight,
            _snapshotPixfmt);

    int ret = _jpeg->setImgFormat(_snapshotWidth, _snapshotHeight,
		    _snapshotPixfmt);
    CHECK(ret == 0);

    LOGI("snapshotFormat=%dx%d(%s), frameSize=%d",
            width, height, strPixfmt, _snapshotFrameSize);

    return _snapshotFrameSize > 0 ? 0 : -1;
}

unsigned int SecCamera::getSnapshotFrameSize(void)
{
    return _snapshotFrameSize;
}

// ======================================================================
// Auto Focus

int SecCamera::startAutoFocus(void)
{
    int ret = _v4l2Cam->setCtrl(V4L2_CID_CAMERA_SET_AUTO_FOCUS,
            AUTO_FOCUS_ON);

    LOGE_IF(0 > ret, "%s:Failed to start auto focus!! ret=%d",
            __func__, ret);

    return ret;
}

int SecCamera::abortAutoFocus(void)
{
    int ret = _v4l2Cam->setCtrl(V4L2_CID_CAMERA_SET_AUTO_FOCUS,
            AUTO_FOCUS_OFF);

    LOGE_IF(0 > ret, "%s:Failed to abort auto focus!! ret=%d",
            __func__, ret);

    return ret;
}

bool SecCamera::getAutoFocusResult(void)
{
    bool afDone = false;
    int ret = _v4l2Cam->getCtrl(V4L2_CID_CAMERA_AUTO_FOCUS_RESULT);
    switch(ret) {
    case 0x01:
        LOGV("%s: af success.", __func__);
        afDone = true;
        break;
    case 0x02: // AF cancelled!
        /* CAMERA_MSG_FOCUS only takes a bool.  true for
         * finished and false for failure.  cancel is still
         * considered a true result.
         */
        LOGI("%s: AF cancelled! but finished anyway.", __func__);
        afDone = true;
        break;
    }

    LOGW_IF(!afDone, "%s: AF failed! ret = %d", __func__, ret);

    return afDone;
}

// ======================================================================
// Settings

int SecCamera::setRotate(int angle)
{
    int rotate = angle % 360;

    if (angle >= 360 || 0 > angle)
        LOGW("%s: input angle, %d is truncated to %d!", __func__, angle, rotate);

    _jpeg->setRotate(angle);

    return 0;
}

int SecCamera::setSceneMode(const char *strSceneMode)
{
    _parms.scene_mode = _v4l2Cam->enumSceneMode(strSceneMode);

    if(!_isPreviewOn)
        return 0;

    int ret = _v4l2Cam->setCtrl(V4L2_CID_CAMERA_SCENE_MODE,
            _parms.scene_mode);

    LOGE_IF(0 > ret, "%s:Failed to set scene-mode, %s!! ret=%d",
            __func__, strSceneMode, ret);

    return 0 > ret ? -1 : 0;
}

int SecCamera::setWhiteBalance(const char *strWhiteBalance)
{
    _parms.white_balance = _v4l2Cam->enumWB(strWhiteBalance);

    if(!_isPreviewOn)
        return 0;

    int ret = _v4l2Cam->setCtrl(V4L2_CID_CAMERA_WHITE_BALANCE,
            _parms.white_balance);

    LOGE_IF(0 > ret, "%s:Failed to set white-balance, %s!! ret=%d",
            __func__, strWhiteBalance, ret);

    return 0 > ret ? -1 : 0;
}

int SecCamera::setEffect(const char *strEffect)
{
    _parms.effects = _v4l2Cam->enumEffect(strEffect);

    if(!_isPreviewOn)
        return 0;

    int ret = _v4l2Cam->setCtrl(V4L2_CID_CAMERA_EFFECT,
            _parms.effects);

    LOGE_IF(0 > ret, "%s:Failed to set effects, %s!! ret=%d",
            __func__, strEffect, ret);

    return 0 > ret ? -1 : 0;
}

int SecCamera::setFlashMode(const char *strFlashMode)
{
    _parms.flash_mode = _v4l2Cam->enumFlashMode(strFlashMode);

    LOGV("setting flash-mode set to %s...", strFlashMode);

    switch(_parms.flash_mode) {
    case FLASH_MODE_TORCH:
        return 0;

    case FLASH_MODE_ON:
    case FLASH_MODE_AUTO:
        break;

    case FLASH_MODE_OFF:
    default:
        break;
    }

    if(!_isPreviewOn)
        return 0;

    int ret = _v4l2Cam->setCtrl(V4L2_CID_CAMERA_FLASH_MODE,
            _parms.flash_mode);

    LOGE_IF(0 > ret, "%s:Failed to set flash-mode, %s!! ret=%d",
            __func__, strFlashMode, ret);

    return 0 > ret ? -1 : 0;
}

int SecCamera::setBrightness(int brightness)
{
    _parms.brightness = _v4l2Cam->enumBrightness(brightness);

    if(!_isPreviewOn)
        return 0;

    int ret = _v4l2Cam->setCtrl(V4L2_CID_CAMERA_BRIGHTNESS,
            _parms.brightness);

    LOGE_IF(0 > ret, "%s:Failed to set brightness, %d!! ret=%d",
            __func__, brightness, ret);

    return 0 > ret ? -1 : 0;
}

int SecCamera::setFocusMode(const char *strFocusMode)
{
    _parms.focus_mode = _v4l2Cam->enumFocusMode(strFocusMode);

    if(!_isPreviewOn)
        return 0;

    int ret = _v4l2Cam->setCtrl(V4L2_CID_CAMERA_FOCUS_MODE,
            _parms.focus_mode);

    LOGE_IF(0 > ret, "%s:Failed to set focus-mode, %s!! ret=%d",
            __func__, strFocusMode, ret);

    return 0 > ret ? -1 : 0;

}

// -----------------------------------

void SecCamera::_initParms(void)
{
    // Initial setting values of camera sensor
    _parms.brightness           = EV_DEFAULT;
    _parms.effects              = IMAGE_EFFECT_NONE;
    _parms.flash_mode           = FLASH_MODE_OFF;
    _parms.white_balance        = WHITE_BALANCE_AUTO;
    _parms.focus_mode           = FOCUS_MODE_AUTO;
    _parms.contrast             = CONTRAST_DEFAULT;
    _parms.iso                  = ISO_AUTO;
    _parms.metering             = METERING_CENTER;
    _parms.saturation           = SATURATION_DEFAULT;
    _parms.scene_mode           = SCENE_MODE_NONE;
    _parms.sharpness            = SHARPNESS_DEFAULT;
    _parms.fps                  = FRAME_RATE_AUTO;
    _parms.capture.timeperframe.numerator = 1;
    _parms.capture.timeperframe.denominator = 30;
}

// ======================================================================
// Jpeg
int SecCamera::getJpegSnapshot(unsigned char *buffer, unsigned int size)
{
    _jpeg->doCompress((unsigned char *)_captureBuf.start,
            _captureBuf.length);

    return _jpeg->copyOutput(buffer, size);
}

int SecCamera::setJpegQuality(int quality)
{
    return _jpeg->setQuality(quality);
}

int SecCamera::setJpegThumbnailSize(int width, int height)
{
    return _jpeg->setThumbSize(width, height);
}

int SecCamera::setGpsInfo(double latitude, double longitude, unsigned int timestamp, int altitude)
{
    return _jpeg->setGps(latitude, longitude, timestamp, altitude);
}

}; // namespace android
