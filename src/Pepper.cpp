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

Func *Pepper::parse(const char *text, bool isFunc) {
    Value androidBg = CFunc::value(gc, androidBackground);
    NameValue android[] = {
        NameValue("bg", androidBg),
        NameValue("background", androidBg),
    };


    NameValue builtin[] = {
        NameValue("type",   CFunc::value(gc, builtinType)),
        NameValue("print",  CFunc::value(gc, builtinPrint)),
        NameValue("gc",     CFunc::value(gc, builtinGC)),
        NameValue("import", CFunc::value(gc, builtinImport)),
        NameValue("android", Map::value(gc, ASIZE(android), android)),
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
