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
#include <hardware/camera.h>
#include <camera/CameraParameters.h>
#include <utils/threads.h>

namespace android {

class SecCameraHardware {

public:
    void        setCallbacks(camera_notify_callback notify_cb,
                             camera_data_callback data_cb,
                             camera_data_timestamp_callback data_cb_timestamp,
                             camera_request_memory req_memory,
                             void* user);

    void        enableMsgType(int32_t msgType);
    void        disableMsgType(int32_t msgType);
    bool        msgTypeEnabled(int32_t msgType);

    status_t    setPreviewWindow(preview_stream_ops* window);
    status_t    storeMetaDataInBuffers(bool enable);

    status_t    startPreview();
    void        stopPreview();
    bool        previewEnabled();

    status_t    startRecording();
    void        stopRecording();
    bool        recordingEnabled();
    void        releaseRecordingFrame(const void* opaque);

    status_t    autoFocus();
    status_t    cancelAutoFocus();

    status_t    takePicture();
    status_t    cancelPicture();

    status_t    setParameters(const CameraParameters& parms);
    CameraParameters  getParameters() const;
    status_t    sendCommand(int32_t command, int32_t arg1, int32_t arg2);
    int         getCameraId(void) const;

    void        release();
    status_t    dump(int fd) const;

    SecCameraHardware(int cameraId);
    virtual    ~SecCameraHardware();
private:

#define DEFINE_THREAD(N, P, L)                                  \
    bool L();                                                   \
    class N : public Thread {                                   \
        SecCameraHardware* _hw;                                 \
    public:                                                     \
        N(SecCameraHardware* hw):Thread(false), _hw(hw) { }     \
        virtual bool threadLoop() { return _hw->L(); }          \
        status_t startLoop() { return run("Camera"#N, P); }     \
    }

    DEFINE_THREAD(PreviewThread, PRIORITY_URGENT_DISPLAY, _previewLoop);
    DEFINE_THREAD(AutoFocusThread, PRIORITY_DEFAULT, _autofocusLoop);
    DEFINE_THREAD(PictureThread, PRIORITY_DEFAULT, _pictureLoop);

#undef DEFINE_THREAD


//------------------------------------------
    enum previewState {
        PREVIEW_IDLE = 0,
        PREVIEW_PENDING,
        PREVIEW_RUNNING,
        PREVIEW_RECORDING,
        PREVIEW_ABORT,
        PREVIEW_INVALID
    };
    enum previewState   _previewState;
    mutable Mutex       _previewLock;

    sp<PreviewThread>   _previewThread;

    /* used by preview thread to block until it's told to run */
    mutable Condition   _previewConditionStateChanged;
    mutable Condition   mPreviewStoppedCondition;

    status_t            _startPreviewLocked(void);
    void                _stopPreviewLocked(void);
//------------------------------------------
    enum focusState {
        FOCUS_IDLE = 0,
        FOCUS_RUN_AF,
        FOCUS_RUN_CAF,
        FOCUS_INVALID
    };
    enum focusState     _focusState;
    mutable Mutex       _focusLock;

    sp<AutoFocusThread> _autofocusThread;

    /* used by auto focus thread to block until it's told to run */
    mutable Mutex       mFocusLock;
    mutable Condition   mFocusCondition;
    bool                mExitAutoFocusThread;

//------------------------------------------
    enum pictureState {
        PICTURE_IDLE = 0,
        PICTURE_CAPTURING,
        PICTURE_COMPRESSING,
        PICTURE_INVALID
    };
    enum pictureState   _pictureState;
    mutable Mutex       _pictureLock;
    mutable Condition   mCaptureCondition;

    sp<PictureThread>   _pictureThread;

    status_t 		_waitPictureComplete(void);
//------------------------------------------

    int                 _cameraId;

    CameraParameters    _parms;

    camera_memory_t*    _previewHeap;
    camera_memory_t*    _rawHeap;
    camera_memory_t*    _recordHeap;

    SecCamera*          _camera;

    preview_stream_ops* _window;

    camera_notify_callback              _cbNotify;
    camera_data_callback                _cbData;
    camera_data_timestamp_callback      _cbDataWithTS;
    camera_request_memory               _cbReqMemory;
    void*                               _cbCookie;

    int32_t             _msgs;

    void        _initParams(void);
    bool        _isParamUpdated(const CameraParameters& newParams,
                                const char* key, const char* newValue) const;
    bool        _isParamUpdated(const CameraParameters& newParams,
                                const char* key, int newValue) const;

    int		_fillWindow(char* previewFrame, int width, int height);
};

}; // namespace android

#endif
