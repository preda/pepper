// Copyright (C) 2013 Mihai Preda

#include <jni.h>
#include "common.h"

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
