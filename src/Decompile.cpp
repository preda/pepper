#include "Decompile.h"
#include "Value.h"
#include "VM.h"
#include "Proto.h"
#include "Object.h"
#include "String.h"

#include <stdio.h>
#include <string.h>


static const char *objTypeName[] = {
    0, "ARRAY", "MAP", "FUNC", "CFUNC", "PROTO", "O_STR",
};

// static const char *emptyVals[] = {0, "[]", "{}", 0, 0, 0, 0, 0, "\"\""};

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
    __(JMP, CALL, RET, FUNC);
    _(MOVE);
    _(GET); _(SET),
    __(ADD, SUB, MUL, DIV);
    __(MOD, POW, AND, OR);
    __(AND, OR, SHL, SHR);
    _(XOR);
    _(LNOT); _(BNOT); _(LEN);
    
#undef _
#undef __
#undef ENTRY
    // opNames[CLOSURE] = "FUNC";
    return true;
}

static void printValue(char *buf, int bufSize, Value a) {
    if (IS_INT(a)) {
        snprintf(buf, bufSize, "%lld", getInteger(a));
    } else if (IS_DOUBLE(a)) {
        snprintf(buf, bufSize, "%g", getDouble(a));
    } else if (a==NIL) {
        snprintf(buf, bufSize, "NIL");
    } else if (IS_STRING(a)) {
        snprintf(buf, bufSize, "\"%s\"", GET_CSTR(a));
    } else if (TAG(a) == T_OBJ) {
        int type = O_TYPE(a);
        snprintf(buf, bufSize, "%5s %p", objTypeName[type], (Object *)a);
    } else if (IS_REG(a)) {
        snprintf(buf, bufSize, "register %d", (int)a);
    } else {
        snprintf(buf, bufSize, "<?%d>", TAG(a));
    }
}

    /* else if (IS_SHORT_STR(a)) {
        int sz = TAG(a) - T_STR;
        char tmp[8] = {0};
        memcpy(tmp, (char *)&a, sz);
        snprintf(buf, bufSize, "\"%s\"", tmp);
        } */

void printValue(Value a) {
    char buf[256];
    printValue(buf, sizeof(buf), a);
    fprintf(stderr, "%s\n", buf);
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
        case JMP:
            printf("%3s   %3s\n", sa, sb);
            break;

        case RET:
            printf("%3s\n", sa);
            break;

        case MOVE:
            printf("%3s,  %3s\n", sc, sa);
            break;

        case GET:
            printf("%3s,  %3s %3s\n", sc, sa, sb);
            break;

        case SET:
            printf("%3s %3s,  %3s\n", sc, sa, sb);
            break;
            
        default:
            printf("%3s,  %3s %3s\n", sc, sa, sb);
        }
    }
}

void printProto(Proto *proto) {
    printf("UpVals:\n");
    int i = 4;
    int c = 0;
    char buf[64];
    for (short *p = proto->ups.buf+4, *end = p-4 + proto->ups.size; p < end; ++p, ++i) {
        int u = *p;
        if (u > 0) {
            printf("%2d: %d\n", i, u-1);
        } else if (u < 0) {
            printf("%2d: [%d]\n", i, -u-1);
        } else {
            Value a = proto->consts.buf[c++];
            printValue(buf, sizeof(buf), a);
            printf("%2d: #%s\n", i, buf);
            if (IS_PROTO(a)) { printProto((Proto *)a); }
        }
    }
    printf("\nCode:\n");
    printBytecode(proto->code.buf, proto->code.size);
}

void printFunc(Func *func) {
    printProto(func->proto);
}
