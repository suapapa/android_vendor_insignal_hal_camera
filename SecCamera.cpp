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

#define CAMERA_DEV_NAME         "/dev/video0"

using namespace android;

namespace android {

SecCamera::SecCamera(int ch) :
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
    LOGI("%s()", __func__);

    _v4l2Cam = new SecV4L2Adapter(CAMERA_DEV_NAME, ch);
    _encoder = get_encoder();
    _tagger = get_tagger();

    if (_v4l2Cam->getFd() == 0) {
        _release();
        LOGE("!! %s: Failed to create SecCamera !!", __func__);
        return;
    }

    _initParms();

    _isInited = true;
}

SecCamera::~SecCamera()
{
    LOGI("%s()", __func__);

    if (!_isInited) {
        LOGV("%s: Can't destroy! SecCamera not inited!", __func__);
        return;
    }

    LOGE_IF(this->stopPreview() < 0,
            "ERR(%s):Fail on stopPreview()\n", __func__);

    stopRecord();
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

    if (_v4l2Rec)
        delete _v4l2Rec;
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
    ret = _v4l2Cam->setupBufs(_previewWidth, _previewHeight, _previewPixfmt,
                              MAX_CAM_BUFFERS, 0);
    CHECK(ret > 0);

    /* start with all buffers in queue */
    ret = _v4l2Cam->qAllBufs();
    CHECK_EQ(ret, 0);

    ret = _v4l2Cam->startStream(true);
    CHECK_EQ(ret, 0);

    _isPreviewOn = true;

    ret = _v4l2Cam->setParm(&_v4l2Params);
    CHECK_EQ(ret, 0);

    ret = _v4l2Cam->waitFrame();
    CHECK_EQ(ret, 0);

    // discard first frame
    int index = _v4l2Cam->dqBuf();
    ret = _v4l2Cam->qBuf(index);
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

    _v4l2Cam->closeBufs();
    _isPreviewOn = false;

    return 0;
}

#ifdef DUAL_PORT_RECORDING
#define CAMERA_DEV_NAME2        "/dev/video2"
//Recording
int SecCamera::startRecord(void)
{
    int ret = 0;
    // aleady started
    if (_isRecordOn == true) {
        LOGE("ERR(%s):Preview was already started\n", __func__);
        return 0;
    }

    if (_v4l2Rec == NULL) {
        _v4l2Rec = new SecV4L2Adapter(CAMERA_DEV_NAME2, _v4l2Cam->getChIdx());
        if (_v4l2Rec->getFd() == 0) {
            LOGE("failed to open rec video ch");
            return -1;
        }
    }

    ret = _v4l2Rec->setupBufs(_previewWidth, _previewHeight,
                              V4L2_PIX_FMT_NV12,
                              MAX_CAM_BUFFERS, 0);
    CHECK(ret > 0);

    /* start with all buffers in queue */
    ret = _v4l2Rec->qAllBufs();
    CHECK_EQ(ret, 0);

    ret = _v4l2Rec->startStream(true);
    CHECK(ret == 0);

    // It is a delay for a new frame, not to show the previous bigger ugly picture frame.
    ret = _v4l2Rec->waitFrame();
    CHECK_EQ(ret, 0);

    int index = _v4l2Rec->dqBuf();
    ret = _v4l2Rec->qBuf(index);
    CHECK(ret == 0);

    _isRecordOn = true;
    LOGI("Recording started!");

    return 0;
}

int SecCamera::stopRecord(void)
{
    LOGV("%s :", __func__);

    if (!_isRecordOn || _v4l2Rec == NULL) {
        return 0;
    }


    int ret = _v4l2Rec->startStream(false);
    CHECK(ret == 0);

    _v4l2Rec->closeBufs();
    _isRecordOn = false;

    LOGI("Recording stopped!");
#if 0
    if (_v4l2Rec) {
        delete _v4l2Rec;
        _v4l2Rec = NULL;
    }
#endif

    return 0;
}

int SecCamera::dqRecordBuffer(int* index, unsigned int* addrY, unsigned int* addrC)
{
    if (!_isRecordOn || _v4l2Rec == NULL) {
        LOGW("Recording stoped! will ignore dqRecordBuffer!");
        return -1;
    }

    _v4l2Rec->waitFrame();
    //*index = _v4l2Rec->blk_dqbuf();
    *index = _v4l2Rec->dqBuf();
    if (!(0 <= *index && *index < MAX_CAM_BUFFERS)) {
        LOGE("ERR(%s):wrong index = %d\n", __func__, *index);
        return -1;
    }

    _v4l2Rec->getAddr(*index, addrY, addrC);

    return 0;
}

void SecCamera::qRecordBuffer(int index)
{
    if (!_isRecordOn || _v4l2Rec == NULL) {
        LOGW("Recording stoped! will ignore qRecordBuffer!");
        return;
    }

    int ret = _v4l2Rec->qBuf(index);
    //CHECK(ret == 0);
}
#endif

int SecCamera::_getPhyAddr(int index, unsigned int* addrY, unsigned int* addrC)
{
    _v4l2Cam->getAddr(index, addrY, addrC);

    return 0;
}

int SecCamera::dqPreviewBuffer(int* index, unsigned int* addrY, unsigned int* addrC)
{
    *index = _v4l2Cam->blk_dqbuf();
    if (!(0 <= *index && *index < MAX_CAM_BUFFERS)) {
        LOGE("ERR(%s):wrong index = %d\n", __func__, *index);
        return -1;
    }

    return _v4l2Cam->getAddr(*index, addrY, addrC);
}

void SecCamera::qPreviewBuffer(int index)
{
    int ret = _v4l2Cam->qBuf(index);
    LOGE_IF(ret, "Failed to queue preview buffer, %d to camera!", index);
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

    LOGI("previewFormat=%dx%d(%s), frameSize=%d",
         width, height, strPixfmt, _previewFrameSize);

    //return _previewFrameSize > 0 ? 0 : -1;
    return 0;
}

unsigned int SecCamera::getPreviewFrameSize(void)
{
    return _v4l2Cam->frameSize();
}


void SecCamera::getPreviewFrameSize(int* width, int* height, int* frameSize)
{
    if (width)
        *width = _previewWidth;
    if (height)
        *height = _previewHeight;
    if (frameSize)
        *frameSize =  _v4l2Cam->frameSize();

}

// ======================================================================
// Snapshot
int SecCamera::endSnapshot(void)
{
    return _v4l2Cam->closeBufs();
}

int SecCamera::startSnapshot(size_t* captureSize)
{
    LOG_TIME_START(0);
    stopPreview();
    LOG_TIME_END(0);

    int ret;
    LOG_TIME_START(1); // prepare
    ret = _v4l2Cam->setupBufs(_snapshotWidth, _snapshotHeight, _snapshotPixfmt,
                              1, 1);
    CHECK_EQ(ret, 1);

    ret = _v4l2Cam->mapBuf(0);
    CHECK_EQ(ret, 0);

    _v4l2Cam->qBuf(0);
    _v4l2Cam->startStream(true);
    LOG_TIME_END(1);

    LOG_CAMERA("%s: stopPreview(%lu), prepare(%lu) us",
               __func__, LOG_TIME(0), LOG_TIME(1));

    _v4l2Cam->mapBufInfo(0, NULL, captureSize);

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
        index = _v4l2Cam->dqBuf();
        _v4l2Cam->qBuf(index);
        skipFirstNFrames--;
    }

    _v4l2Cam->waitFrame();
    index = _v4l2Cam->dqBuf();
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

    void* captureStart;
    size_t captureSize;
    _v4l2Cam->mapBufInfo(0, &captureStart, &captureSize);

    if (size < captureSize) {
        LOGE("%s: buffer size, %d is too small! for snapshot size, %d",
             __func__, size, captureSize);
    } else {
        size = captureSize;
    }

    memcpy(buffer, captureStart, size);

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
    bool afSuccess = false;

    // TODO: following code is only for S5K4ECGX sensor.
    // need to move to SecV4L2Adapter or somewhere less general place.
#define AF_SEARCH_COUNT 80
#define AF_PROGRESS 0x01
#define AF_SUCCESS 0x02
    int ret;

    for (int i = 0; i < AF_SEARCH_COUNT; i++) {
        ret = _v4l2Cam->getCtrl(V4L2_CID_CAMERA_AUTO_FOCUS_RESULT_FIRST);
        if (ret != AF_PROGRESS)
            break;
        usleep(50000);
    }
    if (ret != AF_SUCCESS) {
        LOGV("%s : 1st AF timed out, failed, or was canceled", __func__);
        _v4l2Cam->setCtrl(V4L2_CID_CAMERA_FINISH_AUTO_FOCUS, 0);
        return false;
    }

    for (int i = 0; i < AF_SEARCH_COUNT; i++) {
        ret = _v4l2Cam->getCtrl(V4L2_CID_CAMERA_AUTO_FOCUS_RESULT_SECOND);
        /* low byte is garbage.  done when high byte is 0x0 */
        if (!(ret & 0xff00)) {
            afSuccess = true;
            break;
        }
        usleep(50000);
    }

    LOGI("AF was %s", afSuccess ? "successful" : "fail");

    if (_v4l2Cam->setCtrl(V4L2_CID_CAMERA_FINISH_AUTO_FOCUS, 0) < 0) {
        LOGE("%s: Fail on V4L2_CID_CAMERA_FINISH_AUTO_FOCUS", __func__);
        return false;
    }

    return afSuccess;
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
    if (rawSize < pW * pH * 2) {
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
            _exifParams.thumbData = new uint8_t[thumbJpegSize];
            memcpy(_exifParams.thumbData, thumbJpegData, thumbJpegSize);
            _exifParams.thumbSize = thumbJpegSize;
        }

        return 0;
    }

nothumbnail:
    LOGW("no thumbnail available!");

    if (thumbRawData)
        delete[] thumbRawData;

    _exifParams.thumbData = NULL;
    _exifParams.thumbSize = 0;

    return -1;
}

int SecCamera::compressToJpeg(unsigned char* rawData, size_t rawSize)
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

    LOGI("tagging...");
    int taggedJpegSize = _tagger->tagToJpeg(&_exifParams, jpegBuff, jpegSize);

    if (taggedJpegSize == 0) {
        LOGW("Fail on tagging! will save the jpeg without exif!");
        return jpegSize;
    }

    return taggedJpegSize;
}

int SecCamera::writeJpeg(unsigned char* outBuff, int buffSize)
{
    if (_encoder == NULL) {
        LOGE("%s: has no encoder!", __func__);
        return 0;
    }

    int writtenSize = 0;

    if (_tagger && _tagger->readyToWrite()) {
        writtenSize = _tagger->writeTaggedJpeg(outBuff, buffSize);
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

int SecCamera::setThumbnailQuality(int q)
{
    LOGI("%s: quality = %d", __func__, q);
    _thumbParams.quality = q;
    return 0;
}

int SecCamera::setThumbnailSize(int width, int height)
{
    _thumbParams.width = width;
    _thumbParams.height = height;
    return 0;
}

int SecCamera::setGpsInfo(const char* strLatitude,
                          const char* strLongitude,
                          const char* strAltitude,
                          const char* strTimestamp,
                          const char* strProcessMethod)
{
    _gpsData* gps = &(_exifParams.gps);

    gps->latitude = strtod(strLatitude, NULL);
    gps->longitude = strtod(strLongitude, NULL);
    gps->altitude = strtod(strAltitude, NULL);
    gps->procMethod = (char*)strProcessMethod;
    if (strTimestamp) {
        gps->timestamp = strtol(strTimestamp, NULL, 10);
    }

    return 0;
}

void SecCamera::_initExifParams(void)
{
    LOGV("%s: setting default values for _exifParams...", __func__);
    memset(&_exifParams, 0, sizeof(TaggerParams));
    static const char* strMaker = "Insignal";
    static const char* strModel = "Camera";
    _exifParams.maker = (char*)strMaker;
    _exifParams.model = (char*)strModel;
    _exifParams.wb = EXIF_WB_AUTO;
    _exifParams.metering = EXIF_AVERAGE;
    _exifParams.zoom = 1;
    _exifParams.iso = EXIF_ISO_AUTO;
    _exifParams.thumbData = NULL;
    _exifParams.thumbSize = 0;
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
