/*
**
** Copyright 2008, The Android Open Source Project
** Copyright@ Samsung Electronics Co. LTD
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

//#define LOG_NDEBUG 0
#define LOG_TAG "SecCameraHardware"
#include <utils/Log.h>

#include <utils/threads.h>
#include "SecCameraHardware.h"

#if defined(BOARD_USES_CAMERA_OVERLAY)
#include <hardware/overlay.h>
#include <ui/Overlay.h>
#define CACHEABLE_BUFFERS       0x1
#define ALL_BUFFERS_FLUSHED     -66
#endif

namespace android
{

struct ADDRS {
    unsigned int addr_y;
    unsigned int addr_cbcr;
    unsigned int buf_idx;
};

SecCameraHardware::SecCameraHardware(int cameraId)
    : mParameters(),
      mPreviewHeap(NULL),
      mRawHeap(NULL),
      mRecordHeap(NULL),
      mSecCamera(NULL),
      mRecordRunning(false),
      mCaptureInProgress(false),
      mPreviewFrameRateMicrosec(33333),
#if defined(BOARD_USES_CAMERA_OVERLAY)
      mOverlayBufferIdx(0),
#endif
      mNotifyCb(0),
      mDataCb(0),
      mDataCbTimestamp(0),
      mCallbackCookie(0),
      mMsgEnabled(0)
{
    LOGV("%s :", __func__);

    const char *camPath = CAMERA_DEV_NAME;
    const char *recPath = CAMERA_DEV_NAME2;
    mSecCamera = new SecCamera(camPath, recPath, cameraId);
    if (mSecCamera->flagCreate() == 0) {
        LOGE("%s: Failed to create SecCamera!!", __func__);
        if (mSecCamera) {
            delete mSecCamera;
            mSecCamera = NULL;
        }
        LOGE("%s: !! ABORT to create SecCameraHardware !!", __func__);
        return;
    }

    memset(&mFrameRateTimer,  0, sizeof(DurationTimer));
    memset(&mGpsInfo, 0, sizeof(gps_info));

    _initParams(cameraId);

    mExitAutoFocusThread = false;
    mExitPreviewThread = false;
    /* whether the PreviewThread is active in preview or stopped.  we
     * create the thread but it is initially in stopped state.
     */
    mPreviewRunning = false;
    mPreviewThread = new PreviewThread(this);
    mAutoFocusThread = new AutoFocusThread(this);
    mPictureThread = new PictureThread(this);
}

void SecCameraHardware::_initParams(int cameraId)
{
    LOGV("++%s :", __func__);

    if (mSecCamera == NULL) {
        LOGE("ERR(%s):mSecCamera object is NULL", __func__);
        return;
    }

    //TODO: init parm string should be defined outside from here!
    String8 strCamParam(
        "preview-size=720x480;"
        "preview-size-values=1280x720,720x480,720x480,640x480,320x240,176x144;"
        "preview-format=yuv420sp;"
        "preview-format-values=yuv420sp;"
        "preview-frame-rate=30;"
        "preview-frame-rate-values=7,15,30;"
        "preview-fps-range=15000,30000;"
        "preview-fps-range-values=(15000,30000);"
        "picture-size=2560x1920;"
        "picture-size-values=3264x2448,3264x1968,"
        "2560x1920,2048x1536,2048x1536,2048x1232,"
        "1600x1200,1600x960,"
        "800x480,640x480;"
        "picture-format=jpeg;"
        "picture-format-values=jpeg;"
        "jpeg-thumbnail-width=320;"
        "jpeg-thumbnail-height=240;"
        "jpeg-thumbnail-size-values=320x240,0x0;"
        "jpeg-thumbnail-quality=100;"
        "jpeg-quality=100;"
        "rotation=0;"
        "min-exposure-compensation=-2;"
        "max-exposure-compensation=2;"
        "exposure-compensation-step=1;"
        "exposure-compensation=0;"
        "whitebalance=auto;"
        "whitebalance-values=auto,fluorescent,warm-flurescent,daylight,cloudy-daylight;"
        "effect=none;"
        "effect-values=none,mono,negative,sepia;"
        "antibanding=auto;"
        "antibanding-values=auto,50hz,60hz,off;"
        "scene-mode=auto;"
        "scene-mode-values=auto,portrait,night,landscape,sports,party,snow,sunset,fireworks,candlelight;"
        "focus-mode=auto;"
        "focus-mode-values=auto,macro;"
        "video-frame-format=yuv420sp;"
        "focal-length=343"
    );
    mParameters.unflatten(strCamParam);
    //mParameters.dump();
}

SecCameraHardware::~SecCameraHardware()
{
    LOGV("%s :", __func__);

    this->release();

    mSecCamera = NULL;

    singleton.clear();
}

sp<IMemoryHeap> SecCameraHardware::getPreviewHeap() const
{
    return mPreviewHeap;
}

sp<IMemoryHeap> SecCameraHardware::getRawHeap() const
{
    return mRawHeap;
}

void SecCameraHardware::setCallbacks(notify_callback notify_cb,
                                     data_callback data_cb,
                                     data_callback_timestamp data_cb_timestamp,
                                     void *user)
{
    mNotifyCb        = notify_cb;
    mDataCb          = data_cb;
    mDataCbTimestamp = data_cb_timestamp;
    mCallbackCookie  = user;
}

void SecCameraHardware::enableMsgType(int32_t msgType)
{
    LOGV("%s : msgType = 0x%x, mMsgEnabled before = 0x%x",
         __func__, msgType, mMsgEnabled);
    mMsgEnabled |= msgType;
    LOGV("%s : mMsgEnabled = 0x%x", __func__, mMsgEnabled);
}

void SecCameraHardware::disableMsgType(int32_t msgType)
{
    LOGV("%s : msgType = 0x%x, mMsgEnabled before = 0x%x",
         __func__, msgType, mMsgEnabled);
    mMsgEnabled &= ~msgType;
    LOGV("%s : mMsgEnabled = 0x%x", __func__, mMsgEnabled);
}

bool SecCameraHardware::msgTypeEnabled(int32_t msgType)
{
    return (mMsgEnabled & msgType);
}

#if defined(BOARD_USES_CAMERA_OVERLAY)
bool SecCameraHardware::useOverlay()
{
    LOGV("%s: returning true", __func__);
    return true;
}

status_t SecCameraHardware::setOverlay(const sp<Overlay> &overlay)
{
    LOGV("%s :", __func__);

    if (overlay == NULL) {
        LOGV("%s : overlay == NULL", __func__);
        goto setOverlayFail;
    }
    LOGV("%s : overlay = %p", __func__, overlay->getHandleRef());

    if (overlay->getHandleRef() == NULL) {
        if (mOverlay != NULL) {
            mOverlay->destroy();
            mOverlay = NULL;
        }

        return NO_ERROR;
    }

    if (overlay->getStatus() != NO_ERROR) {
        LOGE("ERR(%s):overlay->getStatus() fail", __func__);
        goto setOverlayFail;
    }

    int overlayWidth, overlayHeight;
    mParameters.getPreviewSize(&overlayWidth, &overlayHeight);
    if (overlay->setCrop(0, 0, overlayWidth, overlayHeight) != NO_ERROR) {
        LOGE("ERR(%s)::(mOverlay->setCrop(0, 0, %d, %d) fail", __func__, overlayWidth, overlayHeight);
        goto setOverlayFail;
    }

    mOverlay = overlay;

    return NO_ERROR;

setOverlayFail :
    if (mOverlay != NULL) {
        mOverlay->destroy();
        mOverlay = NULL;
    }

    return UNKNOWN_ERROR;
}
#endif

// ---------------------------------------------------------------------------

int SecCameraHardware::previewThreadWrapper()
{
    LOGI("%s: starting", __func__);
    while (1) {
        mPreviewLock.lock();
        while (!mPreviewRunning) {
            LOGI("%s: calling mSecCamera->stopPreview() and waiting", __func__);
            mSecCamera->stopPreview();
            /* signal that we're stopping */
            mPreviewStoppedCondition.signal();
            mPreviewCondition.wait(mPreviewLock);
            LOGI("%s: return from wait", __func__);
        }
        mPreviewLock.unlock();

        if (mExitPreviewThread) {
            LOGI("%s: exiting", __func__);
            mSecCamera->stopPreview();
            return 0;
        }
        previewThread();
    }
}

int SecCameraHardware::previewThread()
{
    LOGV(" m_previewThreadFunc ");

    int index;
    unsigned int phyYAddr = 0;
    unsigned int phyCAddr = 0;

    int ret = mSecCamera->getPreviewBuffer(&index, &phyYAddr, &phyCAddr);
    if (0 > ret) {
        LOGE("%s: Faile to get PhyAddr for Preview!", __func__);
        return UNKNOWN_ERROR;
    }
    nsecs_t timestamp = systemTime(SYSTEM_TIME_MONOTONIC);

    sp<MemoryBase> previewBuffer = new MemoryBase(mPreviewHeap,
            mPreviewFrameSize * index, mPreviewFrameSize);

#if defined(BOARD_USES_CAMERA_OVERLAY)
    if (mOverlay != NULL) {
        int ret;
        struct ADDRS addr;
        mOverlayBufferIdx ^= 1;

        addr.addr_y = phyYAddr;
        addr.addr_cbcr = phyCAddr;
        addr.buf_idx = mOverlayBufferIdx;

        ret = mOverlay->queueBuffer((void *)&addr);
        if (ret == ALL_BUFFERS_FLUSHED) {
        } else if (ret == -1) {
            LOGE("ERR(%s):overlay queueBuffer fail", __func__);
        } else {
            overlay_buffer_t overlay_buffer;
            ret = mOverlay->dequeueBuffer(&overlay_buffer);
        }
    }
#endif

    // Notify the client of a new frame.
    if (mDataCb && (mMsgEnabled & CAMERA_MSG_PREVIEW_FRAME)) {
        sp<MemoryBase> previewBuffer = new MemoryBase(mPreviewHeap,
                mPreviewFrameSize * index, mPreviewFrameSize);
        if (previewBuffer != 0)
            mDataCb(CAMERA_MSG_PREVIEW_FRAME, previewBuffer, mCallbackCookie);
    }

    Mutex::Autolock lock(mRecordLock);
    if (mRecordRunning == true) {
        int ret = mSecCamera->getRecordBuffer(&index, &phyYAddr, &phyCAddr);
        if (0 > ret) {
            LOGE("%s: Failed to get PhyAddr for Record!", __func__);
            return UNKNOWN_ERROR;
        }

        struct ADDRS *addrs = (struct ADDRS *)mRecordHeap->base();
        addrs[index].addr_y    = phyYAddr;
        addrs[index].addr_cbcr = phyCAddr;
        addrs[index].buf_idx   = index;

        sp<MemoryBase> recordBuffer = new MemoryBase(mRecordHeap,
                index * sizeof(struct ADDRS), sizeof(struct ADDRS));

        // Notify the client of a new frame.
        if (mDataCbTimestamp && (mMsgEnabled & CAMERA_MSG_VIDEO_FRAME)) {
            LOGV("recording time = %lld us ", timestamp);
            mDataCbTimestamp(timestamp, CAMERA_MSG_VIDEO_FRAME, recordBuffer, mCallbackCookie);
        } else {
            mSecCamera->releaseRecordFrame(index);
        }
    }

    return NO_ERROR;
}

status_t SecCameraHardware::startPreview()
{
    Mutex::Autolock lock(mStateLock);

    mPreviewLock.lock();
    if (mPreviewRunning) {
        // already running
        LOGE("%s : preview thread already running", __func__);
        mPreviewLock.unlock();
        return INVALID_OPERATION;
    }

    if (mCaptureInProgress) {
        LOGE("%s : capture in progress, not allowed", __func__);
        return INVALID_OPERATION;
    }

    //mSecCamera->stopPreview();
    int ret  = mSecCamera->startPreview();
    if (ret < 0) {
        LOGE("ERR(%s):Fail on mSecCamera->startPreview()", __func__);
        return UNKNOWN_ERROR;
    }

    if (mPreviewHeap != NULL)
        mPreviewHeap.clear();

    mPreviewFrameSize = mSecCamera->getPreviewFrameSize();

    unsigned int previewHeapSize = mPreviewFrameSize * kBufferCount;
    mPreviewHeap = new MemoryHeapBase((int)mSecCamera->getFd(),
                                      (size_t)(previewHeapSize), (uint32_t)0);

    mPreviewRunning = true;
    mPreviewCondition.signal();
    mPreviewLock.unlock();

    LOGV("--%s \n", __func__);
    return NO_ERROR;
}

void SecCameraHardware::stopPreview()
{
    LOGV("%s :", __func__);

    /* request that the preview thread stop. */
    mPreviewLock.lock();
    if (mPreviewRunning) {
        mPreviewRunning = false;
        mPreviewCondition.signal();
        /* wait until preview thread is stopped */
        mPreviewStoppedCondition.wait(mPreviewLock);
    } else {
        LOGI("%s : preview not running, doing nothing", __func__);
    }
    mPreviewLock.unlock();
}

bool SecCameraHardware::previewEnabled()
{
    Mutex::Autolock lock(mPreviewLock);
    LOGV("%s : %d", __func__, mPreviewRunning);
    return mPreviewRunning;
}

// ---------------------------------------------------------------------------

status_t SecCameraHardware::startRecording()
{
    LOGV("%s :", __func__);

    Mutex::Autolock lock(mRecordLock);

    if (mRecordHeap == NULL) {
        int recordHeapSize = sizeof(struct ADDRS) * kBufferCount;
        LOGV("mRecordHeap : MemoryHeapBase(recordHeapSize(%d))", recordHeapSize);

        mRecordHeap = new MemoryHeapBase(recordHeapSize);
        if (mRecordHeap->getHeapID() < 0) {
            LOGE("ERR(%s): Record heap creation fail", __func__);
            mRecordHeap.clear();
            return UNKNOWN_ERROR;
        }
    }

    if (mRecordRunning == false) {
        if (mSecCamera->startRecord() < 0) {
            LOGE("ERR(%s):Fail on mSecCamera->startRecord()", __func__);
            return UNKNOWN_ERROR;
        }
        mRecordRunning = true;
    }
    return NO_ERROR;
}

void SecCameraHardware::stopRecording()
{
    LOGV("%s :", __func__);

    Mutex::Autolock lock(mRecordLock);

    if (mRecordRunning == true) {
        if (mSecCamera->stopRecord() < 0) {
            LOGE("ERR(%s):Fail on mSecCamera->stopRecord()", __func__);
            return;
        }
        mRecordRunning = false;
    }
}

bool SecCameraHardware::recordingEnabled()
{
    LOGV("%s :", __func__);

    return mRecordRunning;
}

void SecCameraHardware::releaseRecordingFrame(const sp<IMemory>& mem)
{
    LOG_CAMERA_PREVIEW("%s :", __func__);

    ssize_t offset;
    size_t size;

    sp<IMemoryHeap> heap = mem->getMemory(&offset, &size);
    struct ADDRS *addrs = (struct ADDRS *)((uint8_t *)heap->base() + offset);

    mSecCamera->releaseRecordFrame(addrs->buf_idx);
}

int SecCameraHardware::autoFocusThread()
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
        return NO_ERROR;
    }
    mFocusCondition.wait(mFocusLock);
    /* check early exit request */
    if (mExitAutoFocusThread) {
        mFocusLock.unlock();
        LOGV("%s : exiting on request1", __func__);
        return NO_ERROR;
    }
    mFocusLock.unlock();

    if (0 == (mMsgEnabled & CAMERA_MSG_FOCUS)) {
        LOGW("%s: AF not enabled by App!", __func__);
        return NO_ERROR;
    }

    LOGV("%s : calling setAutoFocus", __func__);
    if (mSecCamera->startAutoFocus() < 0) {
        return UNKNOWN_ERROR;
    }

    //sched_yield();

    bool afDone = mSecCamera->getAutoFocusResult();
    mNotifyCb(CAMERA_MSG_FOCUS, afDone, 0, mCallbackCookie);

    LOGV("%s : exiting with no error", __func__);
    return NO_ERROR;
}

status_t SecCameraHardware::autoFocus()
{
    LOGV("%s :", __func__);
    /* signal autoFocusThread to run once */
    if (mAutoFocusThread != NULL)
        mFocusCondition.signal();
    return NO_ERROR;
}

status_t SecCameraHardware::cancelAutoFocus()
{
    LOGV("%s :", __func__);

    if (mSecCamera->abortAutoFocus() < 0) {
        LOGE("ERR(%s):Fail on mSecCamera->cancelAutofocus()", __func__);
        return UNKNOWN_ERROR;
    }

    return NO_ERROR;
}

int SecCameraHardware::pictureThread()
{
    int ret = NO_ERROR;

    LOGV("doing snapshot...");
    ret = mSecCamera->startSnapshot();
    if (ret != 0) {
        LOGE("%s: Failed to do snapshot!", __func__);
        ret = UNKNOWN_ERROR;
        goto out;
    }

    if (mMsgEnabled & CAMERA_MSG_SHUTTER)
        mNotifyCb(CAMERA_MSG_SHUTTER, 0, 0, mCallbackCookie);

    mSecCamera->getSnapshot();

    if ((mMsgEnabled & CAMERA_MSG_RAW_IMAGE) && mDataCb) {
        LOGV("getting raw snapshot...");
        unsigned char *rawAddr = (unsigned char *)mRawHeap->base();
        int rawSize = mRawHeap->getSize();
        mSecCamera->getRawSnapshot(rawAddr, rawSize);
        // TODO: send raw image via mDataCb
    }

    LOGV("copying meta datas...");
    if (0 <= m_getGpsInfo(&mParameters, &mGpsInfo)) {
        if (mSecCamera->setGpsInfo(mGpsInfo.latitude,  mGpsInfo.longitude,
                                   mGpsInfo.timestamp, mGpsInfo.altitude) < 0) {
            LOGE("%s::setGpsInfo fail.. but making jpeg is progressing\n", __func__);
        }
    }

    if (mDataCb && (mMsgEnabled & CAMERA_MSG_COMPRESSED_IMAGE)) {
        LOGV("getting jpeg snapshot...");
        // TODO: the heap size too enough for jpeg..
        int jpegHeapSize = mSecCamera->getSnapshotFrameSize();
        sp<MemoryHeapBase> jpegHeap = new MemoryHeapBase(jpegHeapSize);
        int jpegSize = mSecCamera->getJpegSnapshot((unsigned char *)jpegHeap->base(),
                       jpegHeapSize);

        // TODO: add exif and thumbanil in jpeg
        sp<MemoryBase> jpegMem = new MemoryBase(jpegHeap, 0, jpegSize);
        mDataCb(CAMERA_MSG_COMPRESSED_IMAGE, jpegMem, mCallbackCookie);
    }

    LOGV("snapshot done");
out:
    ret = mSecCamera->endSnapshot();

    mStateLock.lock();
    mCaptureInProgress = false;
    mStateLock.unlock();

    return ret;
}

status_t SecCameraHardware::takePicture()
{
    LOGV("++%s :", __func__);

    stopPreview();

    Mutex::Autolock lock(mStateLock);
    if (mCaptureInProgress) {
        LOGE("%s : capture already in progress", __func__);
        return INVALID_OPERATION;
    }

    if (mPictureThread->run("CameraPictureThread", PRIORITY_DEFAULT) != NO_ERROR) {
        LOGE("%s : couldn't run picture thread", __func__);
        return INVALID_OPERATION;
    }
    mCaptureInProgress = true;

    return NO_ERROR;
}

status_t SecCameraHardware::cancelPicture()
{
    return NO_ERROR;
}

bool SecCameraHardware::_isParamUpdated(const CameraParameters &newParams,
                                        const char *key, const char *newValue) const
{
    if (NULL == newValue) {
        LOGV("%s: no value to compare!!", __func__);
        return false;
    }

    const char *currValue = mParameters.get(key);
    if (NULL == currValue) {
        LOGV("mParameters has no key, %s", key);
        return false;
    }

    if (0 != strcmp(newValue, currValue)) {
        LOGI("CameraParameter, %s updated: %s->%s", key, currValue, newValue);
        return true;
    }

    return false;
}

bool SecCameraHardware::_isParamUpdated(const CameraParameters &newParams,
                                        const char *key, int newValue) const
{
    if (NULL == mParameters.get(key)) {
        LOGV("%s: mParameters has no key, %s", __func__, key);
        return false;
    }

    int currValue = mParameters.getInt(key);

    if (currValue != newValue) {
        LOGI("CameraParameter, %s updated: %d->%d", key, currValue, newValue);
        return true;
    }

    return false;
}

status_t SecCameraHardware::setParameters(const CameraParameters &params)
{
    LOGV("++%s :", __func__);

    status_t ret = NO_ERROR;
    const char *strKey;
    int width, height, quality;
    const char *strSize;
    const char *strPixfmt;
    int err = 0;

    /* if someone calls us while picture thread is running, it could screw
     * up the sensor quite a bit so return error.  we can't wait because
     * that would cause deadlock with the callbacks
     */
    mStateLock.lock();
    if (mCaptureInProgress) {
        mStateLock.unlock();
        LOGW("%s : capture in progress, not allowed", __func__);
        params.dump();
        return NO_ERROR;
    }
    mStateLock.unlock();

    // preview-size and preview-format
    strSize = params.get(CameraParameters::KEY_PREVIEW_SIZE);
    strPixfmt = params.get(CameraParameters::KEY_PREVIEW_FORMAT);
    if (!mPreviewRunning
            || _isParamUpdated(params, CameraParameters::KEY_PREVIEW_SIZE, strSize)
            || _isParamUpdated(params, CameraParameters::KEY_PREVIEW_FORMAT, strPixfmt)) {
        LOGV("setting preview format to %dx%d(%s)...", width, height, strPixfmt);
        params.getPreviewSize(&width, &height);
        err = mSecCamera->setPreviewFormat(width, height, strPixfmt);
        if (!err) {
#if defined(BOARD_USES_CAMERA_OVERLAY)
            if (mOverlay != NULL
                    && mOverlay->setCrop(0, 0, width, height) != NO_ERROR) {
                LOGE("ERR(%s)::(mOverlay->setCrop(0, 0, %d, %d) fail",
                     __func__, width, height);
                return UNKNOWN_ERROR;
            }
#endif
            mParameters.setPreviewSize(width, height);
            mParameters.setPreviewFormat(strPixfmt);
        }
    }

    // picture-size and picture-format
    strSize = params.get(CameraParameters::KEY_PICTURE_SIZE);
    strPixfmt = params.get(CameraParameters::KEY_PICTURE_FORMAT);
    if (!mPreviewRunning
            || _isParamUpdated(params, CameraParameters::KEY_PICTURE_SIZE, strSize)
            || _isParamUpdated(params, CameraParameters::KEY_PICTURE_FORMAT, strPixfmt)) {
        LOGV("setting picture format to %dx%d(%s)...", width, height, strPixfmt);
        params.getPictureSize(&width, &height);
        err = mSecCamera->setSnapshotFormat(width, height, strPixfmt);
        if (!err) {
            int rawHeapSize = mSecCamera->getSnapshotFrameSize();
            LOGV("mRawHeap : MemoryHeapBase(previewHeapSize(%d))", rawHeapSize);
            mRawHeap = new MemoryHeapBase(rawHeapSize);
            if (mRawHeap->getHeapID() < 0) {
                LOGE("ERR(%s): Raw heap creation fail", __func__);
                mRawHeap.clear();
                return UNKNOWN_ERROR;
            }
            mParameters.setPictureSize(width, height);
            mParameters.setPictureFormat(strPixfmt);
        }
    }

    // jpeg-thumbnail-width and jpeg-thumbnail-height
    width  = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH);
    height = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT);
    if (_isParamUpdated(params, CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, width)
            || _isParamUpdated(params, CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, height)) {
        LOGV("setting thumbnail size to %dx%d", width, height);
        err = mSecCamera->setJpegThumbnailSize(width, height);
        if (!err) {
            mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, width);
            mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, height);
        }
    }

    // jpeg-thumbnail-quality
    strKey = CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY;
    // TODO: add code for thumbnail-quality

    // jpeg-quality
    strKey = CameraParameters::KEY_JPEG_QUALITY;
    quality = params.getInt(strKey);
    if (_isParamUpdated(params, strKey, quality)) {
        LOGV("setting %s to %d...", strKey, quality);
        err = mSecCamera->setJpegQuality(quality);
        if (!err)
            mParameters.set(strKey, quality);
    }

    // rotation
    strKey = CameraParameters::KEY_ROTATION;
    int rot = params.getInt(strKey);
    if (_isParamUpdated(params, strKey, rot)) {
        LOGV("setting %s to %d...", strKey, rot);
        err = mSecCamera->setRotate(rot);
        if (!err)
            mParameters.set(strKey, rot);
    }

    // flash-mode
    strKey = CameraParameters::KEY_FLASH_MODE;
    const char *strFlashMode = params.get(strKey);
    if (_isParamUpdated(params, strKey, strFlashMode)) {
        LOGV("Setting flash-mode to %s...", strFlashMode);
        err = mSecCamera->setFlashMode(strFlashMode);
        if (!err)
            mParameters.set(strKey, strFlashMode);
    }

    // scene-mode
    strKey = CameraParameters::KEY_SCENE_MODE;
    const char *strSceneMode = params.get(strKey);
    if (_isParamUpdated(params, strKey, strSceneMode)) {
        LOGV("setting scene-mode to %s...", strSceneMode);
        err = mSecCamera->setSceneMode(strSceneMode);
        if (!err)
            mParameters.set(strKey, strSceneMode);
    }

    // whitebalance
    strKey = CameraParameters::KEY_WHITE_BALANCE;
    const char *strWhiteBalance = params.get(strKey);
    if (_isParamUpdated(params, strKey, strWhiteBalance)) {
        LOGV("setting whitebalance to %s...", strWhiteBalance);
        err = mSecCamera->setWhiteBalance(strWhiteBalance);
        if (!err)
            mParameters.set(strKey, strWhiteBalance);
    }

    // effect
    strKey = CameraParameters::KEY_EFFECT;
    const char *strEffect = params.get(strKey);
    if (_isParamUpdated(params, strKey, strEffect)) {
        LOGV("setting effect to %s...", strEffect);
        err = mSecCamera->setEffect(strEffect);
        if (!err)
            mParameters.set(strKey, strEffect);
    }

    // exposure-compensation
    strKey = CameraParameters::KEY_EXPOSURE_COMPENSATION;
    int nEv = params.getInt(strKey);
    if (_isParamUpdated(params, strKey, nEv)) {
        LOGV("setting ev to %d...", nEv);
        err = mSecCamera->setBrightness(nEv);
        if (!err)
            mParameters.set(strKey, nEv);
    }

    // frame rate
    int new_frame_rate = params.getPreviewFrameRate();
    if (new_frame_rate < 5 || new_frame_rate > 30)
        new_frame_rate = 30;

    mParameters.setPreviewFrameRate(new_frame_rate);
    // Calculate how long to wait between frames.
    mPreviewFrameRateMicrosec = (int)(1000000.0f / float(new_frame_rate));

    LOGV("frame rate:%d, mPreviewFrameRateMicrosec:%d", new_frame_rate, mPreviewFrameRateMicrosec);
#if 0
    mSecCamera->setFrameRate(new_frame_rate);
#endif

    // fps range
    // TODO: need codes for fps range

    LOGV("--%s : err = %d", __func__, err);
    return err ? UNKNOWN_ERROR : NO_ERROR;
}

CameraParameters SecCameraHardware::getParameters() const
{
    LOGV("%s :", __func__);
    return mParameters;
}

status_t SecCameraHardware::sendCommand(int32_t command, int32_t arg1,
                                        int32_t arg2)
{
    return BAD_VALUE;
}

void SecCameraHardware::release()
{
    LOGV("%s :", __func__);

    /* shut down any threads we have that might be running.  do it here
     * instead of the destructor.  we're guaranteed to be on another thread
     * than the ones below.  if we used the destructor, since the threads
     * have a reference to this object, we could wind up trying to wait
     * for ourself to exit, which is a deadlock.
     */
    if (mPreviewThread != NULL) {
        /* this thread is normally already in it's threadLoop but blocked
         * on the condition variable or running.  signal it so it wakes
         * up and can exit.
         */
        mPreviewThread->requestExit();
        mExitPreviewThread = true;
        mPreviewRunning = true; /* let it run so it can exit */
        mPreviewCondition.signal();
        mPreviewThread->requestExitAndWait();
        mPreviewThread.clear();
    }

    if (mAutoFocusThread != NULL) {
        /* this thread is normally already in it's threadLoop but blocked
         * on the condition variable.  signal it so it wakes up and can exit.
         */
        mFocusLock.lock();
        mAutoFocusThread->requestExit();
        mExitAutoFocusThread = true;
        mFocusCondition.signal();
        mFocusLock.unlock();
        mAutoFocusThread->requestExitAndWait();
        mAutoFocusThread.clear();
    }

    if (mPictureThread != NULL) {
        mPictureThread->requestExitAndWait();
        mPictureThread.clear();
    }

    if (mRawHeap != NULL)
        mRawHeap.clear();

    if (mPreviewHeap != NULL) {
        LOGI("%s: calling mPreviewHeap.dispose()", __func__);
        mPreviewHeap->dispose();
        mPreviewHeap.clear();
    }

    if (mRecordHeap != NULL)
        mRecordHeap.clear();

#if defined(BOARD_USES_CAMERA_OVERLAY)
    if (mOverlay != NULL) {
        mOverlay->destroy();
        mOverlay = NULL;
    }
#endif

    /* close after all the heaps are cleared since those
     * could have dup'd our file descriptor.
     */
    if (mSecCamera != NULL) {
        delete mSecCamera;
        //mSecCamera->Destroy();
        mSecCamera = NULL;
    }
}

status_t SecCameraHardware::dump(int fd, const Vector<String16>& args) const
{
    // TODO: fill dump!

    return NO_ERROR;
}

// ---------------------------------------------------------------------------

int SecCameraHardware::m_getGpsInfo(CameraParameters *camParams, gps_info *gps)
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

wp<CameraHardwareInterface> SecCameraHardware::singleton;

sp<CameraHardwareInterface> SecCameraHardware::createInstance(int cameraId)
{
    LOGV("%s :", __func__);
    if (singleton != 0) {
        sp<CameraHardwareInterface> hardware = singleton.promote();
        if (hardware != 0) {
            return hardware;
        }
    }
    sp<CameraHardwareInterface> hardware(new SecCameraHardware(cameraId));
    singleton = hardware;
    return hardware;
}

}; // namespace android
