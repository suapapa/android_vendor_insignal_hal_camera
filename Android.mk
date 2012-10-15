LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# CameraFactory for ORIGEN board
# TODO: Fix it can select custom factory
LOCAL_SRC_FILES += \
	CameraFactory.cpp

LOCAL_SRC_FILES += \
	SecCamera.cpp \
	SecV4L2Adapter.cpp \

LOCAL_SRC_FILES += \
        CameraHardware.cpp \
        CameraDeviceModule.cpp

LOCAL_SHARED_LIBRARIES := libutils libui liblog libbinder libdl libcutils
LOCAL_SHARED_LIBRARIES += libhardware libcamera_client

# libyuv
LOCAL_STATIC_LIBRARIES += libyuv_static
LOCAL_C_INCLUDES += external/libyuv/files/include

# Software encoder
LOCAL_CFLAGS += -DLIBJPEG_ENCODER
LOCAL_SHARED_LIBRARIES += libjpeg
LOCAL_C_INCLUDES += external/jpeg
LOCAL_SRC_FILES += LibJpegEncoder.cpp

# Hardware encoder
# ifeq ($(TARGET_SOC),exynos4210)
# LOCAL_CFLAGS += -DSAMSUNG_S5P_JPEG_ENCODER
# LOCAL_SHARED_LIBRARIES += libs5pjpeg
# LOCAL_C_INCLUDES += hardware/samsung/exynos4/hal/include
# LOCAL_SRC_FILES += S5PJpegEncoder.cpp
# endif

# ExifTagger
LOCAL_SHARED_LIBRARIES += libexif
LOCAL_C_INCLUDES += external/jhead
LOCAL_SRC_FILES += ExifTagger.cpp

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE := camera.$(TARGET_DEVICE)
include $(BUILD_SHARED_LIBRARY)

