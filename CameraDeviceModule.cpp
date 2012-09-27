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
#define LOG_TAG "CameraDevice"
#include <utils/Log.h>

#include "CameraHardware.h"
#include "CameraFactory.h"
#include "CameraLog.h"

namespace android {

static camera_device_t* g_dev = NULL;
#define CALL_HW(F, ...) \
    ((reinterpret_cast<CameraHardware *>(g_dev->priv))->F(__VA_ARGS__))


static int cam_dev_set_preview_window(struct camera_device* dev,
                                      struct preview_stream_ops* window)
{
    LOG_CAMERA_FUNC_ENTER;
    if (dev->priv == NULL) {
        LOGW_IF(window, "camera device already released and, "
                 "cam_dev_set_preview_window will be ignored!");
        return 0;
    }

    return CALL_HW(setPreviewWindow, window);

}

static void cam_dev_set_callbacks(struct camera_device* dev,
                                  camera_notify_callback notify_cb,
                                  camera_data_callback data_cb,
                                  camera_data_timestamp_callback data_cb_ts,
                                  camera_request_memory req_memory,
                                  void* user)
{
    LOG_CAMERA_FUNC_ENTER;
    CALL_HW(setCallbacks,
            notify_cb, data_cb, data_cb_ts, req_memory, user);

}

static void cam_dev_enable_msg_type(struct camera_device* dev,
                                    int32_t msg_type)
{
    LOG_CAMERA_FUNC_ENTER;
    CALL_HW(enableMsgType, msg_type);
}

static void cam_dev_disable_msg_type(struct camera_device* dev,
                                     int32_t msg_type)
{
    LOG_CAMERA_FUNC_ENTER;
    CALL_HW(disableMsgType, msg_type);
}

static int cam_dev_msg_type_enabled(struct camera_device* dev,
                                    int32_t msg_type)
{
    LOG_CAMERA_FUNC_ENTER;
    return CALL_HW(msgTypeEnabled, msg_type);
}

static int cam_dev_start_preview(struct camera_device* dev)
{
    LOG_CAMERA_FUNC_ENTER;
    return CALL_HW(startPreview);
}

static void cam_dev_stop_preview(struct camera_device* dev)
{
    LOG_CAMERA_FUNC_ENTER;
    CALL_HW(stopPreview);

}

static int cam_dev_preview_enabled(struct camera_device* dev)
{
    LOG_CAMERA_FUNC_ENTER;
    return CALL_HW(previewEnabled);
}

static int cam_dev_store_meta_data_in_buffers(struct camera_device* dev,
        int enable)
{
    LOG_CAMERA_FUNC_ENTER;
    return CALL_HW(storeMetaDataInBuffers, enable);
}

static int cam_dev_start_recording(struct camera_device* dev)
{
    LOG_CAMERA_FUNC_ENTER;
    return CALL_HW(startRecording);
}

static void cam_dev_stop_recording(struct camera_device* dev)
{
    LOG_CAMERA_FUNC_ENTER;
    CALL_HW(stopRecording);

}

static int cam_dev_recording_enabled(struct camera_device* dev)
{
    LOG_CAMERA_FUNC_ENTER;
    return CALL_HW(recordingEnabled);
}

static void cam_dev_release_recording_frame(struct camera_device* dev,
        const void* opaque)
{
    LOG_CAMERA_FUNC_ENTER;
    CALL_HW(releaseRecordingFrame, opaque);
}

static int cam_dev_auto_focus(struct camera_device* dev)
{
    LOG_CAMERA_FUNC_ENTER;
    return CALL_HW(autoFocus);
}

static int cam_dev_cancel_auto_focus(struct camera_device* dev)
{
    LOG_CAMERA_FUNC_ENTER;
    return CALL_HW(cancelAutoFocus);
}

static int cam_dev_take_picture(struct camera_device* dev)
{
    LOG_CAMERA_FUNC_ENTER;
    return CALL_HW(takePicture);
}

static int cam_dev_cancel_picture(struct camera_device* dev)
{
    LOG_CAMERA_FUNC_ENTER;
    return CALL_HW(cancelPicture);
}

static int cam_dev_set_parameters(struct camera_device* dev, const char* parms)
{
    LOG_CAMERA_FUNC_ENTER;
    String8 str(parms);
    CameraParameters p(str);
    return CALL_HW(setParameters, p);
}

static char* cam_dev_get_parameters(struct camera_device* dev)
{
    LOG_CAMERA_FUNC_ENTER;
    CameraParameters parms = CALL_HW(getParameters);
    String8 str = parms.flatten();
    char* ret = strdup(str.string());

    return ret;
}

static void cam_dev_put_parameters(struct camera_device* dev, char* parms)
{
    LOG_CAMERA_FUNC_ENTER;
    free(parms);
}

static int cam_dev_send_command(struct camera_device* dev,
                                int32_t cmd, int32_t arg1, int32_t arg2)
{
    LOG_CAMERA_FUNC_ENTER;
    return CALL_HW(sendCommand, cmd, arg1, arg2);
}

static void cam_dev_release(struct camera_device* dev)
{
    LOG_CAMERA_FUNC_ENTER;
    //CALL_HW(release);
    if (dev->priv) {
        delete static_cast<CameraHardware*>(dev->priv);
        dev->priv = NULL;
    }

}

static int cam_dev_dump(struct camera_device* dev, int fd)
{
    LOG_CAMERA_FUNC_ENTER;
    return CALL_HW(dump, fd);
}

#define SET_METHOD(m) m : cam_dev_##m
static camera_device_ops_t g_dev_ops = {
    SET_METHOD(set_preview_window),
    SET_METHOD(set_callbacks),
    SET_METHOD(enable_msg_type),
    SET_METHOD(disable_msg_type),
    SET_METHOD(msg_type_enabled),
    SET_METHOD(start_preview),
    SET_METHOD(stop_preview),
    SET_METHOD(preview_enabled),
    SET_METHOD(store_meta_data_in_buffers),
    SET_METHOD(start_recording),
    SET_METHOD(stop_recording),
    SET_METHOD(recording_enabled),
    SET_METHOD(release_recording_frame),
    SET_METHOD(auto_focus),
    SET_METHOD(cancel_auto_focus),
    SET_METHOD(take_picture),
    SET_METHOD(cancel_picture),
    SET_METHOD(set_parameters),
    SET_METHOD(get_parameters),
    SET_METHOD(put_parameters),
    SET_METHOD(send_command),
    SET_METHOD(release),
    SET_METHOD(dump),
};
#undef SET_METHOD

static int cam_dev_close(struct hw_device_t* device)
{
    LOG_CAMERA_FUNC_ENTER;
    if (device != (hw_device_t*)g_dev) {
        LOGE("%s: got device %p. expect %p!", __func__, device, g_dev);
        return -EINVAL;
    }

    if (g_dev == NULL) {
        LOGE("%s: Camera deivce already closed. Do nothing!", __func__);
        goto exit;
    }

    if (g_dev->priv) {
        delete static_cast<CameraHardware*>(g_dev->priv);
        g_dev->priv = NULL;
    }

    delete g_dev;
    g_dev = NULL;

exit:
    return 0;
}

static camera_device_t* cam_dev_get_instance(int id)
{
    if (g_dev != NULL) {
        // TODO: check opened ch id
    }

    g_dev = new camera_device_t();
    if (g_dev == NULL) {
        LOGE("%s: Fail to alloc mem for cam_dev_t!", __func__);
        goto done;
    }

    g_dev->common.tag = HARDWARE_DEVICE_TAG;
    g_dev->common.version = 1;
    g_dev->common.close = cam_dev_close;

    g_dev->ops = &g_dev_ops;
    g_dev->priv = new CameraHardware(id);
    if (g_dev->priv == NULL) {
        LOGE("%s: Fail to alloc mem for CameraHardware!", __func__);
        cam_dev_close((hw_device_t*)g_dev);
    }

done:
    return g_dev;
}

static int camera_module_open(const struct hw_module_t* module,
                              const char* id,
                              struct hw_device_t** device)
{
    int camera_id = atoi(id);
    LOGI("Opening camera device...");
    camera_device_t* camera_device = cam_dev_get_instance(camera_id);
    if (camera_device == NULL) {
        LOGE("%s: failed to get camera_device for id, %d",
              __func__, camera_id);
        return -1;
    }

    LOGI("Connecting module with device...");
    camera_device->common.module = const_cast<hw_module_t*>(module);

    *device = (hw_device_t*)camera_device;

    LOGI("Camera module opened");
    return 0;
}

static hw_module_methods_t camera_module_methods = {
    open : camera_module_open,
};

extern "C" {
    struct camera_module HAL_MODULE_INFO_SYM = {
        common : {
            tag           : HARDWARE_MODULE_TAG,
            version_major : 1,
            version_minor : 0,
            id            : CAMERA_HARDWARE_MODULE_ID,
            name          : "Insignal Camera HAL",
            author        : "Homin Lee <suapapa@insignal.co.kr>",
            methods       : &camera_module_methods,
            dso           : NULL,
            reserved      : {0},
        },
        get_number_of_cameras : camera_info_get_number_of_cameras,
        get_camera_info       : camera_info_get_camera_info,
    };
}

};
