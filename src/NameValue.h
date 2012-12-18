#pragma once

struct NameValue {
    const char *name;
    Value value;

    NameValue(const char *n, Value v) : name(n), value(v) {}
    NameValue(Value v) : name(0), value(v) {}
};
