// Copyright (C) 2012 Mihai Preda

#include <jni.h>
#include <android/log.h>
#include "common.h"
#include "Pepper.h"

#define printf(fmt, args...) __android_log_print(3, "Pepper", fmt, ##args);
#define JNI extern "C"
#define JNINAME(method) Java_pepper_app_State_##method

class JavaLink {
    jclass cls;
    jmethodID midBackground;

public:
    JavaLink(JNIEnv *env, jobject jobj);
    ~JavaLink();
    void deinit(JNIEnv *env);
    
    jmethodID getMethod(JNIEnv *env, const char *name, const char *sign);

    void draw(JNIEnv *env, jobject jobj);

    void background(JNIEnv *env, jobject jobj, int r, int g, int b) {
        env->CallVoidMethod(jobj, midBackground, r, g, b);
    }

};

void JavaLink::draw(JNIEnv *env, jobject jobj) {
    background(env, jobj, 50, 100, 150);
}

JavaLink::JavaLink(JNIEnv *env, jobject jobj) {
    cls           = (jclass) env->NewGlobalRef(env->GetObjectClass(jobj));
    midBackground = getMethod(env, "background", "(III)V");
}

void JavaLink::deinit(JNIEnv *env) {
    env->DeleteGlobalRef(cls);
}

JavaLink::~JavaLink() {
}

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
    const char *txt = env->GetStringUTFChars(jtxt, 0);
    Pepper pepper;
    Func *f = pepper.parseStatList(txt);
    env->ReleaseStringUTFChars(jtxt, txt);
    Value v = pepper.run(f, 0, 0);
    return IS_NUM(v) ? (int) GET_NUM(v) : -1;
}


//  __attribute__ ((visibility ("default")))
// #define JNI(ret, method, tail...) extern "C" __attribute__ ((visibility ("default"))) ret Java_pepper_app_##method(JNIEnv *env, jobject jobj, tail)
