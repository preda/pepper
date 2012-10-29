#include "String.h"
#include "Object.h"
#include "GC.h"
#include <string.h>

String *String::alloc(int size) {
    String *s = (String *) GC::alloc(STRING, sizeof(String) - 4 + size, false);
    s->size = size;
    return s;
}

String *String::alloc(const char *p, int size) {
    String *s = alloc(size);
    memcpy(s->s, p, size);
    return s;
}

unsigned String::hashCode(char *buf, int size) {
    unsigned char *p = (unsigned char *) buf;
    unsigned h = 0;
    unsigned char *end = p + size;
    while (p < end) {
        h = (h ^ *p++) * FNV;
    }
    return h;
}

unsigned String::hashCode() {
    return hashCode(s, size);
}

bool String::equals(String *other) {
    if (size != other->size || (cachedHash && other->cachedHash && cachedHash != other->cachedHash)) {
        return false;
    }
    return !memcmp(s, other->s, size);
}
