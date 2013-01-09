// Copyright (C) 2012 - 2013 Mihai Preda

#include "SimpleMap.h"

Value SimpleMap::remove(Value key) {
    const int pos = index.remove(key);
    if (pos < 0) { 
        return VNIL; 
    } else {
        Value val = vals.get(pos);
        Value top = vals.pop();
        if (pos < index.size()) {
            vals.setDirect(pos, top);
        }
        return val;
    }
}

Value SimpleMap::get(Value key) {
    const int pos = index.getPos(key);
    return pos < 0 ? VNIL : vals.get(pos);
}

Value SimpleMap::set(Value key, Value v) {
    const int pos = index.add(key);
    Value prev = VNIL;
    if (pos >= 0) {
        prev = vals.get(pos);
        vals.setDirect(pos, v);
    } else {
        vals.push(v);
    }
    return prev;
}

Value SimpleMap::getOrAdd(Value key, Value v) {
    const int pos = index.add(key);
    return pos >= 0 ? vals.get(pos) : (vals.push(v), v);
}
