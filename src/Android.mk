LOCAL_PATH := $(call my-dir)


include $(CLEAR_VARS)
LOCAL_MODULE    := ffi
LOCAL_SRC_FILES := ../lib/libffi.a
include $(PREBUILT_STATIC_LIBRARY)



include $(CLEAR_VARS)
LOCAL_ARM_MODE  := arm
LOCAL_MODULE    := run
LOCAL_CFLAGS    := -O2 -Wall -std=c++11
LOCAL_LDLIBS    := -ldl
# -lffi
LOCAL_C_INCLUDES := jni/android-include
LOCAL_STATIC_LIBRARIES := ffi
LOCAL_SRC_FILES := Array.cpp CFunc.cpp Decompile.cpp FFI.cpp fnv.cpp Func.cpp GC.cpp Lexer.cpp Map.cpp Object.cpp Parser.cpp Proto.cpp String.cpp SymbolMap.cpp SymbolTable.cpp Value.cpp Vector.cpp VM.cpp error.cpp alloc.cpp run.cpp

include $(BUILD_EXECUTABLE)


#LOCAL_LDLIBS := -llog -ljnigraphics -lz -lGLESv1_CM -landroid
#include $(BUILD_SHARED_LIBRARY)
