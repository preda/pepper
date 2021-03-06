LOCAL_PATH := $(call my-dir)


#include $(CLEAR_VARS)
#LOCAL_MODULE    := ffi
#LOCAL_SRC_FILES := ../android/lib/libffi.a
#include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := pepper
LOCAL_SRC_FILES := jni.cpp Array.cpp CFunc.cpp Decompile.cpp fnv.cpp Func.cpp GC.cpp Lexer.cpp Map.cpp Object.cpp Parser.cpp Pepper.cpp Proto.cpp Stack.cpp String.cpp SymbolTable.cpp Value.cpp Vector.cpp VM.cpp error.cpp alloc.cpp main.cpp SimpleMap.cpp Index.cpp builtin.cpp JavaLink.cpp
LOCAL_LDLIBS := -llog
LOCAL_CFLAGS := -std=c++11 -Wall
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := pep
LOCAL_ARM_MODE  := arm
LOCAL_CFLAGS    := -std=c++11 -Wall
# -O2 -Wall -std=c++11
LOCAL_LDLIBS    := -ldl
LOCAL_C_INCLUDES := $(LOCAL_PATH)/android-include
#LOCAL_STATIC_LIBRARIES := ffi
LOCAL_SRC_FILES := Array.cpp CFunc.cpp Decompile.cpp fnv.cpp Func.cpp GC.cpp Lexer.cpp Map.cpp Object.cpp Parser.cpp Pepper.cpp Proto.cpp Stack.cpp String.cpp SymbolTable.cpp Value.cpp Vector.cpp VM.cpp error.cpp alloc.cpp main.cpp SimpleMap.cpp Index.cpp builtin.cpp JavaLink.cpp
include $(BUILD_EXECUTABLE)
