// Copyright (C) 2012 - 2013 Mihai Preda

#include "builtin.h"
#include "Value.h"
#include "Array.h"
#include "Map.h"
#include "Object.h"
#include "String.h"
#include "VM.h"
#include "CFunc.h"
#include "StringBuilder.h"
#include "Parser.h"

#include <assert.h>
#include <stdio.h>

Value builtinPrint(VM *vm, int op, void *data, Value *stack, int nCallArgs) {
    assert(op == CFunc::CFUNC_CALL && !data);
    assert(nCallArgs > 0);
    StringBuilder buf;    
    for (int i = 1; i < nCallArgs; ++i) {
        buf.append(stack[i]);
        buf.append(' ');
    }
    printf("%s\n", buf.cstr());
    return VNIL;    
}

char *typeStr(Value v); // defined in StringBuilder.cpp

Value builtinType(VM *vm, int op, void *data, Value *stack, int nCallArgs) {
    assert(op == CFunc::CFUNC_CALL && !data);
    assert(nCallArgs > 0);
    if (nCallArgs < 2) { return VNIL; }
    return String::value(vm->getGC(), typeStr(stack[1]));
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

Value builtinParse(VM *vm, int op, void *data, Value *stack, int nCallArg) {
    if (op != CFunc::CFUNC_CALL) { return VNIL; }
    if (nCallArg < 2) { return VNIL; }
    Value v = stack[1];
    if (!IS_STRING(v)) { return VNIL; }
    const char *text = GET_CSTR(v);
    Func *f = Parser::parseInEnv(vm->getGC(), text, true);
    return f ? VAL_OBJ(f) : VNIL;
}

#ifdef __ANDROID__
#include "JavaLink.h"
Value javaClass(VM *vm, int op, void *data, Value *stack, int nCallArg) {
    if (op != CFunc::CFUNC_CALL) { return VNIL; }
    if (nCallArg < 2) { return VNIL; }
    Value v = stack[1];
    if (!IS_STRING(v)) { return VNIL; }
    const char *name = GET_CSTR(v);
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
}
#endif
