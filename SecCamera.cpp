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
#define LOG_TAG "SecCamera"
#include <utils/Log.h>
#include "CameraLog.h"

#include <stdlib.h>
#include <math.h>

#include "SecCamera.h"
#include "CameraFactory.h"

using namespace android;

namespace android {

// ======================================================================
// Camera controls
void SecCamera::releaseRecordFrame(int i)
{
    LOGV("releaseframe : (%d)", i);

#ifdef DUAL_PORT_RECORDING
    int ret = _v4l2Rec->qbuf(i);
    //CHECK(ret == 0);
#endif

}

// ======================================================================
// Constructor & Destructor

SecCamera::SecCamera(const char* camPath, const char* recPath, int ch) :
    _isInited(false),
    _previewWidth(0),
    _previewHeight(0),
    _previewPixfmt(-1),
    _previewFrameSize(0),
    _snapshotWidth(0),
    _snapshotHeight(0),
    _snapshotPixfmt(-1),
    _isPreviewOn(false),
    _isRecordOn(false),
    _v4l2Cam(NULL),
    _v4l2Rec(NULL),
    _encoder(NULL),
    _tagger(NULL)
{
    LOGV("%s()", __func__);

    _v4l2Cam = new SecV4L2Adapter(camPath, ch);
#ifdef DUAL_PORT_RECORDING
    _v4l2Rec = new SecV4L2Adapter(recPath, ch);
#endif
    _encoder = get_encoder();
    _tagger = get_tagger();

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

    if (!_isInited) {
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
    memset(&_exifParams, 0, sizeof(TaggerParams));

    if (_encoder)
        delete _encoder;

    if (_tagger)
        delete _tagger;

    if (_v4l2Cam)
        delete _v4l2Cam;

#ifdef DUAL_PORT_RECORDING
    if (_v4l2Rec)
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
    if (_isPreviewOn == true) {
        LOGE("ERR(%s):Preview was already started\n", __func__);
        return 0;
    }

    int ret = 0;

    ret = _v4l2Cam->setFmt(_previewWidth, _previewHeight, _previewPixfmt, 0);
    CHECK_EQ(ret, 0);

    _v4l2Cam->initBufs(_previewBufs, _previewWidth, _previewHeight,
                       _previewPixfmt, MAX_CAM_BUFFERS);
    ret = _v4l2Cam->reqBufs(V4L2_BUF_TYPE_VIDEO_CAPTURE, MAX_CAM_BUFFERS);
    CHECK_EQ(ret, MAX_CAM_BUFFERS);

    ret = _v4l2Cam->queryBufs(_previewBufs, V4L2_BUF_TYPE_VIDEO_CAPTURE, MAX_CAM_BUFFERS);
    CHECK_EQ(ret, 0);

    /* start with all buffers in queue */
    for (int i = 0; i < MAX_CAM_BUFFERS; i++) {
        ret = _v4l2Cam->qbuf(i);
        CHECK_EQ(ret, 0);
    }

    ret = _v4l2Cam->startStream(true);
    CHECK_EQ(ret, 0);

    _isPreviewOn = true;

    ret = _v4l2Cam->setParm(&_v4l2Params);
    CHECK_EQ(ret, 0);

    ret = _v4l2Cam->waitFrame();
    CHECK_EQ(ret, 0);

    int index = _v4l2Cam->dqbuf();
    ret = _v4l2Cam->qbuf(index);
    CHECK_EQ(ret, 0);

    return 0;
}

int SecCamera::stopPreview(void)
{
    LOG_CAMERA_FUNC_ENTER;

    if (_isPreviewOn == false)
        return 0;

    int ret = _v4l2Cam->startStream(false);
    CHECK(ret == 0);

    _v4l2Cam->closeBufs(_previewBufs, MAX_CAM_BUFFERS);
    _isPreviewOn = false; //Kamat check

    return 0;
}

//Recording
#ifdef DUAL_PORT_RECORDING
int SecCamera::startRecord(void)
{
    LOGI("++%s() \n", __func__);
    int ret = 0;
    // aleady started
    if (_isRecordOn == true) {
        LOGE("ERR(%s):Preview was already started\n", __func__);
        return 0;
    }

    int color_format = V4L2_PIX_FMT_NV12;

    ret = _v4l2Rec->setFmt(_previewWidth, _previewHeight, color_format, 0);
    CHECK(ret == 0);

    _v4l2Cam->initBufs(_recordBufs, _previewWidth, _previewHeight,
                       _previewPixfmt, MAX_CAM_BUFFERS);
    ret = _v4l2Rec->reqBufs(V4L2_BUF_TYPE_VIDEO_CAPTURE, MAX_CAM_BUFFERS);
    CHECK_EQ(ret, MAX_CAM_BUFFERS);

    ret = _v4l2Rec->queryBufs(_recordBufs, V4L2_BUF_TYPE_VIDEO_CAPTURE, MAX_CAM_BUFFERS);
    CHECK_EQ(ret, 0);

    /* start with all buffers in queue */
    for (int i = 0; i < MAX_CAM_BUFFERS; i++) {
        ret = _v4l2Rec->qbuf(i);
        CHECK(ret == 0);
    }

    ret = _v4l2Rec->startStream(true);
    CHECK(ret == 0);

    // It is a delay for a new frame, not to show the previous bigger ugly picture frame.
    ret = _v4l2Rec->waitFrame();
    CHECK_EQ(ret, 0);

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

    if (_isRecordOn == false)
        return 0;


    int ret = _v4l2Rec->startStream(false);
    CHECK(ret == 0);

    _v4l2Rec->closeBufs(_recordBufs, MAX_CAM_BUFFERS);
    _isRecordOn = false;

    LOGI("Recording stopped!");

    return 0;
}

int SecCamera::getRecordBuffer(int* index, unsigned int* addrY, unsigned int* addrC)
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
    if (!(0 <= *index && *index < MAX_CAM_BUFFERS)) {
        LOGE("ERR(%s):wrong index = %d\n", __func__, *index);
        return -1;
    }

    _v4l2Rec->getAddr(*index, addrY, addrC);

    return 0;
}
#endif //DUAL_PORT_RECORDING

int SecCamera::_getPhyAddr(int index, unsigned int* addrY, unsigned int* addrC)
{
    int ret = _v4l2Cam->qbuf(index);
    CHECK(ret == 0);

    _v4l2Cam->getAddr(index, addrY, addrC);

    return 0;
}

int SecCamera::getPreviewBuffer(int* index, unsigned int* addrY, unsigned int* addrC)
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
    if (!(0 <= *index && *index < MAX_CAM_BUFFERS)) {
        LOGE("ERR(%s):wrong index = %d\n", __func__, *index);
        return -1;
    }

    return _getPhyAddr(*index, addrY, addrC);
}

void SecCamera::pausePreview()
{
    _v4l2Cam->setCtrl(V4L2_CID_STREAM_PAUSE, 0);
}

int SecCamera::setPreviewFormat(int width, int height, const char* strPixfmt)
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

void SecCamera::getPreviewFrameSize(int* width, int* height, int* frameSize)
{
    if (width)
        *width = _previewWidth;
    if (height)
        *height = _previewHeight;
    if (frameSize)
        *frameSize = _previewFrameSize;
}

// ======================================================================
// Snapshot
int SecCamera::endSnapshot(void)
{
    return _v4l2Cam->closeBufs(&_captureBuf, 1);
}

int SecCamera::startSnapshot(size_t* captureSize)
{
    LOG_TIME_START(0);
    stopPreview();
    LOG_TIME_END(0);

    int ret = _v4l2Cam->setFmt(_snapshotWidth, _snapshotHeight, _snapshotPixfmt, 1);
    CHECK(ret == 0);

    LOG_TIME_START(1); // prepare
    _v4l2Cam->initBufs(&_captureBuf, _snapshotWidth, _snapshotHeight, _snapshotPixfmt, 1);
    _v4l2Cam->reqBufs(V4L2_BUF_TYPE_VIDEO_CAPTURE, 1);
    _v4l2Cam->queryBufs(&_captureBuf, V4L2_BUF_TYPE_VIDEO_CAPTURE, 1);
    _v4l2Cam->qbuf(0);
    _v4l2Cam->startStream(true);
    LOG_TIME_END(1);

    LOG_CAMERA("%s: stopPreview(%lu), prepare(%lu) us",
               __func__, LOG_TIME(0), LOG_TIME(1));

    *captureSize = _captureBuf.length;

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

int SecCamera::getRawSnapshot(uint8_t* buffer, size_t size)
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

int SecCamera::setSnapshotFormat(int width, int height, const char* strPixfmt)
{
    _snapshotWidth  = width;
    _snapshotHeight = height;
    _snapshotPixfmt = _v4l2Cam->nPixfmt(strPixfmt);

    _pictureParams.width = _snapshotWidth;
    _pictureParams.height = _snapshotHeight;
    _pictureParams.format = _snapshotPixfmt;

    // We will make thumbnail by shrink from original image
    // So, same image format is used
    _thumbParams.format = _snapshotPixfmt;

    LOGI("snapshotFormat=%dx%d(%s)", width, height, strPixfmt);

    _exifParams.width = _snapshotWidth;
    _exifParams.height = _snapshotHeight;

    return 0;
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
// TODO: get result of AF is 2 step,
// V4L2_CID_CAMERA_AUTO_FOCUS_RESULT_FIRST and V4L2_CID_CAMERA_AUTO_FOCUS_RESULT_SECOND
#if 0
    int ret = _v4l2Cam->getCtrl(V4L2_CID_CAMERA_AUTO_FOCUS_RESULT);
    switch (ret) {
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
#endif

    return afDone;
}

// ======================================================================
// Settings

int SecCamera::setRotate(int angle)
{
    int rotate = angle % 360;

    if (angle >= 360 || 0 > angle)
        LOGW("%s: input angle, %d is truncated to %d!", __func__, angle, rotate);

    _exifParams.rotation = angle;

    return 0;
}

int SecCamera::setSceneMode(const char* strSceneMode)
{
    _v4l2Params.scene_mode = _v4l2Cam->enumSceneMode(strSceneMode);

    if (!_isPreviewOn)
        return 0;

    int ret = _v4l2Cam->setCtrl(V4L2_CID_CAMERA_SCENE_MODE,
                                _v4l2Params.scene_mode);

    LOGE_IF(0 > ret, "%s:Failed to set scene-mode, %s!! ret=%d",
            __func__, strSceneMode, ret);

    return 0 > ret ? -1 : 0;
}

int SecCamera::setWhiteBalance(const char* strWhiteBalance)
{
    _v4l2Params.white_balance = _v4l2Cam->enumWB(strWhiteBalance);
    if (_v4l2Params.white_balance == WHITE_BALANCE_AUTO)
        _exifParams.wb = EXIF_WB_AUTO;
    else
        _exifParams.wb = EXIF_WB_MANUAL;

    if (!_isPreviewOn)
        return 0;

    int ret = _v4l2Cam->setCtrl(V4L2_CID_CAMERA_WHITE_BALANCE,
                                _v4l2Params.white_balance);

    LOGE_IF(0 > ret, "%s:Failed to set white-balance, %s!! ret=%d",
            __func__, strWhiteBalance, ret);

    return 0 > ret ? -1 : 0;
}

int SecCamera::setEffect(const char* strEffect)
{
    _v4l2Params.effects = _v4l2Cam->enumEffect(strEffect);

    if (!_isPreviewOn)
        return 0;

    int ret = _v4l2Cam->setCtrl(V4L2_CID_CAMERA_EFFECT,
                                _v4l2Params.effects);

    LOGE_IF(0 > ret, "%s:Failed to set effects, %s!! ret=%d",
            __func__, strEffect, ret);

    return 0 > ret ? -1 : 0;
}

int SecCamera::setFlashMode(const char* strFlashMode)
{
    _v4l2Params.flash_mode = _v4l2Cam->enumFlashMode(strFlashMode);

    LOGV("setting flash-mode set to %s...", strFlashMode);

    switch (_v4l2Params.flash_mode) {
    case FLASH_MODE_TORCH:
        return 0;

    case FLASH_MODE_ON:
    case FLASH_MODE_AUTO:
        break;

    case FLASH_MODE_OFF:
    default:
        break;
    }

    if (!_isPreviewOn)
        return 0;

    int ret = _v4l2Cam->setCtrl(V4L2_CID_CAMERA_FLASH_MODE,
                                _v4l2Params.flash_mode);

    LOGE_IF(0 > ret, "%s:Failed to set flash-mode, %s!! ret=%d",
            __func__, strFlashMode, ret);

    return 0 > ret ? -1 : 0;
}

int SecCamera::setBrightness(int brightness)
{
    _v4l2Params.brightness = _v4l2Cam->enumBrightness(brightness);

    if (!_isPreviewOn)
        return 0;

    int ret = _v4l2Cam->setCtrl(V4L2_CID_CAMERA_BRIGHTNESS,
                                _v4l2Params.brightness);

    LOGE_IF(0 > ret, "%s:Failed to set brightness, %d!! ret=%d",
            __func__, brightness, ret);

    return 0 > ret ? -1 : 0;
}

int SecCamera::setFocusMode(const char* strFocusMode)
{
    _v4l2Params.focus_mode = _v4l2Cam->enumFocusMode(strFocusMode);

    if (!_isPreviewOn)
        return 0;

    int ret = _v4l2Cam->setCtrl(V4L2_CID_CAMERA_FOCUS_MODE,
                                _v4l2Params.focus_mode);

    LOGE_IF(0 > ret, "%s:Failed to set focus-mode, %s!! ret=%d",
            __func__, strFocusMode, ret);

    return 0 > ret ? -1 : 0;

}

// -----------------------------------

void SecCamera::_initParms(void)
{
    memset(&_pictureParams, 0, sizeof(EncoderParams));
    memset(&_thumbParams, 0, sizeof(EncoderParams));

    // Initial setting values of camera sensor
    _v4l2Params.brightness           = EV_DEFAULT;
    _v4l2Params.effects              = IMAGE_EFFECT_NONE;
    _v4l2Params.flash_mode           = FLASH_MODE_OFF;
    _v4l2Params.white_balance        = WHITE_BALANCE_AUTO;
    _v4l2Params.focus_mode           = FOCUS_MODE_AUTO;
    _v4l2Params.contrast             = CONTRAST_DEFAULT;
    _v4l2Params.iso                  = ISO_AUTO;
    _v4l2Params.metering             = METERING_CENTER;
    _v4l2Params.saturation           = SATURATION_DEFAULT;
    _v4l2Params.scene_mode           = SCENE_MODE_NONE;
    _v4l2Params.sharpness            = SHARPNESS_DEFAULT;
    _v4l2Params.fps                  = FRAME_RATE_AUTO;
    _v4l2Params.capture.timeperframe.numerator = 1;
    _v4l2Params.capture.timeperframe.denominator = 30;
}

// ======================================================================
// Jpeg
int SecCamera::_createThumbnail(uint8_t* rawData, int rawSize)
{
    uint8_t* thumbRawData = NULL;
    int thumbRawSize = 0;

    int pW = _pictureParams.width;
    int pH = _pictureParams.height;
    int tW = _thumbParams.width;
    int tH = _thumbParams.height;

    if (_tagger == NULL || tW == 0 || tH == 0)
        goto nothumbnail;

    LOGI("making thumb(%dx%d) from picture(%dx%d)...", tW, tH, pW, pH);
    if (rawSize != pW * pH * 2) {
        LOGE("%s: rawSize=%d, expected=%d", __func__, rawSize, pW * pH * 2);
        goto nothumbnail;
    }

    thumbRawSize = tW * tH * 2;
    LOGV("making raw image for thumbnail(%dx%d)...", tW, tH);
    thumbRawData = new uint8_t[thumbRawSize];

    if (thumbRawData && thumbRawSize) {
        LOGV("shrinking raw image for thumbnail...");
        int ret = _scaleDownYuv422(rawData, pW, pH, thumbRawData, tW, tH);
        if (ret != 0) {
            LOGE("%s: somthing wrong while shrinking raw image for thumbnail!",
                 __func__);
            goto nothumbnail;
        }

        LOGV("compressing thumbnail...");
        _encoder->doCompress(&_thumbParams, thumbRawData, thumbRawSize);

        // now, JPEG data exists in _encoder. free rawdata.
        delete[] thumbRawData;

        uint8_t* thumbJpegData = NULL;
        int thumbJpegSize = 0;
        _encoder->getOutput(&thumbJpegData, &thumbJpegSize);
        if (thumbJpegData && thumbJpegSize) {
            LOGV("copying compressed thumbnail to TaggerParams...");

            // It will be deleted from createTaggedJpeg.
            _exifParams.thumb_data = new uint8_t[thumbJpegSize];
            memcpy(_exifParams.thumb_data, thumbJpegData, thumbJpegSize);
            _exifParams.thumb_size = thumbJpegSize;
        }

        return 0;
    }

nothumbnail:
    LOGW("no thumbnail available!");

    if (thumbRawData)
        delete[] thumbRawData;

    _exifParams.thumb_data = NULL;
    _exifParams.thumb_size = 0;

    return -1;
}

int SecCamera::compress2Jpeg(unsigned char* rawData, size_t rawSize)
{
    if (rawData == NULL || rawSize == 0) {
        LOGE("%s: null input!", __func__);
        return 0;
    }

    if (_encoder == NULL) {
        LOGE("%s: has no encoder!", __func__);
        return 0;
    }

    int ret = 0;

    _createThumbnail(rawData, rawSize);

    LOGI("encording to JPEG...");
    _encoder->doCompress(&_pictureParams, rawData, rawSize);

    uint8_t* jpegBuff = NULL;
    int jpegSize = 0;
    _encoder->getOutput(&jpegBuff, &jpegSize);
    if (0 > jpegSize)
        return -1;

    if (_tagger == NULL) {
        LOGW("No tagger! jpeg will be served without tag");
        return jpegSize;
    }

    LOGI("creating tagged JPEG...");
    int taggedJpegSize = _tagger->createTaggedJpeg(&_exifParams, jpegBuff, jpegSize);

    //TODO: release alloc memories in _encoder
    // now, taggedJpeg data exists in _exif

    return taggedJpegSize;
}

int SecCamera::getJpeg(unsigned char* outBuff, int buffSize)
{
    if (_encoder == NULL) {
        LOGE("%s: has no encoder!", __func__);
        return 0;
    }

    int writtenSize = 0;

    if (_tagger) {
        writtenSize = _tagger->copyJpegWithExif(outBuff, buffSize);
    } else {
        uint8_t* jpegBuff = NULL;
        int jpegSize = 0;
        _encoder->getOutput(&jpegBuff, &jpegSize);

        memcpy(outBuff, jpegBuff, jpegSize);
        writtenSize = jpegSize;
    }

    if (writtenSize > buffSize) {
        LOGE("%s: buffer overflowed!!", __func__);
        return -1;
    }

    if (!(writtenSize > 0)) {
        LOGE("%s: Opps, nothing copied!!", __func__);
        return -1;
    }

    return 0;
}

int SecCamera::setPictureQuality(int q)
{
    LOGI("%s: quality = %d", __func__, q);
    _pictureParams.quality = q;
    return 0;
}

int SecCamera::setThumbQuality(int q)
{
    LOGI("%s: quality = %d", __func__, q);
    _thumbParams.quality = q;
    return 0;
}

int SecCamera::setJpegThumbnailSize(int width, int height)
{
    _thumbParams.width = width;
    _thumbParams.height = height;
    return 0;
}

int SecCamera::_convertGPSCoord(double coord, int* deg, int* min, int* sec)
{
    double tmp;

    if (coord == 0) {
        LOGE("%s: Invalid GPS coordinate", __func__);

        return -1;
    }

    *deg = (int) floor(coord);
    tmp = (coord - floor(coord)) * 60;
    *min = (int) floor(tmp);
    tmp = (tmp - floor(tmp)) * 60;
    *sec = (int) floor(tmp);

    if (*sec >= 60) {
        *sec = 0;
        *min += 1;
    }

    if (*min >= 60) {
        *min = 0;
        *deg += 1;
    }

    return 0;
}

int SecCamera::setGpsInfo(const char* strLatitude, const char* strLongitude,
                          const char* strAltitude, const char* strTimestamp,
                          const char* strProcessMethod, int nAltitudeRef,
                          const char* strMapDatum, const char* strGpsVersion)
{
    gps_data* gps = &_exifParams.gps_location;
    double gpsCoord;

    gpsCoord = strtod(strLatitude, NULL);
    _convertGPSCoord(gpsCoord, &gps->latDeg, &gps->latMin, &gps->latSec);
    gps->latRef = (gpsCoord < 0) ? (char*) "S" : (char*) "N";

    gpsCoord = strtod(strLongitude, NULL);
    _convertGPSCoord(gpsCoord, &gps->longDeg, &gps->longMin, &gps->longSec);
    gps->longRef = (gpsCoord < 0) ? (char*) "W" : (char*) "E";

    gpsCoord = strtod(strAltitude, NULL);
    gps->altitude = gpsCoord;

    if (NULL != strTimestamp) {
        gps->timestamp = strtol(strTimestamp, NULL, 10);
        struct tm* timeinfo = localtime((time_t*) & (gps->timestamp));
        if (timeinfo != NULL)
            strftime(gps->datestamp, 11, "%Y:%m:%d", timeinfo);
    }

    gps->altitudeRef = nAltitudeRef;
    gps->mapdatum = (char*)strMapDatum;
    gps->versionId = (char*)strGpsVersion;
    gps->procMethod = (char*)strProcessMethod;

    return 0;
}

void SecCamera::_initExifParams(void)
{
    LOGV("%s: setting default values for _exifParams...", __func__);
    memset(&_exifParams, 0, sizeof(TaggerParams));
    static const char* strMaker = "TJMedia";
    static const char* strModel = "TDMK";
    _exifParams.maker = (char*)strMaker;
    _exifParams.model = (char*)strModel;
    _exifParams.wb = EXIF_WB_AUTO;
    _exifParams.metering_mode = EXIF_AVERAGE;
    _exifParams.zoom = 1;
    _exifParams.iso = EXIF_ISO_AUTO;
    _exifParams.thumb_data = NULL;
    _exifParams.thumb_size = 0;
}

int SecCamera::_scaleDownYuv422(uint8_t* srcBuf, uint32_t srcWidth, uint32_t srcHight,
                                uint8_t* dstBuf, uint32_t dstWidth, uint32_t dstHight)
{
    int32_t step_x, step_y;
    int32_t iXsrc, iXdst;
    int32_t x, y, src_y_start_pos, dst_pos, src_pos;

    if (dstWidth % 2 != 0 || dstHight % 2 != 0) {
        LOGE("scale_down_yuv422: invalid width, height for scaling");
        return -1;
    }

    step_x = srcWidth / dstWidth;
    step_y = srcHight / dstHight;

    dst_pos = 0;
    for (uint32_t y = 0; y < dstHight; y++) {
        src_y_start_pos = (y * step_y * (srcWidth * 2));

        for (uint32_t x = 0; x < dstWidth; x += 2) {
            src_pos = src_y_start_pos + (x * (step_x * 2));

            dstBuf[dst_pos++] = srcBuf[src_pos    ];
            dstBuf[dst_pos++] = srcBuf[src_pos + 1];
            dstBuf[dst_pos++] = srcBuf[src_pos + 2];
            dstBuf[dst_pos++] = srcBuf[src_pos + 3];
        }
    }

    return 0;
}

}; // namespace android
