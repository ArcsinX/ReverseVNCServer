LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LIBVNCSERVER_ROOT:=LibVNCServer-0.9.9
LIBJPEG_ROOT:=../jpeg-6b
LIBJPEG_ROOT:=../jpeg-turbo
LIBPNG_ROOT:=../libpng-1.5.18
OPENSSL_ROOT:=../openssl

HAVE_LIBZ=1
HAVE_LIBJPEG=1
HAVE_LIBPNG=1
HAVE_OPENSSL=1

ifdef HAVE_LIBZ
ZLIBSRCS := \
	$(LIBVNCSERVER_ROOT)/libvncserver/zlib.c \
	$(LIBVNCSERVER_ROOT)/libvncserver/zrle.c \
	$(LIBVNCSERVER_ROOT)/libvncserver/zrleoutstream.c \
	$(LIBVNCSERVER_ROOT)/libvncserver/zrlepalettehelper.c \
	$(LIBVNCSERVER_ROOT)/common/zywrletemplate.c
ifdef HAVE_LIBJPEG
ifdef HAVE_LIBPNG
TIGHTSRCS := $(LIBVNCSERVER_ROOT)/libvncserver/tight.c
endif
endif
endif

LOCAL_SRC_FILES:= \
	reversevncserver.c \
    framebuffer.c \
	$(LIBVNCSERVER_ROOT)/libvncserver/main.c \
	$(LIBVNCSERVER_ROOT)/libvncserver/rfbserver.c \
	$(LIBVNCSERVER_ROOT)/libvncserver/rfbregion.c \
	$(LIBVNCSERVER_ROOT)/libvncserver/auth.c \
	$(LIBVNCSERVER_ROOT)/libvncserver/sockets.c \
	$(LIBVNCSERVER_ROOT)/libvncserver/stats.c \
	$(LIBVNCSERVER_ROOT)/libvncserver/corre.c \
	$(LIBVNCSERVER_ROOT)/libvncserver/hextile.c \
	$(LIBVNCSERVER_ROOT)/libvncserver/rre.c \
	$(LIBVNCSERVER_ROOT)/libvncserver/translate.c \
	$(LIBVNCSERVER_ROOT)/libvncserver/cutpaste.c \
	$(LIBVNCSERVER_ROOT)/libvncserver/httpd.c \
	$(LIBVNCSERVER_ROOT)/libvncserver/cursor.c \
	$(LIBVNCSERVER_ROOT)/libvncserver/font.c \
	$(LIBVNCSERVER_ROOT)/libvncserver/draw.c \
	$(LIBVNCSERVER_ROOT)/libvncserver/selbox.c \
	$(LIBVNCSERVER_ROOT)/common/d3des.c \
	$(LIBVNCSERVER_ROOT)/common/vncauth.c \
	$(LIBVNCSERVER_ROOT)/libvncserver/cargs.c \
	$(LIBVNCSERVER_ROOT)/common/minilzo.c \
	$(LIBVNCSERVER_ROOT)/libvncserver/ultra.c \
	$(LIBVNCSERVER_ROOT)/libvncserver/scale.c \
	$(ZLIBSRCS) \
	$(TIGHTSRCS)

ifdef HAVE_OPENSSL
LOCAL_SRC_FILES += 	\
    $(LIBVNCSERVER_ROOT)/libvncserver/websockets.c \
	$(LIBVNCSERVER_ROOT)/libvncserver/rfbssl_openssl.c \
 	$(LIBVNCSERVER_ROOT)/libvncserver/rfbcrypto_openssl.c
endif

ifdef HAVE_LIBJPEG
LOCAL_SRC_FILES += \
	$(LIBVNCSERVER_ROOT)/common/turbojpeg.c
endif

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/$(LIBVNCSERVER_ROOT)/libvncserver \
	$(LOCAL_PATH)/$(LIBVNCSERVER_ROOT)/common \
	$(LOCAL_PATH)/$(LIBVNCSERVER_ROOT)/rfb \
	$(LOCAL_PATH)/$(LIBVNCSERVER_ROOT)

ifdef HAVE_LIBJPEG
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(LIBJPEG_ROOT)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(LIBJPEGTURBO_ROOT)
endif
ifdef HAVE_LIBPNG
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(LIBPNG_ROOT)
endif
ifdef HAVE_LIBPNG
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(OPENSSL_ROOT)/include
endif

ifdef HAVE_LIBZ
LOCAL_SHARED_LIBRARIES := libz
LOCAL_LDLIBS := -lz
endif

LOCAL_STATIC_LIBRARIES := 
ifdef HAVE_LIBJPEG
LOCAL_STATIC_LIBRARIES += libjpeg
endif
ifdef HAVE_LIBPNG
LOCAL_STATIC_LIBRARIES += libpng libssl_static libcrypto_static
endif
ifdef HAVE_OPENSSL
LOCAL_STATIC_LIBRARIES += libssl_static libcrypto_static
endif

LOCAL_MODULE:= reversevncserver

include $(BUILD_EXECUTABLE)
