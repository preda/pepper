// Copyright (C) 2012 Mihai Preda

#include "Pepper.h"
#include "GC.h"
#include "Parser.h"
#include "VM.h"
#include "Array.h"
#include "Map.h"
#include "SymbolTable.h"
#include "CFunc.h"
#include "FFI.h"
#include "String.h"
#include "NameValue.h"

#include <assert.h>

Pepper::Pepper() :
    gc(new GC()),
    vm(new VM(this))
{
    assert(sizeof(Array) == 2 * sizeof(long));
    /*
    NameValue builtins[] = {
        NameValue("type",   VNIL),
        NameValue("print",  VNIL),
        NameValue("string", Map::value(gc, ASIZE(stringMethods), stringMethods)),
        NameValue("ffi",    CFunc::value(gc, ffiConstruct)),

        NameValue(EMPTY_MAP),
        NameValue(EMPTY_ARRAY),
        NameValue(EMPTY_STRING),
        NameValue(VAL_NUM(-1)),
        NameValue(ONE),
        NameValue(ZERO),
        NameValue("nil", VNIL),
    };

    const int nUps = ASIZE(builtins);
    ups = new Value[nUps];

    for (int i = 0; i < nUps; ++i) {
        NameValue *nv = &builtins[i];
        if (nv->name) {
            syms->set(String::value(gc, nv->name), i - nUps);
        }
        ups[i] = nv->value;
    }
    upsTop = ups + nUps;
    */
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

Func *Pepper::parse(const char *text, bool isFunc) {
    NameValue builtin[] = {
        NameValue("type",  VNIL),
        NameValue("print", VNIL),
        NameValue("gc", VNIL),
        NameValue("ffi",   CFunc::value(gc, ffiConstruct)),
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
