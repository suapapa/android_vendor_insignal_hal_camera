/*
**
** Copyright 2008, The Android Open Source Project
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

#ifndef __ANDROID_HARDWARE_LIBCAMERA_SEC_CAMERA_HARDWARE_H__
#define __ANDROID_HARDWARE_LIBCAMERA_SEC_CAMERA_HARDWARE_H__

#include "SecCamera.h"
#include <utils/threads.h>
#include <camera/CameraHardwareInterface.h>
#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>
#include <utils/threads.h>

namespace android {

class SecCameraHardware : public CameraHardwareInterface {

public:
    virtual sp<IMemoryHeap> getPreviewHeap() const;
    virtual sp<IMemoryHeap> getRawHeap() const;

    virtual void        setCallbacks(notify_callback notify_cb,
            data_callback data_cb,
            data_callback_timestamp data_cb_timestamp,
            void* user);

    virtual void        enableMsgType(int32_t msgType);
    virtual void        disableMsgType(int32_t msgType);
    virtual bool        msgTypeEnabled(int32_t msgType);

#if defined(BOARD_USES_CAMERA_OVERLAY)
    virtual bool        useOverlay();
    virtual status_t    setOverlay(const sp<Overlay> &overlay);
#endif

    virtual status_t    startPreview();
    virtual void        stopPreview();
    virtual bool        previewEnabled();

    virtual status_t    startRecording();
    virtual void        stopRecording();
    virtual bool        recordingEnabled();
    virtual void        releaseRecordingFrame(const sp<IMemory>& mem);

    virtual status_t    autoFocus();
    virtual status_t    cancelAutoFocus();

    virtual status_t    takePicture();
    virtual status_t    cancelPicture();

    virtual status_t    setParameters(const CameraParameters& params);
    virtual CameraParameters  getParameters() const;
    virtual status_t    sendCommand(int32_t command,
                                    int32_t arg1, int32_t arg2);

    virtual void        release();
    virtual status_t    dump(int fd, const Vector<String16>& args) const;

    static sp<CameraHardwareInterface> createInstance(int cameraId);

private:
    SecCameraHardware(int cameraId);
    virtual    ~SecCameraHardware();

    class PreviewThread : public Thread {
        SecCameraHardware *mHardware;
    public:
        PreviewThread(SecCameraHardware *hw):
        Thread(false),
        mHardware(hw) { }
        virtual void onFirstRef() {
            run("CameraPreviewThread", PRIORITY_URGENT_DISPLAY);
        }
        virtual bool threadLoop() {
            mHardware->previewThreadWrapper();
            return false;
        }
    };

    class PictureThread : public Thread {
        SecCameraHardware *mHardware;
    public:
        PictureThread(SecCameraHardware *hw):
        Thread(false),
        mHardware(hw) { }
        virtual bool threadLoop() {
            mHardware->pictureThread();
            return false;
        }
    };

    class AutoFocusThread : public Thread {
        SecCameraHardware *mHardware;
    public:
        AutoFocusThread(SecCameraHardware *hw):
        Thread(false),
        mHardware(hw) { }
        virtual void onFirstRef() {
            run("CameraAutoFocusThread", PRIORITY_DEFAULT);
        }
        virtual bool threadLoop() {
            mHardware->autoFocusThread();
            return true;
        }
    };

    typedef struct {
        double       latitude;   // degrees, WGS ellipsoid
        double       longitude;  // degrees
        unsigned int timestamp;  // seconds since 1/6/1980
        int          altitude;   // meters
    } gps_info;

    static wp<CameraHardwareInterface> singleton;

    /* used to guard threading state */
    mutable Mutex       mStateLock;

    CameraParameters    mParameters;

    static const int    kBufferCount = MAX_BUFFERS;
    static const int    kBufferCountForRecord = MAX_BUFFERS;

    sp<MemoryHeapBase>  mPreviewHeap;
    sp<MemoryHeapBase>  mRawHeap;
    sp<MemoryHeapBase>  mRecordHeap;
    sp<MemoryBase>      mBuffers      [kBufferCount];

    unsigned int        mPreviewFrameSize;

    SecCamera          *mSecCamera;
    bool                mPreviewRunning;
    bool                mExitPreviewThread;

    bool                mRecordRunning;
    mutable Mutex       mRecordLock;

    bool		mCaptureInProgress;

    unsigned int        mPreviewFrameRateMicrosec;
    DurationTimer       mFrameRateTimer;

#if defined(BOARD_USES_CAMERA_OVERLAY)
    sp<Overlay>         mOverlay;
    int                 mOverlayBufferIdx;
#endif

    notify_callback     mNotifyCb;
    data_callback       mDataCb;
    data_callback_timestamp mDataCbTimestamp;
    void *              mCallbackCookie;

    int32_t             mMsgEnabled;

    int                 mAFMode;

    gps_info            mGpsInfo;

    void       _initParams(int cameraId);
    bool       _isParamUpdated(const CameraParameters &newParams,
                               const char *key, const char *newValue) const;
    bool       _isParamUpdated(const CameraParameters &newParams,
                               const char *key, int newValue) const;

    sp<PreviewThread>   mPreviewThread;
    int                 previewThread();
    int                 previewThreadWrapper();

    /* used by preview thread to block until it's told to run */
    mutable Mutex       mPreviewLock;
    mutable Condition   mPreviewCondition;
    mutable Condition   mPreviewStoppedCondition;

    sp<AutoFocusThread> mAutoFocusThread;
    int                 autoFocusThread();

    /* used by auto focus thread to block until it's told to run */
    mutable Mutex       mFocusLock;
    mutable Condition   mFocusCondition;
    bool                mExitAutoFocusThread;

    sp<PictureThread>   mPictureThread;
    int                 pictureThread();

    int        m_getGpsInfo(CameraParameters * camParams, gps_info * gps);
};

}; // namespace android

#endif
