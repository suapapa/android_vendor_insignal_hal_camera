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

#ifndef __ANDROID_HARDWARE_LIBCAMERA_CAMERA_HARDWARE_H__
#define __ANDROID_HARDWARE_LIBCAMERA_CAMERA_HARDWARE_H__

#include "SecCamera.h"
#include <hardware/camera.h>
#include <camera/CameraParameters.h>
#include <utils/threads.h>

namespace android {

class CameraHardware {

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

    status_t    setParameters(const CameraParameters& parms, bool needInit = false);
    CameraParameters  getParameters() const;
    status_t    sendCommand(int32_t command, int32_t arg1, int32_t arg2);
    int         getCameraId(void) const;

    void        release();
    status_t    dump(int fd) const;

    CameraHardware(int cameraId);
    virtual    ~CameraHardware();

private:
    int                 _cameraId;
    SecCamera*          _camera;

    int32_t             _msgs;
    camera_memory_t*    _previewHeap;
    camera_memory_t*    _rawHeap;
    camera_memory_t*    _recordHeap;


    camera_notify_callback              _cbNotify;
    camera_data_callback                _cbData;
    camera_data_timestamp_callback      _cbDataWithTS;
    camera_request_memory               _cbReqMemory;
    void*                               _cbCookie;

    CameraParameters    _parms;
    void                _initParams(void);
    bool                _isParamUpdated(const CameraParameters& newParams,
                                        const char* key,
                                        const char* newValue) const;
    bool                _isParamUpdated(const CameraParameters& newParams,
                                        const char* key,
                                        int newValue) const;

    preview_stream_ops* _window;
    status_t            _fillWindow(const char* previewFrame,
                                    int width, int height,
                                    const char* strPixfmt);

#define DEFINE_THREAD(N, P, L)                                  \
    bool L();                                                   \
    class N : public Thread {                                   \
        CameraHardware* _hw;                                    \
    public:                                                     \
        N(CameraHardware* hw):Thread(false), _hw(hw) { }        \
        virtual bool threadLoop() { return _hw->L(); }          \
        status_t startLoop() { return run("Camera"#N, P); }     \
    }

    DEFINE_THREAD(PreviewThread, PRIORITY_URGENT_DISPLAY, _previewLoop);
    sp<PreviewThread>   _previewThread;
    enum previewState {
        PREVIEW_IDLE = 0,
        PREVIEW_PENDING,
        PREVIEW_RUNNING,
        PREVIEW_RECORDING,
        PREVIEW_ABORT,
        PREVIEW_INVALID
    };
    enum previewState   _previewState;
    mutable Condition   _previewStateChangedCondition;
    mutable Condition   _previewStoppedCondition;
    mutable Mutex       _previewLock;
    status_t            _startPreviewLocked(void);
    void                _stopPreviewLocked(void);

    DEFINE_THREAD(FocusThread, PRIORITY_DEFAULT, _focusLoop);
    sp<FocusThread> _focusThread;
    enum focusState {
        FOCUS_IDLE = 0,
        FOCUS_RUN_AF,
        FOCUS_RUN_CAF,
        FOCUS_GET_RESULT,
        FOCUS_ABORT,
        FOCUS_INVALID
    };
    enum focusState     _focusState;
    mutable Condition   _focusStateChangedCondition;
    mutable Mutex       _focusLock;


    DEFINE_THREAD(PictureThread, PRIORITY_DEFAULT, _pictureLoop);
    sp<PictureThread>   _pictureThread;
    enum pictureState {
        PICTURE_IDLE = 0,
        PICTURE_CAPTURING,
        PICTURE_COMPRESSING,
        PICTURE_INVALID
    };
    enum pictureState   _pictureState;
    mutable Condition   _pictureStateChangedCondition;
    mutable Mutex       _pictureLock;
    status_t            _waitPictureComplete(void);

#undef DEFINE_THREAD

};

}; // namespace android

#endif
