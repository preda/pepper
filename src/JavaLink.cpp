// Copyright (C) 2013 Mihai Preda

#include "JavaLink.h"

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
