# Copyright (C) 2013 Amlogic
#
#

LOCAL_PATH := $(call my-dir)

# HAL module implemenation, not prelinked and stored in
# /system/lib/hw/screen_source.amlogic.so
include $(CLEAR_VARS)
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_SRC_FILES := aml_screen.cpp v4l2_vdin.cpp

LOCAL_SHARED_LIBRARIES:= libutils

LOCAL_MODULE := screen_source.amlogic
LOCAL_CFLAGS:= -DLOG_TAG=\"screen_source\"
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
