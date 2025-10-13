#
# Copyright (C) YuqiaoZhang(HanetakaChou)
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

# https://developer.android.com/ndk/guides/android_mk

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := qxcb

LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparty/Qt/build-linux/x86_64/plugins/platforms/libqxcb.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := composeplatforminputcontextplugin

LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparty/Qt/build-linux/x86_64/plugins/platforminputcontexts/libcomposeplatforminputcontextplugin.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := qsvgicon

LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparty/Qt/build-linux/x86_64/plugins/iconengines/libqsvgicon.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := qgif

LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparty/Qt/build-linux/x86_64/plugins/imageformats/libqgif.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := qico

LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparty/Qt/build-linux/x86_64/plugins/imageformats/libqico.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := qjpeg

LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparty/Qt/build-linux/x86_64/plugins/imageformats/libqjpeg.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := qsvg

LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparty/Qt/build-linux/x86_64/plugins/imageformats/libqsvg.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := qicns

LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparty/Qt/build-linux/x86_64/plugins/imageformats/libqicns.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := qtga

LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparty/Qt/build-linux/x86_64/plugins/imageformats/libqtga.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := qtiff

LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparty/Qt/build-linux/x86_64/plugins/imageformats/libqtiff.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := qwbmp

LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparty/Qt/build-linux/x86_64/plugins/imageformats/libqwbmp.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := qwebp

LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparty/Qt/build-linux/x86_64/plugins/imageformats/libqwebp.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := Qt5XcbQpa

LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparty/Qt/build-linux/x86_64/lib/libQt5XcbQpa.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := Qt5XkbCommonSupport

LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparty/Qt/build-linux/x86_64/lib/libQt5XkbCommonSupport.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := Qt5ThemeSupport

LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparty/Qt/build-linux/x86_64/lib/libQt5ThemeSupport.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := Qt5ServiceSupport

LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparty/Qt/build-linux/x86_64/lib/libQt5ServiceSupport.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := Qt5FontDatabaseSupport

LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparty/Qt/build-linux/x86_64/lib/libQt5FontDatabaseSupport.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := Qt5EdidSupport

LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparty/Qt/build-linux/x86_64/lib/libQt5EdidSupport.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := Qt5Svg

LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparty/Qt/build-linux/x86_64/lib/libQt5Svg.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := Qt5Widgets 

LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparty/Qt/build-linux/x86_64/lib/libQt5Widgets.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := Qt5Gui 

LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparty/Qt/build-linux/x86_64/lib/libQt5Gui.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := qtfreetype 

LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparty/Qt/build-linux/x86_64/lib/libqtfreetype.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := qtharfbuzz 

LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparty/Qt/build-linux/x86_64/lib/libqtharfbuzz.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := qtlibjpeg 

LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparty/Qt/build-linux/x86_64/lib/libqtlibjpeg.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := qtlibpng 

LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparty/Qt/build-linux/x86_64/lib/libqtlibpng.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := Qt5Core 

LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparty/Qt/build-linux/x86_64/lib/libQt5Core.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := qtpcre2 

LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparty/Qt/build-linux/x86_64/lib/libqtpcre2.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := BRX-OpenFileDialog

LOCAL_SRC_FILES := \
    $(LOCAL_PATH)/../source/linux_main.cpp

LOCAL_CFLAGS :=

ifeq (armeabi-v7a,$(TARGET_ARCH_ABI))
LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON := true
else ifeq (arm64-v8a,$(TARGET_ARCH_ABI))
LOCAL_CFLAGS +=
else ifeq (x86,$(TARGET_ARCH_ABI))
LOCAL_CFLAGS += -mf16c
LOCAL_CFLAGS += -mfma
LOCAL_CFLAGS += -mavx2
else ifeq (x86_64,$(TARGET_ARCH_ABI))
LOCAL_CFLAGS += -mf16c
LOCAL_CFLAGS += -mfma
LOCAL_CFLAGS += -mavx2
else
LOCAL_CFLAGS +=
endif

LOCAL_CFLAGS += -Wall
LOCAL_CFLAGS += -Werror=return-type

LOCAL_CFLAGS += -finput-charset=UTF-8 -fexec-charset=UTF-8

LOCAL_CFLAGS += -DQT_CORE_LIB
LOCAL_CFLAGS += -DQT_GUI_LIB
LOCAL_CFLAGS += -DQT_WIDGETS_LIB

LOCAL_C_INCLUDES :=
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../thirdparty/Qt/include

LOCAL_CPPFLAGS := 
LOCAL_CPPFLAGS += -std=c++20

LOCAL_LDFLAGS :=
LOCAL_LDFLAGS += -Wl,--enable-new-dtags
LOCAL_LDFLAGS += -Wl,-rpath,\$$ORIGIN
LOCAL_LDFLAGS += -Wl,--version-script,$(LOCAL_PATH)/BRX-OpenFileDialog.map

LOCAL_LDFLAGS += -ldl
LOCAL_LDFLAGS += -lxcb
LOCAL_LDFLAGS += -lxcb-icccm
LOCAL_LDFLAGS += -lxcb-image
LOCAL_LDFLAGS += -lxcb-shm
LOCAL_LDFLAGS += -lxcb-keysyms
LOCAL_LDFLAGS += -lxcb-randr
LOCAL_LDFLAGS += -lxcb-render-util
LOCAL_LDFLAGS += -lxcb-render
LOCAL_LDFLAGS += -lxcb-shape
LOCAL_LDFLAGS += -lxcb-sync
LOCAL_LDFLAGS += -lxcb-xfixes
LOCAL_LDFLAGS += -lxcb-xinerama
LOCAL_LDFLAGS += -lxcb-xkb
LOCAL_LDFLAGS += -lxcb-xinput
LOCAL_LDFLAGS += -lxkbcommon-x11
LOCAL_LDFLAGS += -lxkbcommon

LOCAL_STATIC_LIBRARIES :=
LOCAL_STATIC_LIBRARIES += qxcb
LOCAL_STATIC_LIBRARIES += composeplatforminputcontextplugin
LOCAL_STATIC_LIBRARIES += qsvgicon
LOCAL_STATIC_LIBRARIES += qgif
LOCAL_STATIC_LIBRARIES += qico
LOCAL_STATIC_LIBRARIES += qjpeg
LOCAL_STATIC_LIBRARIES += qsvg
LOCAL_STATIC_LIBRARIES += qicns
LOCAL_STATIC_LIBRARIES += qtga
LOCAL_STATIC_LIBRARIES += qtiff
LOCAL_STATIC_LIBRARIES += qwbmp
LOCAL_STATIC_LIBRARIES += qwebp
LOCAL_STATIC_LIBRARIES += Qt5XcbQpa
LOCAL_STATIC_LIBRARIES += Qt5XkbCommonSupport
LOCAL_STATIC_LIBRARIES += Qt5ThemeSupport
LOCAL_STATIC_LIBRARIES += Qt5ServiceSupport
LOCAL_STATIC_LIBRARIES += Qt5FontDatabaseSupport
LOCAL_STATIC_LIBRARIES += Qt5EdidSupport
LOCAL_STATIC_LIBRARIES += Qt5Svg 
LOCAL_STATIC_LIBRARIES += Qt5Widgets 
LOCAL_STATIC_LIBRARIES += Qt5Gui
LOCAL_STATIC_LIBRARIES += qtfreetype
LOCAL_STATIC_LIBRARIES += qtharfbuzz
LOCAL_STATIC_LIBRARIES += qtlibjpeg
LOCAL_STATIC_LIBRARIES += qtlibpng
LOCAL_STATIC_LIBRARIES += Qt5Core 
LOCAL_STATIC_LIBRARIES += qtpcre2

LOCAL_SHARED_LIBRARIES :=
LOCAL_SHARED_LIBRARIES += BRX-ImGui-Font-Asset

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_MODULE := BRX-WSI

LOCAL_SRC_FILES := \
    $(LOCAL_PATH)/../source/brx_wsi_linux.cpp

LOCAL_CFLAGS :=

ifeq (armeabi-v7a,$(TARGET_ARCH_ABI))
LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON := true
else ifeq (arm64-v8a,$(TARGET_ARCH_ABI))
LOCAL_CFLAGS +=
else ifeq (x86,$(TARGET_ARCH_ABI))
LOCAL_CFLAGS += -mf16c
LOCAL_CFLAGS += -mfma
LOCAL_CFLAGS += -mavx2
else ifeq (x86_64,$(TARGET_ARCH_ABI))
LOCAL_CFLAGS += -mf16c
LOCAL_CFLAGS += -mfma
LOCAL_CFLAGS += -mavx2
else
LOCAL_CFLAGS +=
endif

LOCAL_CFLAGS += -Wall
LOCAL_CFLAGS += -Werror=return-type

LOCAL_C_INCLUDES :=

LOCAL_CPPFLAGS := 
LOCAL_CPPFLAGS += -std=c++20

LOCAL_LDFLAGS :=
LOCAL_LDFLAGS += -Wl,--enable-new-dtags
LOCAL_LDFLAGS += -Wl,-rpath,\$$ORIGIN
LOCAL_LDFLAGS += -Wl,--version-script,$(LOCAL_PATH)/BRX-WSI.map

LOCAL_LDFLAGS += -lxcb

LOCAL_STATIC_LIBRARIES :=

LOCAL_SHARED_LIBRARIES :=
LOCAL_SHARED_LIBRARIES += BRX-ImGui
LOCAL_SHARED_LIBRARIES += McRT-Malloc

include $(BUILD_SHARED_LIBRARY)
