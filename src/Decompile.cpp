// Copyright (C) 2012 - 2013 Mihai Preda

#include "Decompile.h"
#include "Value.h"
#include "VM.h"
#include "Proto.h"
#include "Object.h"
#include "String.h"
#include "StringBuilder.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#define _(n) #n
static const char *opNames[] = {
    #include "opcodes.inc"
};
#undef _

void printValue(char *buf, int bufSize, Value a) {
    StringBuilder sb;
    sb.append(a);
    snprintf(buf, bufSize, "%s", sb.cstr());
}

static void printOperand(char *buf, int bufSize, int v) {
    snprintf(buf, bufSize, "%d", v);
}

void printProto(Proto *proto, int indent);

static void printBytecode(unsigned *start, int size, int indent, bool dump) {
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
        if (dump) {
            printf("%*s%02d: %02x%02x%02x%02x   %-6s ", indent, "", i, op, c, a, b, opNames[op]);
        } else {
            printf("%*s%02d: %-6s ", indent, "", i, opNames[op]);
        }
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
            printf("%3d, %s\n", c, buf);            
            p += 2;
            
            if (IS_PROTO(v)) {
                printProto((Proto *) GET_OBJ(v), indent + 4);
            }

            break;
        }
            
        case MOVE_R:
        case LEN:
        case FUNC:
        case NOTL:
            printf("%3s,  %3s\n", sc, sa);
            break;

        case GETI:
        case GETF:
            printf("%3s,  %3s %3s\n", sc, sa, sb);
            break;

        case SETI:
        case SETF:
            printf("%3s %3s,  %3s\n", sc, sa, sb);
            break;
            
        default:
            printf("%3s,  %3s %3s\n", sc, sa, sb);
        }
    }
}

void printProto(Proto *proto, int indent) {
    StringBuilder buf;
    int n = proto->ups.size();
    for (int i = 0; i < n; ++i) {
        buf.append((double) proto->ups.get(i));
        buf.append(' ');
    }
    printf("%*sProto %s\n", indent, "", buf.cstr());
    printBytecode(proto->code.buf(), proto->code.size(), indent, false);
}

void printFunc(Func *func) {
    printProto(func->proto, 0);
}
