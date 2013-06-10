#include "Type.h"
#include "Value.h"
#include "String.h"
#include "Array.h"
#include "Map.h"
#include "GC.h"
#include "CFunc.h"
#include "Object.h"
#include "VM.h"

#include <unistd.h>

Types::Types(VM *vm) :
    stringType(new StringType(this, vm->gc)),
    arrayType(new ArrayType(this, vm->gc)),
    mapType(new MapType(this, vm->gc)),
    functionType(new FunctionType(this, vm)),
    defaultType(new Type(this))
{
}

Types::~Types() {
    delete stringType;
    delete arrayType;
    delete mapType;
    delete functionType;
    delete defaultType;
}

Type *Types::type(Value v) {
    assert(!IS_NIL(v));
    if (IS_STRING(v)) {
        return stringType;
    } else if (IS_ARRAY(v)) {
        return arrayType;
    } else if (IS_MAP(v)) {
        return mapType;
    } else if (IS_FUNC(v) || IS_CFUNC(v) || IS_CF(v)) {
        return functionType;
    } else {
        return defaultType;
    }
}

// -- String --

StringType::StringType(Types *types, GC *gc) :
    Type(types),
    stringSuper(Map::makeMap(gc, "find", CFunc::value(gc, String::findField), NULL))
{
    gc->addRoot(GET_OBJ(stringSuper));
}

Value StringType::indexGet(Value self, Value key) {
    return key == STR("super") ? stringSuper : String::indexGet(self, key);
}

// -- Arrray --

ArrayType::ArrayType(Types *types, GC *gc) :
    Type(types),
    arraySuper(Map::makeMap(gc, "size", CFunc::value(gc, Array::sizeField), NULL))
{
    gc->addRoot(GET_OBJ(arraySuper));
}

Value ArrayType::indexGet(Value self, Value key) {
    return key == STR("super") ? arraySuper : ARRAY(self)->indexGet(key);
}

bool ArrayType::indexSet(Value self, Value key, Value v) {
    return ARRAY(self)->indexSet(key, v);
}

// -- Map --

MapType::MapType(Types *types, GC *gc) :
    Type(types)    
{
}

Value MapType::indexGet(Value self, Value key) {
    return MAP(self)->indexGet(key);
}

bool MapType::indexSet(Value self, Value key, Value v) {
    return MAP(self)->indexSet(key, v);
}

// -- FunctionType --

FunctionType::FunctionType(Types *types, VM *vm) :
    Type(types),
    vm(vm)
{
}

Value FunctionType::call(Value self, int nArgs, Value *args) {
    return vm->run(self, nArgs, args);
}

// -- Type --

Type::Type(Types *types) :
    types(types)
{
}

Type *Type::type(Value v) {
    return types->type(v);
}

Type::~Type() {
}

bool Type::hasIndexSet() { return false; }
bool Type::hasRemove()   { return hasIndexSet(); }
bool Type::hasIndexGet() { return hasIndexSet(); }
bool Type::hasFieldSet() { return hasIndexSet(); }
bool Type::hasFieldGet() { return hasIndexGet(); }
bool Type::hasCall()     { return false; }

Value Type::call(Value self, int nArgs, Value *args) {
    return VERR;
}

Value Type::indexGet(Value self, Value key) {
    // if (key == STATIC_STRING("super")) { return super; }
    return VERR;
}

bool Type::indexSet(Value self, Value key, Value v) {
    return IS_NIL(v) ? remove(self, key) : false;
}

bool Type::remove(Value self, Value key) {
    return false;
}

bool Type::isProperty(Value v) {
    return !IS_NIL(v) && types->type(v)->indexGet(v, VAL_NUM(0)) == STR("_prop");
}

// get without invoking the eventual property.get
Value Type::auxFieldGet(Value self, Value key) {
    Value v = indexGet(self, key);
    if (IS_NIL(v) && (v=indexGet(self, STR("super")), !IS_NIL(v))) {
        v = type(v)->fieldGet(v, key);
    }
    return v;
}

Value Type::fieldGet(Value self, Value key) {    
    Value v = auxFieldGet(self, key);
    if (isProperty(v)) {
        Value get = type(v)->indexGet(v, VAL_NUM(1));
        Type *p = type(get);
        if (p->hasCall()) {
            Value args[] = {self, key, v};
            v = p->call(get, 3, args);
        }
    }
    return v;
}

bool Type::fieldSet(Value self, Value key, Value v) {
    Value old = auxFieldGet(self, key);
    if (isProperty(old)) {
        Value set = type(old)->indexGet(old, VAL_NUM(2));
        Type *p = type(set);
        if (p->hasCall()) {
            Value args[] = {self, key, v, old};
            v = p->call(set, 4, args);
            return true;
        }
        return false;
    } else {
        return indexSet(self, key, v);
    }
}
