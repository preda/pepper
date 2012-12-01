#include "Decompile.h"
#include "Value.h"
#include "VM.h"
#include "Proto.h"
#include "Object.h"
#include "String.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

static const char *objTypeName[] = {
    0, "ARRAY", "MAP", "FUNC", "CFUNC", "PROTO", "O_STR",
};

#define _(n) #n
static const char *opNames[] = {
    #include "opcodes.inc"
};
#undef _

/*
static const char *typeNames[] = {
    "NIL", "[]", "{}", 0,
    0, 0, 0, 0,
    "\"\"",
};
*/

static void printValue(char *buf, int bufSize, Value a) {
    if (IS_NUM(a)) {
        snprintf(buf, bufSize, "%f", GET_NUM(a));
    } else if (IS_NIL(a)) {
        snprintf(buf, bufSize, "NIL");
    } else if (IS_STRING(a)) {
        snprintf(buf, bufSize, "\"%s\"", GET_CSTR(a));
    } else if (IS_OBJ(a)) {
        int type = O_TYPE(a);
        snprintf(buf, bufSize, "%5s %p", objTypeName[type], GET_OBJ(a));
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

static void printOperand(char *buf, int bufSize, int v) {
    snprintf(buf, bufSize, "%d", v);
}

void printBytecode(unsigned *start, int size) {
    // static bool inited = init();
    // (void) inited;
    assert(sizeof(opNames) / sizeof(opNames[0]) == N_OPCODES);
    char sa[32], sb[32], sc[32];
    char buf[256];
    for (unsigned *p = start, *end = start + size; p < end; ++p) {
        unsigned code = *p;
        int i = p - start;
        int op = OP(code), a = OA(code), b = OB(code), c = OC(code);
        printOperand(sa, sizeof(sa), a);
        printOperand(sb, sizeof(sb), b);
        printOperand(sc, sizeof(sc), c);
        printf("%2d: %02x%02x%02x%02x   %-6s ", i, op, c, a, b, opNames[op]);
        switch (op) {
        case JMP:    printf("%3d\n", i + OD(code) + 1); break;

        case JF: 
        case JT:
        case FOR:
        case LOOP:
            printf("%3d,  %3d\n", i + OD(code) + 1, c); break;

        case JLT:
        case JNIS: printf("%3d,  %3d %3d\n", i + OSC(code) + 1, a, b); break;

        case RET:    printf("%3d\n", a); break;

        case MOVE_I: printf("%3d,  #%d\n", c, (int)OD(code)); break;

        case MOVE_C: {
            Value v = *(p+1) | ((u64)*(p+2) << 32);
            printValue(buf, sizeof(buf), v);
            printf("%3d, #%s\n", c, buf);            
            p += 2;
            break;
        }
            
        case MOVE_R:
        case LEN:
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
    /*
    printf("UpVals: ");
    int i = 0;
    // char buf[64];
    for (short *p = proto->ups.buf+i, *end = p-i + proto->ups.size; p < end; ++p, ++i) {
        printf("%3d, ", (int)*p);
    }
    printf("\n\nCode:\n");
    */
    printBytecode(proto->code.buf, proto->code.size);
}

void printFunc(Func *func) {
    printProto(func->proto);
}
