#include "fnv.h"
#include <stdio.h>
#include <stdlib.h>

u64 hash(const char *s) {
    u64 h = hash64(s);
    return h;
    // return h >> 32;
    // return ((unsigned)(h>>32)) ^ ((unsigned) h);
}

int main() {
    char tab[64];
    int i = 0;
    for (char c = 'a'; c <= 'z'; ++c) {
        tab[i++] = c;
    }
    for (char c = 'A'; c <= 'Z'; ++c) {
        tab[i++] = c;
    }
    for (char c = '0'; c <= '9'; ++c) {
        tab[i++] = c;
    }
    tab[i++] = '_';
    int n = i;
    char str[64] = {0};
    int pos[64];
    for (int i = 0; i < 64; ++i) {
        pos[i] = -1;
    }

    unsigned char *bits = (unsigned char *) calloc(1024 * 1024 * 1024, 4);
    int nc = 0;
    unsigned t = 0;
    while (true) {
        int p = 0;
        while(pos[p] == n-1) { 
            pos[p] = 0;
            str[p] = 'a';
            ++p;
        }
        ++pos[p];
        str[p] = tab[pos[p]];
        u64 hh = hash(str);
        unsigned char mask = 1 << (((unsigned)(hh>>32)) & 7);
        unsigned char *ptr = bits + ((unsigned)hh);
        ++t;
        if (*ptr & mask) {
            printf("'%s' %llx %u\n", str, hh, t);
            if (++nc > 10) { return 0; }
            /*
            unsigned ch = h;
            for (int i = 0; i < 64; ++i) {
                pos[i] = -1;
                str[i] = 0;
            }
            while (true) {
                int p = 0;
                while (pos[p] == n-1) {
                    pos[p] = 0;
                    str[p] = 'a';
                    ++p;
                }
                ++pos[p];
                str[p] = tab[pos[p]];
                unsigned h2 = hash(str);
                if (h2 == ch) {
                    printf("'%s' %x\n", str, h2);
                }
            }
            return 0;
            */
        } else {
            *ptr |= mask;
        }
        // printf("%s\n", str);
    }
}

/*
unsigned hash(const char *s) {
    unsigned h = 5381;
    byte *p = (byte *) s;
    while (*p) {
        h = h * 33 + *p++;
    }
    return h;
}

unsigned hash(const char *s) {
    unsigned h = 0; // 0xcbf29ce484222325ULL;
    byte *p = (byte *) s;
    while (*p) {
        h ^= *p++;
        h *= 0x01000193;
    }
    return h;
}
*/

