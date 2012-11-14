#include "Decompile.h"
#include "Value.h"
#include "VM.h"
#include "Proto.h"
#include "Object.h"

#include <stdio.h>
#include <string.h>

static const char *objTypeName[] = {
    0, "ARRAY", "MAP", "FUNC", "CFUNC", 0, 0, 0, "STRING"
};

static const char *emptyVals[] = {0, "[]", "{}", 0, 0, 0, 0, 0, "\"\""};

void printValue(char *buf, int bufSize, Value a) {
    const int t = TAG(a);
    // printf("tag %d\n", t);
    if (t == INTEGER) {
        snprintf(buf, bufSize, "%lld", getInteger(a));
        // printf("foo %s\n", buf);
    } else if (IS_DOUBLE_TAG(t)) {
        snprintf(buf, bufSize, "%g", getDouble(a));
    } else if (t == OBJECT) {
        int type = ((Object *) a)->type;
        snprintf(buf, bufSize, "OBJECT(%s)", objTypeName[type]);
    } else if (t <= STRING) {
        snprintf(buf, bufSize, "%s", emptyVals[t]);
    } else if (t > STRING && t <= STRING+6) {
        int sz = t - STRING;
        char tmp[8] = {0};
        memcpy(tmp, (char *)a, sz);
        snprintf(buf, bufSize, "\"%s\"", tmp);
    } else {
        snprintf(buf, bufSize, "<?%d>", t);
    }
}

#define OP(c) ((byte) (c))
#define OC(c) ((byte) (c >> 8))
#define OA(c) ((byte) (c >> 16))
#define OB(c) ((byte) (c >> 24))
#define OAB(c) ((unsigned short) (c >> 16))

static const char *opNames[] = {
    "CALL", "RET ", "MOVE",
    "ADD ", "SUB ", "MUL ", "DIV ", "MOD ", "POW ", "AND ", "OR  ", "XOR ",
    "NOT ", "LEN ",
};

static const char *typeNames[] = {
    "NIL", "[] ", "{} ", 0,
    0, 0, 0, 0,
    "\"\" ",
};

static void printOperand(char *buf, int bufSize, int bit, int v) {
    if (!bit) {
        snprintf(buf, bufSize, "%d", v);
    } else {
        if (v < 0x80) {
            snprintf(buf, bufSize, "#%-d", (int)(((signed char)(v<<1))>>1));
        } else if (v < 0xf0) {
            snprintf(buf, bufSize, "[%d]", (int)(v & 0x7f));
        } else {
            snprintf(buf, bufSize, "#%s", typeNames[v & 0xf]);
        }
    }
}

void printBytecode(unsigned *p, int size) {
    char sa[32], sb[32], sc[32];
    for (unsigned *end = p + size; p < end; ++p) {
        unsigned code = *p;
        int fullOp = OP(code), c = OC(code), a = OA(code), b = OB(code);
        int op = fullOp & 0x1f;    
        printOperand(sc, sizeof(sc), fullOp & 0x80, c);
        printOperand(sa, sizeof(sa), fullOp & 0x20, a);
        printOperand(sb, sizeof(sb), fullOp & 0x40, b);
        printf("%02x %02x %02x %02x    %s %4s %4s %4s\n", fullOp, c, a, b, opNames[op], sc, sa, sb);
    }
}

void printProto(Proto *proto) {
    printf("UpVals:\n");
    int i = 0;
    int c = 0;
    char buf[64];
    for (short *p = proto->ups.buf, *end = p + proto->ups.size; p < end; ++p, ++i) {
        int u = *p;
        if (u > 0) {
            printf("%2d %d\n", i, u-1);
        } else if (u < 0) {
            printf("%2d [%d]\n", i, -u-1);
        } else {
            printValue(buf, sizeof(buf), proto->consts.buf[c++]);
            printf("%2d #%s\n", i, buf);
        }
    }
    printf("\nCode:\n");
    printBytecode(proto->code.buf, proto->code.size);
}
