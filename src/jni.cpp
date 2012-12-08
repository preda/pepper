// Copyright (C) 2012 Mihai Preda

#include <jni.h>
#include <android/log.h>

#define printf(fmt, args...) __android_log_print(3, "Pepper", fmt, ##args);

#define JNI extern "C"
//  __attribute__ ((visibility ("default")))

// #define JNI(ret, method, tail...) extern "C" __attribute__ ((visibility ("default"))) ret Java_pepper_app_##method(JNIEnv *env, jobject jobj, tail)

JNI void Java_pepper_app_State_draw(JNIEnv *env, jobject jobj) {
    printf("enter State.draw");
}

