// Copyright (C) 2012 - 2013 Mihai Preda

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
#include "builtin.h"
#include "Stack.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

Pepper::Pepper(void *context) :
    gc(new GC()),
    vm(new VM(gc, context))
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

Func *Pepper::parse(const char *text, bool isFunc) {
    NameValue android[] = {};
    NameValue java[] = {
        NameValue("class", CFunc::value(gc, javaClass)),
    };

    NameValue builtin[] = {
        NameValue("type",   CFunc::value(gc, builtinType)),
        NameValue("print",  CFunc::value(gc, builtinPrint)),
        NameValue("gc",     CFunc::value(gc, builtinGC)),
        NameValue("import", CFunc::value(gc, builtinImport)),
        NameValue("android", Map::value(gc, ASIZE(android), android)),
        NameValue("java", Map::value(gc, ASIZE(java), java)),
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
