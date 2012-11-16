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

static const char *opNames[32];

static const char *typeNames[] = {
    "NIL", "[]", "{}", 0,
    0, 0, 0, 0,
    "\"\"",
};

static bool init() {
#define ENTRY(t, n) t[n]=#n
#define _(n) ENTRY(opNames, n)
#define __(a, b, c, d) _(a); _(b); _(c); _(d);
    __(JMP, CALL, RETURN, MOVE);
    _(GET); _(SET),
    __(ADD, SUB, MUL, DIV);
    __(MOD, POW, AND, OR);
    _(XOR); _(NOT); _(LEN);
#undef _
#undef __
#undef ENTRY
    return true;
}


static void printValue(char *buf, int bufSize, Value a) {
    const int t = TAG(a);
    // printf("tag %d\n", t);
    if (t == INTEGER) {
        snprintf(buf, bufSize, "%lld", getInteger(a));
        // printf("foo %s\n", buf);
    } else if (IS_DOUBLE_TAG(t)) {
        snprintf(buf, bufSize, "%g", getDouble(a));
    } else if (t == OBJECT) {
        if (a==NIL) {
            snprintf(buf, bufSize, "NIL");
        } else {
            int type = ((Object *) a)->type;
            snprintf(buf, bufSize, "OBJECT(%s)", objTypeName[type]);
        }
    } else if (t <= STRING) {
        snprintf(buf, bufSize, "%s", emptyVals[t]);
    } else if (t > STRING && t <= STRING+6) {
        int sz = t - STRING;
        char tmp[8] = {0};
        memcpy(tmp, (char *)&a, sz);
        snprintf(buf, bufSize, "\"%s\"", tmp);
    } else {
        snprintf(buf, bufSize, "<?%d>", t);
    }
}

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
    static bool inited = init();
    (void) inited;
    char sa[32], sb[32], sc[32];
    int i = 0;
    for (unsigned *end = p + size; p < end; ++p, ++i) {
        unsigned code = *p;
        int fullOp = OP(code), a = OA(code), b = OB(code), c = OC(code);
        int op = fullOp & 0x1f;    
        printOperand(sa, sizeof(sa), fullOp & 0x20, a);
        printOperand(sb, sizeof(sb), fullOp & 0x40, b);
        printOperand(sc, sizeof(sc), fullOp & 0x80, c);
        printf("%2d: %02x%02x%02x%02x   %-4s ", i, fullOp, a, b, c, opNames[op]);
        switch (op) {
        case JMP: {
            // int offset  = a;
            // int jmpDest = i+offset+1;
            /*
            if (fullOp & 0x20 && b==0) {
                printf("%3d:\n", jmpDest);
            } else {
            */
            printf("%3s   %3s\n", sa, sb);
            // }
            break;
        }

        case MOVE:
            printf("%3s,  %3s\n", sc, sa);
            break;

        default:
            printf("%3s,  %3s %3s\n", sc, sa, sb);
        }
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
            printf("%2d: %d\n", i, u-1);
        } else if (u < 0) {
            printf("%2d: [%d]\n", i, -u-1);
        } else {
            printValue(buf, sizeof(buf), proto->consts.buf[c++]);
            printf("%2d: #%s\n", i, buf);
        }
    }
    printf("\nCode:\n");
    printBytecode(proto->code.buf, proto->code.size);
}
