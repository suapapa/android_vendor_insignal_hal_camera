# When zero we link against libqcamera; when 1, we dlopen libqcamera.
ifeq ($(BOARD_CAMERA_LIBRARIES),libcamera)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	SecCameraInfo.cpp \
	SecV4L2Adapter.cpp \
	SecCamera.cpp \
	SecCameraHardware.cpp

# for videodev2_samsung.h and jpeg_api.h
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../include
LOCAL_SHARED_LIBRARIES := libutils libui liblog libbinder libdl libcutils
LOCAL_SHARED_LIBRARIES += libcamera_client
LOCAL_CFLAGS :=-fno-short-enums

ifeq ($(BOARD_USE_JPEG),true)
LOCAL_SRC_FILES += \
	ExynosHWJpeg.cpp

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libs5pjpeg
LOCAL_SHARED_LIBRARIES += libs5pjpeg
endif

ifeq ($(BOARD_USES_CAMERA_OVERLAY),true)
LOCAL_CFLAGS += -DBOARD_USES_CAMERA_OVERLAY
endif

LOCAL_MODULE_TAGS := eng

LOCAL_MODULE := libcamera

include $(BUILD_SHARED_LIBRARY)

endif

