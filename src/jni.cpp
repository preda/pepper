// Copyright (C) 2012 Mihai Preda

#include <jni.h>
#include <android/log.h>
#include "JavaLink.h"
#include "common.h"
#include "Pepper.h"

#define printf(fmt, args...) __android_log_print(3, "Pepper", fmt, ##args);
#define JNI extern "C"
#define JNINAME(method) Java_pepper_app_State_##method

jmethodID JavaLink::getMethod(JNIEnv *env, const char *name, const char *sign) {
    jmethodID ret = env->GetMethodID(cls, name, sign);
    if (!ret) { printf("getMethod '%s' '%s'", name, sign); }
    return ret;
}

JNI long JNINAME(init)(JNIEnv *env, jobject jobj) {
    JavaLink *link = new JavaLink(env, jobj);
    return (long) link;
}

JNI void JNINAME(deinit)(JNIEnv *env, jobject jobj, long jlink) {
    JavaLink *link = (JavaLink *) jlink;
    link->deinit(env);
    delete link;    
}

JNI void JNINAME(draw)(JNIEnv *env, jobject jobj, long jlink) {
    printf("enter State.draw");
    JavaLink *link = (JavaLink *) jlink;
    link->draw(env, jobj);
}

JNI int JNINAME(run)(JNIEnv *env, jobject jobj, jstring jtxt) {
    JavaLink context(env, jobj);
    // env->FindClass("java/lang/String");
    const char *txt = env->GetStringUTFChars(jtxt, 0);
    Pepper pepper(&context);
    Func *f = pepper.parseStatList(txt);
    env->ReleaseStringUTFChars(jtxt, txt);
    Value v = pepper.run(f, 0, 0);
    return IS_NUM(v) ? (int) GET_NUM(v) : -1;
}

//  __attribute__ ((visibility ("default")))
// #define JNI(ret, method, tail...) extern "C" __attribute__ ((visibility ("default"))) ret Java_pepper_app_##method(JNIEnv *env, jobject jobj, tail)
