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

#define ASIZE(x) (sizeof(x)/sizeof(x[0]))

Pepper::Pepper() :
    gc(new GC()),
    syms(new SymbolTable(gc)),
    vm(new VM(this)),

    EMPTY_ARRAY(VAL_OBJ(Array::alloc(gc))),
    EMPTY_MAP(VAL_OBJ(Map::alloc(gc)))
{
    NameValue stringMethods[] = {
        NameValue("find", CFunc::value(gc, String::method_find)),
    };

    builtinString = Map::value(gc, ASIZE(stringMethods), stringMethods);
    
    NameValue builtins[] = {
        NameValue("type",   VNIL),
        NameValue("print",  VNIL),
        NameValue("string", builtinString),
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
}

Pepper::~Pepper() {
    delete gc;
    gc = 0;
    delete syms;
    syms = 0;
    delete ups;
    ups = 0;
    upsTop = 0;
    delete vm;
    vm = 0;
}

Value Pepper::run(Func *f, int nArg, Value *args) {
    return vm->run(f, nArg, args);
}

Func *Pepper::parseFunc(const char *text) {
    syms->pushContext();
    Func *f = Parser::parseFunc(this, syms, upsTop, text);
    syms->popContext();
    return f;
}

Func *Pepper::parseStatList(const char *text) {
    syms->pushContext();
    Func *f = Parser::parseStatList(this, syms, upsTop, text);
    syms->popContext();
    return f;
}
