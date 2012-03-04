LOCAL_PATH:= $(call my-dir)
ORG_PATH := $(LOCAL_PATH)

include $(CLEAR_VARS)
include $(call all-makefiles-under,$(LOCAL_PATH))

include $(CLEAR_VARS)
LOCAL_PATH := $(ORG_PATH)

LOCAL_MODULE    := gljni
LOCAL_CFLAGS    := -Werror
LOCAL_SRC_FILES := main.cpp test.cpp point.cpp image.cpp context.cpp canvas.cpp timer.cpp network.cpp http.cpp js.cpp ifaddrs-android.cpp

LOCAL_C_INCLUDES:= $(NDKROOT)/sources/cxx-stl/stlport/stlport $(LOCAL_PATH)/ $(LOCAL_PATH)/include $(LOCAL_PATH)/include/v8 $(LOCAL_PATH)/include/png

LOCAL_LDLIBS    := $(NDKROOT)/sources/cxx-stl/stlport/libs/armeabi/libstlport_static.a -L$(LOCAL_PATH)/lib/$(TARGET_ARCH_ABI) -lGLESv1_CM -ldl -lz -llog -lv8 -lpng -lcurl

LOCAL_STATIC_LIBRARIES := libzip

APP_STL := stlport_static

include $(BUILD_SHARED_LIBRARY)
