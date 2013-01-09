// Copyright (C) 2012 - 2013 Mihai Preda

#pragma once

#include "Value.h"
#include "Vector.h"
#include "Index.h"

class SimpleMap {
    Vector<Value> vals;
    Index index;
    
 public:
    Value set(Value key, Value v);      // returns previous value or NIL
    Value get(Value key);               // returns NIL if not found
    Value getOrAdd(Value key, Value v); // sets if not existing and returns get(key)
    Value remove(Value key);            // returns previous value or NIL
};
