// Copyright (C) 2013 Mihai Preda

#include "JavaLink.h"

void JavaLink::draw(JNIEnv *env, jobject jobj) {
    background(env, jobj, 50, 100, 150);
}

JavaLink::JavaLink(JNIEnv *env, jobject jobj) :
    env(env)
{
    cls           = (jclass) env->NewGlobalRef(env->GetObjectClass(jobj));
    midBackground = getMethod(env, "background", "(III)V");
}

void JavaLink::deinit(JNIEnv *env) {
    env->DeleteGlobalRef(cls);
}

JavaLink::~JavaLink() {
}
