// Copyright (C) 2012 Mihai Preda

#include "Pepper.h"
#include "GC.h"
#include "Parser.h"
#include "VM.h"
#include "Array.h"
#include "Map.h"
#include "SymbolTable.h"
#include "CFunc.h"
#include "String.h"
#include "NameValue.h"
#include "Object.h"

#include <assert.h>
#include <stdio.h>

Pepper::Pepper() :
    gc(new GC()),
    vm(new VM(this))
{
    assert(sizeof(Array) == 2 * sizeof(long));
}

Pepper::~Pepper() {
    delete gc;
    gc = 0;
    delete vm;
    vm = 0;
}

Value Pepper::run(Func *f, int nArg, Value *args) {
    return vm->run(f, nArg, args);
}

static Value funcGC(VM *vm, int op, void *data, Value *stack, int nCallArg) {
    assert(op == CFunc::CFUNC_CALL && !data);
    assert(nCallArg > 0);
    vm->gcCollect(stack + nCallArg);
    return VNIL;
}

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

static Value funcPrint(VM *vm, int op, void *data, Value *stack, int nCallArgs) {
    assert(op == CFunc::CFUNC_CALL && !data);
    assert(nCallArgs > 0);
    
    for (int i = 1; i < nCallArgs; ++i) {
        printVal((i>1)?" ":"", stack[i]);
    }
    printf("\n");
    return VNIL;    
}

Func *Pepper::parse(const char *text, bool isFunc) {
    NameValue builtin[] = {
        NameValue("type",  VNIL),
        NameValue("print", CFunc::value(gc, funcPrint)),
        NameValue("gc",    CFunc::value(gc, funcGC)),
        // NameValue("ffi",   CFunc::value(gc, ffiConstruct)),
    };

    Value ups[] = {
        Map::value(gc, ASIZE(builtin), builtin),
        VAL_OBJ(Map::alloc(gc)), VAL_OBJ(Array::alloc(gc)), EMPTY_STRING,
        VAL_NUM(-1), ONE, ZERO,
        VNIL,
    };

    const char *upNames[] = {
        "builtin",
        0, 0, 0,
        0, 0, 0,
        "nil",
    };
    const int nUps = ASIZE(ups);
    assert(nUps == ASIZE(upNames));

    SymbolTable syms(gc);
    for (int i = 0; i < nUps; ++i) {
        if (upNames[i]) {
            syms.set(String::value(gc, upNames[i]), i - nUps);
        }
    }
    syms.pushContext();
    Func *f = isFunc ? 
        Parser::parseFunc(this, &syms, ups + nUps, text) :
        Parser::parseStatList(this, &syms, ups + nUps, text);
    syms.popContext();
    return f;
}

Func *Pepper::parseFunc(const char *text) {
    return parse(text, true);
}

Func *Pepper::parseStatList(const char *text) {
    return parse(text, false);
}
