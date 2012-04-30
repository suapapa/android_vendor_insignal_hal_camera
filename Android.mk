# When zero we link against libqcamera; when 1, we dlopen libqcamera.
#ifeq ($(BOARD_CAMERA_LIBRARIES),libcamera)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# Camera Info for Origenboard
# TODO: Fix it cat to select custom info
LOCAL_SRC_FILES += \
	CameraInfo.cpp

LOCAL_SRC_FILES += \
	CameraHardware.cpp \
	SecCamera.cpp \
	SecV4L2Adapter.cpp \

LOCAL_SRC_FILES += \
        CameraDevice.cpp \
        CameraModule.cpp

# for videodev2_samsung.h and jpeg_api.h
#LOCAL_C_INCLUDES += $(LOCAL_PATH)/../include
LOCAL_C_INCLUDES += hardware/samsung/exynos4/hal/include
LOCAL_SHARED_LIBRARIES := libutils libui liblog libbinder libdl libcutils
LOCAL_SHARED_LIBRARIES += libhardware libcamera_client

LOCAL_C_INCLUDES += external/libyuv/files/include
LOCAL_STATIC_LIBRARIES += libyuv_static
#LOCAL_CFLAGS :=-fno-short-enums

#ifeq ($(BOARD_USE_JPEG),true)
LOCAL_SRC_FILES += \
	ExynosHWJpeg.cpp

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libs5pjpeg
LOCAL_SHARED_LIBRARIES += libs5pjpeg
#endif

#LOCAL_MODULE_TAGS := eng
#LOCAL_MODULE := libcamera

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := camera.$(TARGET_DEVICE)
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
#LOCAL_MODULE := camera.origen_test

include $(BUILD_SHARED_LIBRARY)

#endif

