#include "common.h"

u64 hash64(byte *p, int len, u64 h) {
    byte *end = p + len;
    while (p < end) {
        h ^= *p++;
        h *= 0x100000001b3ULL;
        // h += (h<<1) + (h<<4) + (h<<5) + (h<<7) + (h<<8) + (h<<40);
        // h = (h << 40) + h * 0x1b3;
    }
    return h;
}

u64 hash64(const char *s) {
    u64 h = 0; // 0xcbf29ce484222325ULL;
    byte *p = (byte *) s;
    while (*p) {
        h ^= *p++;
        h *= 0x100000001b3ULL;
        // h += (h<<1) + (h<<4) + (h<<5) + (h<<7) + (h<<8) + (h<<40);
        // h = (h << 40) + h * 0x1b3;
    }
    return h;
}
