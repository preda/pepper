// Copyright (C) 2012 - 2013 Mihai Preda

#include "builtin.h"
#include "Value.h"
#include "Array.h"
#include "Map.h"
#include "Object.h"
#include "String.h"
#include "VM.h"
#include "CFunc.h"
#include "JavaLink.h"

#include <assert.h>
#include <stdio.h>

static void printVal(const char *prefix, Value v) {
    if (IS_STRING(v)) {
        printf("%s'%s'", prefix, GET_CSTR(v)); 
    } else if (IS_NUM(v)) {
        double d = GET_NUM(v);
        if (d == (long long) d) {
            printf("%s%lld", prefix, (long long) d);
        } else {
            printf("%s%f", prefix, d);
        }
    } else if (IS_OBJ(v)) {
        Object *p = GET_OBJ(v);
        if (IS_ARRAY(v)) {
            Array *a = (Array *) p;
            int size = a->size();
            printf("%s[", prefix);
            for (int i = 0; i < size; ++i) { 
                printVal(i?", ":"", a->getI(i)); 
            }
            printf("]");
        } else if (IS_MAP(v)) {
            Map *m = (Map *) p;
            int size = m->size();
            printf("%s{", prefix);
            Value *keys = m->keyBuf();
            Value *vals = m->valBuf();
            for (int i = 0; i < size; ++i) {
                printVal(i?", ":"", keys[i]);
                printVal(":", vals[i]);
            }
            printf("}");
        } else {
            printf("%s%p", prefix, p);
        }
    } else if (IS_NIL(v)) {
        printf("%sNIL", prefix);
    } else if (IS_CF(v)) {
        printf("%sCFunc %p", prefix, GET_CF(v));
    }
}

Value builtinPrint(VM *vm, int op, void *data, Value *stack, int nCallArgs) {
    assert(op == CFunc::CFUNC_CALL && !data);
    assert(nCallArgs > 0);
    
    for (int i = 1; i < nCallArgs; ++i) {
        printVal((i>1)?" ":"", stack[i]);
    }
    printf("\n");
    return VNIL;    
}

Value builtinType(VM *vm, int op, void *data, Value *stack, int nCallArgs) {
    assert(op == CFunc::CFUNC_CALL && !data);
    assert(nCallArgs > 0);
    if (nCallArgs < 2) { return VNIL; }
    Value v = stack[1];
    const char *s = 0;
    if (IS_NIL(v)) {
        s = "nil";
    } else if (IS_NUM(v)) {
        s = "number";
    } else if (IS_STRING(v)) {
        s = "string";
    } else if (IS_ARRAY(v)) {
        s = "array";
    } else if (IS_MAP(v)) {
        s = "map";
    } else if (IS_FUNC(v) || IS_CFUNC(v)) {
        s = "func";
    }
    // else if (IS_PROTO(v)) { s = "proto";
    assert(s); // unexpected type
    return String::value(vm->getGC(), s);
}

Value builtinGC(VM *vm, int op, void *data, Value *stack, int nCallArg) {
    assert(op == CFunc::CFUNC_CALL && !data);
    assert(nCallArg > 0);
    // vm->gcCollect(stack + nCallArg);
    return VNIL;
}

Value builtinImport(VM *vm, int op, void *data, Value *stack, int nCallArg) {
    assert(op == CFunc::CFUNC_CALL && !data);
    assert(nCallArg > 0);
    if (nCallArg >= 2) {
        Value v = stack[1];
        if (IS_STRING(v)) {
            // TODO
        }
    }
    return VNIL;
}

#ifdef __ANDROID__
#include "JavaLink.h"
Value javaClass(VM *vm, int op, void *data, Value *stack, int nCallArg) {
    if (op != CFunc::CFUNC_CALL) { return VNIL; }
    if (nCallArg < 2) { return VNIL; }
    Value v = stack[1];
    if (!IS_STRING(v)) { return VNIL; }
    const char *name = GET_CSTR(v);
<<<<<<< HEAD
    if (!name) { return VNIL; }
    JavaLink *java = (JavaLink *)(vm->getContext());
    if (!java) { return VNIL; }
    JNIEnv *env = java->env;
    if (!env) { return VNIL; }
    // return VNIL;
    jclass cls = env->FindClass("java/lang/String");
    return Map::makeMap(vm->getGC(), "_class", VAL_CP(cls), 0); 
    // return String::value(vm->getGC(), name);
}
#else
Value javaClass(VM *vm, int op, void *data, Value *stack, int nCallArg) {
    return VNIL;
=======
    JavaLink *java = (JavaLink *)(vm->getContext());
    


    return String::value(vm->getGC(), name);
>>>>>>> c89de160155495315eed277e3d38c86286b2afab
}
#endif
