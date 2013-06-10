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

Value builtinFileRead(VM *vm, int op, void *data, Value *stack, int nCallArgs) {
    assert(op == CFunc::CFUNC_CALL && !data);
    if (nCallArgs >= 2) {
        Value v = stack[1];
        if (IS_STRING(v)) {
            const char *name = GET_CSTR(v);
            FILE *fi = fopen(name, "rb");
            if (fi) {
                Vector<char> chars;
                char buf[4 * 1024];
                while (true) {
                    int n = fread(buf, 1, sizeof(buf), fi);
                    chars.append(buf, n);
                    if (n < (int)sizeof(buf)) { break; }
                }
                fclose(fi);
                return String::value(vm->gc, chars.buf(), chars.size());
            }
        }    
    }
    return VNIL;
}

Value builtinPrint(VM *vm, int op, void *data, Value *stack, int nCallArgs) {
    assert(op == CFunc::CFUNC_CALL && !data);
    assert(nCallArgs > 0);
    StringBuilder buf;    
    for (int i = 1; i < nCallArgs; ++i) {
        buf.append(stack[i], true);
        buf.append(' ');
    }
    printf("%s\n", buf.cstr());
    return VNIL;    
}

Value builtinType(VM *vm, int op, void *data, Value *stack, int nCallArgs) {
    assert(op == CFunc::CFUNC_CALL && !data);
    assert(nCallArgs > 0);
    if (nCallArgs < 2) { return VNIL; }
    return String::value(vm->gc, typeStr(stack[1]));
}

static Value builtinParse(VM *vm, int op, void *data, Value *stack, int nCallArg, bool isFunc) {
    if (op != CFunc::CFUNC_CALL) { return VNIL; }
    if (nCallArg < 2) { return VNIL; }
    Value v = stack[1];
    if (!IS_STRING(v)) { return VNIL; }
    const char *text = GET_CSTR(v);
    Func *f = Parser::parseInEnv(vm->pepper(), text, isFunc);
    return f ? VAL_OBJ(f) : VNIL;
}

Value builtinParseFunc(VM *vm, int op, void *data, Value *stack, int nCallArg) {
    return builtinParse(vm, op, data, stack, nCallArg, true);
}

Value builtinParseBlock(VM *vm, int op, void *data, Value *stack, int nCallArg) {
    return builtinParse(vm, op, data, stack, nCallArg, false);
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
    jclass cls = env->FindClass("java/lang/String");
    return Map::makeMap(vm->gc(), "_class", VAL_CP(cls), 0); 
}
#else
Value javaClass(VM *vm, int op, void *data, Value *stack, int nCallArg) {
    return VNIL;
}
#endif
