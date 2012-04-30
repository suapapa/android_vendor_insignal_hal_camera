/*
 *
 * Copyright 2008, The Android Open Source Project
 * Copyright@ Samsung Electronics Co. LTD
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
#define LOG_TAG "CameraHardware"
#include <utils/Log.h>

#include <libyuv.h>
#include <ui/Rect.h>
#include <ui/GraphicBufferMapper.h>
#include <media/stagefright/MetadataBufferType.h>

#include "CameraInfo.h"
#include "CameraHardware.h"

namespace android {

struct ADDRS {
    uint32_t type;
    unsigned int addr_y;
    unsigned int addr_cbcr;
    unsigned int buf_idx;
};

#define MAX_CAM_BUFFERS    (8)

#define CALL_WIN(F, ...)                                        \
    if (_window) {                                              \
        if (_window->F(_window, __VA_ARGS__)) {                 \
            LOGE("%s: Failed while run %s", __func__, #F);      \
            return UNKNOWN_ERROR;                           \
        }                                                       \
    }

CameraHardware::CameraHardware(int cameraId)
    : _cameraId(-1),
      _parms(),
      _previewHeap(NULL),
      _rawHeap(NULL),
      _recordHeap(NULL),
      _camera(NULL),
      _window(NULL),
      _cbNotify(NULL),
      _cbData(NULL),
      _cbDataWithTS(NULL),
      _cbReqMemory(NULL),
      _cbCookie(NULL),
      _msgs(0)
{
    LOGV("%s :", __func__);

    // TODO : get path and ch-idx from CameraInfo
    const char* camPath = CAMERA_DEV_NAME;
    const char* recPath = CAMERA_DEV_NAME2;
    _camera = new SecCamera(camPath, recPath, cameraId);
    if (_camera->getFd() == 0) {
        delete _camera;
        _camera = NULL;
        LOGE("%s: Failed to create SecCamera!!", __func__);
        return;
    }

    _cameraId = cameraId;
    _initParams();

    mExitAutoFocusThread = false;
    /* whether the PreviewThread is active in preview or stopped.  we
     * create the thread but it is initially in stopped state.
     */
    _previewState = PREVIEW_IDLE;
    //_previewThread = new PreviewThread(this);
    _previewThread = new PreviewThread(this);
    _previewThread->startLoop();

    _focusState = FOCUS_IDLE;
    _autofocusThread = new AutoFocusThread(this);
    _autofocusThread->startLoop();

    _pictureState = PICTURE_IDLE;
    _pictureThread = new PictureThread(this);

    LOGI("CameraHardware inited");
}

int CameraHardware::getCameraId(void) const
{
    return _cameraId;
}

void CameraHardware::_initParams(void)
{
    String8 strCamParam(camera_info_get_default_camera_param_str(_cameraId));
    _parms.unflatten(strCamParam);
}

CameraHardware::~CameraHardware()
{
    release();
    LOGI("CameraHardware destroyed");
}

status_t CameraHardware::setPreviewWindow(preview_stream_ops* window)
{
    Mutex::Autolock lock(_previewLock);

    _window = window;

    if (_previewState == PREVIEW_RUNNING) {
        LOGI("Preview window changed while preview is running");
        _stopPreviewLocked();
    }

    if (_window == NULL) {
        LOGV("%s: received NULL window!", __func__);
        return OK;
    }

    int minWinBufs = 0;
    CALL_WIN(get_min_undequeued_buffer_count, &minWinBufs);
    LOGE_IF(minWinBufs >= MAX_CAM_BUFFERS,
            "minWinBufs, %d is too high! expected at most %d.",
            minWinBufs, MAX_CAM_BUFFERS - 1);

    CALL_WIN(set_buffer_count, MAX_CAM_BUFFERS);

    int w, h;
    _parms.getPreviewSize(&w, &h);

    CALL_WIN(set_usage, GRALLOC_USAGE_SW_WRITE_OFTEN);

    // TODO: pixel format hardcoded!!

    // I/SecV4L2Adapter( 2088): passed fmt = 825382478 found pixel format[9]: YUV 4:2:0 planar, Y/CrCb
    // FYI ./system/core/include/system/graphics.h
    CALL_WIN(set_buffers_geometry, w, h, HAL_PIXEL_FORMAT_RGB_565);
    //CALL_WIN(set_buffers_geometry, w, h, HAL_PIXEL_FORMAT_YV12); // 3 plannar
    //CALL_WIN(set_buffers_geometry, w, h, HAL_PIXEL_FORMAT_YCrCb_420_SP); // 2 plannar

    if (_previewState == PREVIEW_PENDING) {
        LOGI("Starting pended preview...");
        status_t ret = _startPreviewLocked();
        return ret;
    }

    return OK;
}

status_t CameraHardware::storeMetaDataInBuffers(bool enable)
{
    // FIXME:
    // metadata buffer mode can be turned on or off.
    // Samsung needs to fix this.
    if (!enable) {
        LOGE("Non-metadata buffer mode is not supported!");
        return INVALID_OPERATION;
    }
    return OK;
}

void CameraHardware::setCallbacks(camera_notify_callback notify_cb,
                                     camera_data_callback data_cb,
                                     camera_data_timestamp_callback data_cb_timestamp,
                                     camera_request_memory req_memory,
                                     void* user)
{
    _cbNotify           = notify_cb;
    _cbData             = data_cb;
    _cbDataWithTS       = data_cb_timestamp;
    _cbReqMemory        = req_memory;
    _cbCookie           = user;
}

void CameraHardware::enableMsgType(int32_t msgType)
{
    _msgs |= msgType;
}

void CameraHardware::disableMsgType(int32_t msgType)
{
    _msgs &= ~msgType;
}

bool CameraHardware::msgTypeEnabled(int32_t msgType)
{
    return (_msgs & msgType);
}

// ---------------------------------------------------------------------------

int CameraHardware::_fillWindow(char* previewFrame, int width, int height)
{
    if (_window == NULL) {
        LOGE("%s: No window!", __func__);
        return UNKNOWN_ERROR;
    }

    buffer_handle_t* buf = NULL;
    int stride = 0;
    CALL_WIN(dequeue_buffer, &buf, &stride);

    /* Let the preview window to lock the buffer. */
    //TODO: cancel_buffer when it failed!!
    CALL_WIN(lock_buffer, buf);

    const Rect bounds(width, height);
    GraphicBufferMapper& grbuffer_mapper(GraphicBufferMapper::get());
    void* vaddr = NULL;

#if 0
    mGrallocHal->lock(mGrallocHal, *buf_handle,
                      GRALLOC_USAGE_SW_WRITE_OFTEN /*| GRALLOC_USAGE_YUV_ADDR*/,
                      0, 0, width, height, vaddr_rgb))
#endif

    status_t res = grbuffer_mapper.lock(*buf, GRALLOC_USAGE_SW_WRITE_OFTEN, bounds, &vaddr);
    if (res != NO_ERROR || vaddr == NULL) {
    LOGE("%s: grbuffer_mapper.lock failure: %d -> %s",
         __FUNCTION__, res, strerror(res));
        CALL_WIN(cancel_buffer, buf);
        return UNKNOWN_ERROR;
    }

    // TODO: this assume windown and preview both are RGB565
    memcpy(vaddr, previewFrame, width* height * 2);

#if 0
    char* src_y = previewFrame;
    char* src_u = src_y + (width * height);
    char* src_v = src_u + (width * height / 4);

    memcpy(vaddr_yuv[0], src_v, width* height / 2);
    memcpy(vaddr_yuv[1], src_v, width* height / 4);
    memcpy(vaddr_yuv[2], src_u, width* height / 4);
#endif

#if 0
    libyuv::NV12ToRGB565((const uint8*)src_y, width,
                         (const uint8*)src_u, width >> 1,
                         (uint8*)vaddr_rgb[0], width * 2, width, height);
#endif

    /* Show it. */
    CALL_WIN(enqueue_buffer, buf);

    // Post the filled buffer!
    grbuffer_mapper.unlock(*buf);

    return NO_ERROR;
}

bool CameraHardware::_previewLoop()
{
    // while (1) {
    _previewLock.lock();
    while (_previewState != PREVIEW_RUNNING
            && _previewState != PREVIEW_RECORDING) {
        LOGI("%s: calling _camera->stopPreview() and waiting", __func__);
        _camera->stopPreview();
        if (_previewState == PREVIEW_ABORT) {
            LOGI("Exiting preview thread...");
            _previewLock.unlock();
            return false;
        }

        // signal that we're stopped
        mPreviewStoppedCondition.signal();
        _previewConditionStateChanged.wait(_previewLock);
        LOGI("%s: return from wait", __func__);
    }
    _previewLock.unlock();

    int index;
    int ret = _camera->getPreviewBuffer(&index, NULL, NULL);
    if (0 > ret) {
        LOGE("%s: Faile to get PhyAddr for Preview!", __func__);
        return false;
    }

    nsecs_t timestamp = systemTime(SYSTEM_TIME_MONOTONIC);

    int w, h, frameSize;
    _camera->getPreviewFrameSize(&w, &h, &frameSize);
    int offset =  frameSize * index;
    _fillWindow(((char*)_previewHeap->data) + offset, w, h);

#if 0
    // Notify the client of a new frame.
    if (_cbData && (_msgs & CAMERA_MSG_PREVIEW_FRAME)) {
        const char* preview_format = _parms.getPreviewFormat();
        if (!strcmp(preview_format, CameraParameters::PIXEL_FORMAT_YUV420SP)) {
            // Color conversion from YUV420 to NV21
            char* vu = ((char*)_previewHeap->data) + offset + w * h;
            const int uv_size = (w * h) >> 1;
            char saved_uv[uv_size];
            memcpy(saved_uv, vu, uv_size);
            char* u = saved_uv;
            char* v = u + (uv_size >> 1);

            int h = 0;
            while (h < w * h / 4) {
                *vu++ = *v++;
                *vu++ = *u++;
                ++h;
            }
        }
        _cbData(CAMERA_MSG_PREVIEW_FRAME, _previewHeap, index, NULL, _cbCookie);
    }
#endif

    //_fillWindow(((char*)_previewHeap->data) + offset, w, h);

    _previewLock.lock();
    if (_previewState == PREVIEW_RECORDING) {
        unsigned int phyYAddr, phyCAddr;
        int ret = _camera->getRecordBuffer(&index, &phyYAddr, &phyCAddr);
        if (0 > ret) {
            LOGE("%s: Failed to get PhyAddr for Record!", __func__);
            return false;
        }

        struct ADDRS* addrs     = (struct ADDRS*)_recordHeap->data;
        addrs[index].type       = kMetadataBufferTypeCameraSource;
        addrs[index].addr_y     = phyYAddr;
        addrs[index].addr_cbcr  = phyCAddr;
        addrs[index].buf_idx    = index;

        // Notify the client of a new frame.
        if (_cbDataWithTS && (_msgs & CAMERA_MSG_VIDEO_FRAME)) {
            _cbDataWithTS(timestamp, CAMERA_MSG_VIDEO_FRAME,
                          _recordHeap, index, _cbCookie);
        } else {
            _camera->releaseRecordFrame(index);
        }
    }
    _previewLock.unlock();
    // }
    return true;
}


status_t CameraHardware::_startPreviewLocked()
{
    LOGV("%s", __func__);

    if (_previewState != PREVIEW_PENDING) {
        LOGE("%s: Seems like no preview window set yet!", __func__);
        return UNKNOWN_ERROR;
    }

    int ret  = _camera->startPreview();
    if (ret < 0) {
        LOGE("ERR(%s):Fail on mSecCamera->startPreview()", __func__);
        return UNKNOWN_ERROR;
    }

    if (_previewHeap)
        _previewHeap->release(_previewHeap);

    _previewHeap = _cbReqMemory(_camera->getFd(),
                                _camera->getPreviewFrameSize(),
                                MAX_CAM_BUFFERS, 0 /* no cookie */);
    if (_previewHeap == NULL) {
        LOGE("%s: Failed to request memory for preview!", __func__);
        _camera->stopPreview();
        return NO_MEMORY;
    }

    _previewState = PREVIEW_RUNNING;
    _previewConditionStateChanged.signal();

    return NO_ERROR;
}

status_t CameraHardware::startPreview()
{
    if (_waitPictureComplete() != NO_ERROR) {
        LOGE("%s: Too long wait for capture finish!", __func__);
        return TIMED_OUT;
    }

    Mutex::Autolock lock(_previewLock);

    if (_previewState != PREVIEW_IDLE) {
        LOGE("%s: Expected preview is idle but, _previewState = %d",
             __func__, _previewState);
        return UNKNOWN_ERROR;
    }

    _previewState = PREVIEW_PENDING;
    if (_window == NULL) {
        LOGW("%s: No preview window set yet. preview will be started "
             "when a window is set.", __func__);
        return NO_ERROR;
    }

    return _startPreviewLocked();
}

void CameraHardware::_stopPreviewLocked()
{
    if (_previewState != PREVIEW_RUNNING) {
        LOGV("%s: Expected preview is running but, _previewState = %d",
             __func__, _previewState);
        return;
    }

    // request that the preview thread stop.
    _previewState = PREVIEW_IDLE;
    _previewConditionStateChanged.signal();
    // wait until preview thread is stopped.
    mPreviewStoppedCondition.wait(_previewLock);
}

void CameraHardware::stopPreview()
{
    Mutex::Autolock lock(_previewLock);

    _stopPreviewLocked();
}

bool CameraHardware::previewEnabled()
{
    Mutex::Autolock lock(_previewLock);

    return (_previewState == PREVIEW_RUNNING || _previewState == PREVIEW_PENDING);
}

// ---------------------------------------------------------------------------

status_t CameraHardware::startRecording()
{
    LOGV("%s :", __func__);

    Mutex::Autolock lock(_previewLock);

    if (_previewState != PREVIEW_RUNNING) {
        LOGV("%s: Expected preview is running but, _previewState = %d",
             __func__, _previewState);
        return UNKNOWN_ERROR;
    }

    if (_recordHeap)
        _recordHeap->release(_recordHeap);

    _recordHeap = _cbReqMemory(-1, sizeof(struct ADDRS), MAX_CAM_BUFFERS, NULL);
    if (_recordHeap == NULL) {
        LOGE("ERR(%s): Record heap creation fail", __func__);
        return NO_MEMORY;
    }

    if (_camera->startRecord() < 0) {
        LOGE("ERR(%s):Fail on _camera->startRecord()", __func__);
        return UNKNOWN_ERROR;
    }
    _previewState = PREVIEW_RECORDING;

    return NO_ERROR;
}

void CameraHardware::stopRecording()
{
    LOGV("%s :", __func__);

    Mutex::Autolock lock(_previewLock);

    if (_previewState != PREVIEW_RECORDING) {
        LOGW("%s: Expected recording is running but, _previewState = %d",
             __func__, _previewState);
        return;
    }

    if (_camera->stopRecord() < 0) {
        LOGE("ERR(%s):Fail on _camera->stopRecord()", __func__);
        return;
    }
    _previewState = PREVIEW_RUNNING;
}

bool CameraHardware::recordingEnabled()
{
    LOGV("%s :", __func__);

    Mutex::Autolock lock(_previewLock);

    return (_previewState == PREVIEW_RECORDING);
}

void CameraHardware::releaseRecordingFrame(const void* opaque)
{
    struct ADDRS* addrs = (struct ADDRS*)opaque;
    _camera->releaseRecordFrame(addrs->buf_idx);
}

bool CameraHardware::_autofocusLoop()
{
    LOGV("%s : starting", __func__);

    /* block until we're told to start.  we don't want to use
     * a restartable thread and requestExitAndWait() in cancelAutoFocus()
     * because it would cause deadlock between our callbacks and the
     * caller of cancelAutoFocus() which both want to grab the same lock
     * in CameraServices layer.
     */
    mFocusLock.lock();
    /* check early exit request */
    if (mExitAutoFocusThread) {
        mFocusLock.unlock();
        LOGV("%s : exiting on request0", __func__);
        return false;
    }
    mFocusCondition.wait(mFocusLock);
    /* check early exit request */
    if (mExitAutoFocusThread) {
        mFocusLock.unlock();
        LOGV("%s : exiting on request1", __func__);
        return false;
    }
    mFocusLock.unlock();

    if (0 == (_msgs & CAMERA_MSG_FOCUS)) {
        LOGW("%s: AF not enabled by App!", __func__);
        return NO_ERROR;
    }

    LOGV("%s : calling setAutoFocus", __func__);
    if (_camera->startAutoFocus() < 0) {
        LOGE("Stop AutoFocusThread!");
        return false;
    }

    //sched_yield();

    bool afDone = _camera->getAutoFocusResult();
    _cbNotify(CAMERA_MSG_FOCUS, afDone, 0, _cbCookie);

    LOGV("%s : exiting with no error", __func__);
    return true;
}

status_t CameraHardware::autoFocus()
{
    LOGV("%s :", __func__);
    /* signal autoFocusThread to run once */
    if (_autofocusThread != NULL)
        mFocusCondition.signal();
    return NO_ERROR;
}

status_t CameraHardware::cancelAutoFocus()
{
    LOGV("%s :", __func__);

//TODO: is it cancleable?
#if 0
    if (_camera->abortAutoFocus() < 0) {
        LOGE("ERR(%s):Fail on _camera->cancelAutofocus()", __func__);
        return UNKNOWN_ERROR;
    }
#endif

    return NO_ERROR;
}

bool CameraHardware::_pictureLoop()
{
    int ret = NO_ERROR;

    Mutex::Autolock lock(_pictureLock);

    //_pictureLock.lock();
    _pictureState = PICTURE_CAPTURING;
    mCaptureCondition.broadcast();
    //_pictureLock.unlock();

    LOGV("doing snapshot...");
    ret = _camera->startSnapshot();
    if (ret != 0) {
        LOGE("%s: Failed to do snapshot!", __func__);
        ret = UNKNOWN_ERROR;
        goto out;
    }

    if (_cbNotify && (_msgs & CAMERA_MSG_SHUTTER))
        _cbNotify(CAMERA_MSG_SHUTTER, 0, 0, _cbCookie);

    _camera->getSnapshot();

    if (_cbData && (_msgs & CAMERA_MSG_RAW_IMAGE)) {
        LOGV("getting raw snapshot...");
        uint8_t* rawAddr = (uint8_t*)_rawHeap->data;
        int rawSize = _rawHeap->size;
        _camera->getRawSnapshot(rawAddr, rawSize);

        _cbData(CAMERA_MSG_RAW_IMAGE, _rawHeap, 0, NULL, _cbCookie);
    } else if (_cbNotify && (_msgs & CAMERA_MSG_RAW_IMAGE_NOTIFY)) {
        _cbNotify(CAMERA_MSG_RAW_IMAGE_NOTIFY, 0, 0, _cbCookie);
    }

    _pictureState = PICTURE_COMPRESSING;
    mCaptureCondition.broadcast();

    if (_cbData && (_msgs & CAMERA_MSG_COMPRESSED_IMAGE)) {
        // TODO: the heap size too enough for jpeg..
        int jpegDataHeapSize = _camera->getSnapshotFrameSize();
        camera_memory_t* jpegDataHeap = _cbReqMemory(-1, jpegDataHeapSize, 1, 0);
        if (jpegDataHeap == NULL) {
            LOGE("%s: Failed to get memory for jpegJeap!", __func__);
            return false;
        }

        LOGV("getting jpeg snapshot...");
        int jpegDataSize = _camera->getJpegSnapshot((uint8_t*)jpegDataHeap->data,
                           jpegDataHeapSize);

        LOGV("copying meta datas...");
        // TODO: add exif and thumbanil in jpeg
        int jpegMetaSize = 0;


        LOGV("joining jpeg meta and data...");
        int jpegHeapSize = jpegMetaSize + jpegDataSize ;
        camera_memory_t* jpegHeap = _cbReqMemory(-1, jpegHeapSize, 1, 0);
        if (jpegHeap == NULL) {
            LOGE("%s: Failed to get memory for jpegJeap!", __func__);
            return false;
        }
        memcpy((uint8_t*)jpegHeap->data, jpegDataHeap->data, jpegDataSize);

        _cbData(CAMERA_MSG_COMPRESSED_IMAGE, jpegHeap, 0, NULL, _cbCookie);

        jpegDataHeap->release(jpegDataHeap);
        //jpegMetaHeap->release(jpegMetaHeap);
        jpegHeap->release(jpegHeap);
    }

    LOGV("snapshot done");
out:
    ret = _camera->endSnapshot();

    _pictureState = PICTURE_IDLE;
    mCaptureCondition.broadcast();

    return false;
}

status_t CameraHardware::_waitPictureComplete()
{
    Mutex::Autolock lock(_pictureLock);

    // 5 seconds timeout
    nsecs_t endTime = 5000000000LL + systemTime(SYSTEM_TIME_MONOTONIC);
    while (_pictureState != PICTURE_IDLE) {
        nsecs_t remainingTime = endTime - systemTime(SYSTEM_TIME_MONOTONIC);
        if (remainingTime <= 0) {
            LOGE("Timed out waiting picture thread.");
            return TIMED_OUT;
        }
        LOGD("Waiting for picture thread to complete.");
        mCaptureCondition.waitRelative(_pictureLock, remainingTime);
    }
    return NO_ERROR;
}

status_t CameraHardware::takePicture()
{
    stopPreview();

    if (_waitPictureComplete() != NO_ERROR) {
        LOGE("%s: Too long wait for capture finish!", __func__);
        return TIMED_OUT;
    }

    if (_pictureThread->startLoop() != NO_ERROR) {
        LOGE("%s : couldn't run picture thread", __func__);
        return UNKNOWN_ERROR;
    }

    return NO_ERROR;
}

status_t CameraHardware::cancelPicture()
{
    return NO_ERROR;
}

bool CameraHardware::_isParamUpdated(const CameraParameters& newParams,
                                        const char* key, const char* newValue) const
{
    if (NULL == newValue) {
        LOGV("%s: no value to compare for %s!!", __func__, key);
        return false;
    }

    const char* currValue = _parms.get(key);
    if (NULL == currValue) {
        LOGV("_parms has no key, %s", key);
        return false;
    }

    if (0 != strcmp(newValue, currValue)) {
        LOGI("CameraParameter, %s updated: %s->%s", key, currValue, newValue);
        return true;
    }

    return false;
}

bool CameraHardware::_isParamUpdated(const CameraParameters& newParams,
                                        const char* key, int newValue) const
{
    if (NULL == _parms.get(key)) {
        LOGV("%s: _parms has no key, %s", __func__, key);
        return false;
    }

    int currValue = _parms.getInt(key);

    if (currValue != newValue) {
        LOGI("CameraParameter, %s updated: %d->%d", key, currValue, newValue);
        return true;
    }

    return false;
}

status_t CameraHardware::setParameters(const CameraParameters& parms)
{
    status_t ret = NO_ERROR;
    const char* strKey;
    int width, height, quality;
    const char* strSize;
    const char* strPixfmt;
    int err = 0;

    /* if someone calls us while picture thread is running, it could screw
     * up the sensor quite a bit so return error.  we can't wait because
     * that would cause deadlock with the callbacks
     */
    if (_waitPictureComplete() != NO_ERROR) {
        LOGE("%s: Too long wait for capture finish!", __func__);
        return TIMED_OUT;
    }

    // preview-size and preview-format
    strSize = parms.get(CameraParameters::KEY_PREVIEW_SIZE);
    strPixfmt = parms.get(CameraParameters::KEY_PREVIEW_FORMAT);
    if (_previewState != PREVIEW_RUNNING
            || _isParamUpdated(parms, CameraParameters::KEY_PREVIEW_SIZE, strSize)
            || _isParamUpdated(parms, CameraParameters::KEY_PREVIEW_FORMAT, strPixfmt)) {
        parms.getPreviewSize(&width, &height);
        LOGV("setting preview format to %dx%d(%s)...", width, height, strPixfmt);
        err = _camera->setPreviewFormat(width, height, strPixfmt);
        if (!err) {
#if defined(BOARD_USES_CAMERA_OVERLAY)
#endif
            _parms.setPreviewSize(width, height);
            _parms.setPreviewFormat(strPixfmt);
        }
    }

    // picture-size and picture-format
    strSize = parms.get(CameraParameters::KEY_PICTURE_SIZE);
    strPixfmt = parms.get(CameraParameters::KEY_PICTURE_FORMAT);
    if (_previewState != PREVIEW_RUNNING
            || _isParamUpdated(parms, CameraParameters::KEY_PICTURE_SIZE, strSize)
            || _isParamUpdated(parms, CameraParameters::KEY_PICTURE_FORMAT, strPixfmt)) {
        parms.getPictureSize(&width, &height);
        LOGV("setting picture format to %dx%d(%s)...", width, height, strPixfmt);
        err = _camera->setSnapshotFormat(width, height, strPixfmt);
        if (!err) {
            int rawHeapSize = _camera->getSnapshotFrameSize();
            if (_rawHeap != NULL)
                _rawHeap->release(_rawHeap);

            _rawHeap = _cbReqMemory(-1, rawHeapSize, 1, 0);
            if (_rawHeap == NULL) {
                LOGE("%s: Failed to create RawHeap!", __func__);
                return NO_MEMORY;
            }
            _parms.setPictureSize(width, height);
            _parms.setPictureFormat(strPixfmt);
        }
    }

    // jpeg-thumbnail-width and jpeg-thumbnail-height
    width  = parms.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH);
    height = parms.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT);
    if (_isParamUpdated(parms, CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, width)
            || _isParamUpdated(parms, CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, height)) {
        LOGV("setting thumbnail size to %dx%d", width, height);
        err = _camera->setJpegThumbnailSize(width, height);
        if (!err) {
            _parms.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, width);
            _parms.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, height);
        }
    }

    // jpeg-thumbnail-quality
    strKey = CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY;
    // TODO: add code for thumbnail-quality

    // jpeg-quality
    strKey = CameraParameters::KEY_JPEG_QUALITY;
    quality = parms.getInt(strKey);
    if (_isParamUpdated(parms, strKey, quality)) {
        LOGV("setting %s to %d...", strKey, quality);
        err = _camera->setJpegQuality(quality);
        if (!err)
            _parms.set(strKey, quality);
    }

    // rotation
    strKey = CameraParameters::KEY_ROTATION;
    int rot = parms.getInt(strKey);
    if (_isParamUpdated(parms, strKey, rot)) {
        LOGV("setting %s to %d...", strKey, rot);
        err = _camera->setRotate(rot);
        if (!err)
            _parms.set(strKey, rot);
    }

    // flash-mode
    strKey = CameraParameters::KEY_FLASH_MODE;
    const char* strFlashMode = parms.get(strKey);
    if (_isParamUpdated(parms, strKey, strFlashMode)) {
        LOGV("Setting flash-mode to %s...", strFlashMode);
        err = _camera->setFlashMode(strFlashMode);
        if (!err)
            _parms.set(strKey, strFlashMode);
    }

    // scene-mode
    strKey = CameraParameters::KEY_SCENE_MODE;
    const char* strSceneMode = parms.get(strKey);
    if (_isParamUpdated(parms, strKey, strSceneMode)) {
        LOGV("setting scene-mode to %s...", strSceneMode);
        err = _camera->setSceneMode(strSceneMode);
        if (!err)
            _parms.set(strKey, strSceneMode);
    }

    // whitebalance
    strKey = CameraParameters::KEY_WHITE_BALANCE;
    const char* strWhiteBalance = parms.get(strKey);
    if (_isParamUpdated(parms, strKey, strWhiteBalance)) {
        LOGV("setting whitebalance to %s...", strWhiteBalance);
        err = _camera->setWhiteBalance(strWhiteBalance);
        if (!err)
            _parms.set(strKey, strWhiteBalance);
    }

    // effect
    strKey = CameraParameters::KEY_EFFECT;
    const char* strEffect = parms.get(strKey);
    if (_isParamUpdated(parms, strKey, strEffect)) {
        LOGV("setting effect to %s...", strEffect);
        err = _camera->setEffect(strEffect);
        if (!err)
            _parms.set(strKey, strEffect);
    }

    // exposure-compensation
    strKey = CameraParameters::KEY_EXPOSURE_COMPENSATION;
    int nEv = parms.getInt(strKey);
    if (_isParamUpdated(parms, strKey, nEv)) {
        LOGV("setting ev to %d...", nEv);
        err = _camera->setBrightness(nEv);
        if (!err)
            _parms.set(strKey, nEv);
    }

    // frame rate
    int new_frame_rate = parms.getPreviewFrameRate();
    if (new_frame_rate < 5 || new_frame_rate > 30)
        new_frame_rate = 30;

    _parms.setPreviewFrameRate(new_frame_rate);
#if 0
    _camera->setFrameRate(new_frame_rate);
#endif

    // fps range
    // TODO: need codes for fps range

    LOGV("--%s : err = %d", __func__, err);
    return err ? UNKNOWN_ERROR : NO_ERROR;
}

CameraParameters CameraHardware::getParameters() const
{
    LOGV("%s :", __func__);
    return _parms;
}

status_t CameraHardware::sendCommand(int32_t command, int32_t arg1,
                                        int32_t arg2)
{
    return BAD_VALUE;
}

void CameraHardware::release()
{
    LOGV("%s :", __func__);

    /* shut down any threads we have that might be running.  do it here
     * instead of the destructor.  we're guaranteed to be on another thread
     * than the ones below.  if we used the destructor, since the threads
     * have a reference to this object, we could wind up trying to wait
     * for ourself to exit, which is a deadlock.
     */
    if (_previewThread != NULL) {
        /* this thread is normally already in it's threadLoop but blocked
         * on the condition variable or running.  signal it so it wakes
         * up and can exit.
         */
        _previewLock.lock();
        _previewThread->requestExit();
        _previewState = PREVIEW_ABORT;
        _previewConditionStateChanged.signal();
        _previewLock.unlock();
        _previewThread->requestExitAndWait();
        _previewThread.clear();
    }

    if (_autofocusThread != NULL) {
        /* this thread is normally already in it's threadLoop but blocked
         * on the condition variable.  signal it so it wakes up and can exit.
         */
        mFocusLock.lock();
        _autofocusThread->requestExit();
        mExitAutoFocusThread = true;
        mFocusCondition.signal();
        mFocusLock.unlock();
        _autofocusThread->requestExitAndWait();
        _autofocusThread.clear();
    }

    if (_pictureThread != NULL) {
        _pictureThread->requestExitAndWait();
        _pictureThread.clear();
    }

    // release heaps
    if (_rawHeap) {
        _rawHeap->release(_rawHeap);
        _rawHeap = NULL;
    }
    if (_previewHeap) {
        _previewHeap->release(_previewHeap);
        _previewHeap = NULL;
    }
    if (_recordHeap) {
        _recordHeap->release(_recordHeap);
        _recordHeap = NULL;
    }

    /* close after all the heaps are cleared since those
     * could have dup'd our file descriptor.
     */
    if (_camera != NULL) {
        delete _camera;
        //_camera->Destroy();
        _camera = NULL;
    }
}

status_t CameraHardware::dump(int fd) const
{
    // TODO: fill dump!

    return NO_ERROR;
}

// ---------------------------------------------------------------------------
#if 0
int CameraHardware::m_getGpsInfo(CameraParameters* camParams, gps_info* gps)
{
    int flag_gps_info_valid = 1;
    int each_info_valid = 1;

    if (camParams == NULL || gps == NULL)
        return -1;

#define PARSE_LOCATION(what,type,fmt,desc)                              \
    do {                                                                \
        const char *what##_str = camParams->get("gps-"#what);           \
        if (what##_str) {                                               \
            type what = 0;                                              \
            if (sscanf(what##_str, fmt, &what) == 1)                    \
                gps->what = what;                                       \
            else {                                                      \
                LOGE("GPS " #what " %s could not"                       \
                        " be parsed as a " #desc,                       \
                        what##_str);                                    \
                each_info_valid = 0;                                    \
            }                                                           \
        } else                                                          \
            each_info_valid = 0;                                        \
    } while(0)

    PARSE_LOCATION(latitude,  double, "%lf", "double float");
    PARSE_LOCATION(longitude, double, "%lf", "double float");

    if (each_info_valid == 0)
        flag_gps_info_valid = 0;

    PARSE_LOCATION(timestamp, long, "%ld", "long");
    if (!gps->timestamp)
        gps->timestamp = time(NULL);

    PARSE_LOCATION(altitude, int, "%d", "int");

#undef PARSE_LOCATION

    if (flag_gps_info_valid == 1)
        return 0;
    else
        return -1;
}
#endif

}; // namespace android