// Copyright (C) 2013 Mihai Preda

#pragma once

#include "value.h"

class GC;
class Type;
class VM;

class Types {
    Type
        *stringType,
        *arrayType,
        *mapType,
        *functionType,
        *defaultType;
        
 public:
    Types(VM *vm);
    ~Types();

    Type *type(Value v);
};

class Type {
    Value auxFieldGet(Value self, Value key);
    Types *types;
    
 public:
    Type(Types *types);
    virtual ~Type();

    Type *type(Value v);
    bool isProperty(Value v);
    
    virtual bool hasIndexGet();
    virtual bool hasIndexSet();
    virtual bool hasFieldGet();
    virtual bool hasFieldSet();
    virtual bool hasCall();
    virtual bool hasRemove();

    virtual Value indexGet(Value self, Value key);
    virtual bool  indexSet(Value self, Value key, Value v);
    virtual bool  remove(Value self, Value key);

    virtual Value fieldGet(Value self, Value key);
    virtual bool  fieldSet(Value self, Value key, Value v);
    
    virtual Value call(Value self, int nArgs, Value *args);
};

class FunctionType : public Type {
    VM *vm;
    
 public:
    FunctionType(Types *types, VM *vm);

    bool hasCall() { return true; }
    Value call(Value self, int nArgs, Value *args);
};

class StringType : public Type {
    Value stringSuper;
    
 public:
    StringType(Types *types, GC *gc);
    
    Value indexGet(Value self, Value key);
    
    bool hasIndexGet() { return true; }
};

class ArrayType : public Type {
    Value arraySuper;
    
 public:
    ArrayType(Types *types, GC *gc);
    
    Value indexGet(Value self, Value key);
    bool indexSet(Value self, Value key, Value v);

    bool hasIndexSet() { return true; }
    bool hasFieldSet() { return false; }
};

class MapType : public Type {
 public:
    MapType(Types *types, GC *gc);
    
    Value indexGet(Value self, Value key);
    bool indexSet(Value self, Value key, Value v);

    bool hasIndexSet() { return true; }    
};
