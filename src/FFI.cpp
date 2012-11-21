#include "FFI.h"
#include "CFunc.h"
#include "Object.h"
#include "String.h"

#include <string.h>
#include <dlfcn.h>
#include <stdio.h>

#define SIZE(t) (((t) & CPTR) ? sizeof(void*) : typeSize[(t) & 0xf])

const char *nextType(const char *s, const char *delims, int *len, const char **next) {
    while (*s == ' ') { ++s; }
    const char *p = s;
    while (*p && !strchr(delims, *p)) { ++p; }
    const char *p2 = p-1;
    while (*p2 == ' ') { --p2; }
    *len = p2 - s + 1;
    if (*p) { ++p; }
    *next = *p ? p : 0;
    return s;    
}

#define CHAR_PTR (CPTR|CCHAR)
#define CONST_CHAR_PTR (CCONST|CPTR|CCHAR)

static int strToType(const char *s, int len) {
    const char *names[] = {
        "void", "const char *", "char *", "void *", 
        "int", "unsigned", "long", "unsigned long", "long long", "unsigned long long",
        "double", "char", "...", "ptrdiff",
    };
    int types[] = {
        CVOID, CONST_CHAR_PTR, CHAR_PTR, CPTR|CVOID,
        CINT, CINT, CLONG, CLONG, CLONGLONG, CLONGLONG,
        CDOUBLE, CINT, CELLIPSE, PTRDIFF,
    };
    int n = sizeof(types) / sizeof(types[0]);
    for (int i = 0; i < n; ++i) {
        if (!strncmp(s, names[i], len)) {
            return types[i];
        }
    }
    return -1;
}

int parseTypesC(const char *decl, int maxSize, byte *outTypes, byte *retType, bool *hasEllipsis) {
    *retType = CVOID;
    *hasEllipsis = false;
    const char *next;
    int len;
    const char *s = nextType(decl, "(", &len, &next);
    int t = strToType(s, len);
    ERR(t < 0, E_FFI_INVALID_SIGNATURE);
    *retType = t;
    int n = 0;
    while (next) {
        s = nextType(next, ",)", &len, &next);
        t = strToType(s, len);
        ERR(t < 0, E_FFI_INVALID_SIGNATURE);
        outTypes[n++] = t;
    }
    if (n > 1 && outTypes[n-1]==CELLIPSE) {
        --n;
        *hasEllipsis = true;
        int prev = outTypes[n-1];
        ERR(!(prev==CONST_CHAR_PTR || prev==CHAR_PTR), E_FFI_INVALID_SIGNATURE);
    }
    for (int i = 0; i < n; ++i) {
        ERR(outTypes[i] == CELLIPSE, E_FFI_INVALID_SIGNATURE);
    }
    return n;
}

int parseTypesVararg(const char *decl, int maxSize, byte *outTypes) {
    int n = 0;
    const char *p = decl;
    while (n < maxSize) {
        p = strchr(p, '%');
        if (p == 0) { return n; }
        char c;
        while ((c=*p) && !(('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'))) { ++p; }
        int lng = 0;
    again:
        c = *p;
        if (!c) { return n; }
        int t = 0;
        switch (c) {
        case 's': t = CPTR | CCHAR; break;
        case 'p': t = CPTR; break;
        case 'd': case 'u': case 'c': case 'x':
            t = lng==0 ? CINT : lng==1 ? CLONG : CLONGLONG; 
            break;
        case 'e': case 'E': case 'f': case 'F': case 'g': case 'G':
            t = CDOUBLE;
            break;
        case 'l': ++lng; ++p; goto again;
        }
        outTypes[n++] = t;
    }
    return n;
}

u64 castValue(Value &v, int t) {
    if ((t & CPTR) && (t&0xf)==CCHAR) { // char*
        if (IS_STRING(v)) {
            return (u64) GET_CSTR(v);
        }
    } else if (t & CPTR) {
        // TODO: array -> pointer
    } else if ((t & 0xf) == CDOUBLE) {
        ERR(!IS_NUMBER(v), E_FFI_TYPE_MISMATCH);
        u64 tmp;
        *(double*)(&tmp) = getDouble(v);
        printf("* %d %f %llx\n", t, getDouble(v), tmp);
        return tmp;
    } else if (t==CCHAR || t==CINT || t==CLONG || t==CLONGLONG) {
        if (IS_INT(v)) {
            return getInteger(v);
        }
    }
    return 0;
}

unsigned makeCode(byte *types, int n) {
    static const int typeSize[] = {0, 4, 4, sizeof(long), 8, 8};
    unsigned code = 1;
    for (int i = 0; i < n; ++i) {
        int t = types[i];
        code = (code << 1) | (SIZE(t) >> 3);
    }
    return code;
}

static Value ffiConstructInt(Value *stack, int nCallArg) {
    ERR(nCallArg != 2, E_FFI_N_ARGS);
    Value a = stack[0], b = stack[1];
    ERR(!IS_STRING(a) || !IS_STRING(b), E_FFI_TYPE_MISMATCH);
    void *func = dlsym(0, GET_CSTR(a));
    if (!func) { return NIL; }
    FFIData d;
    d.f = func;
    d.nArg = parseTypesC(GET_CSTR(b), 8, d.argType, &d.retType, &d.hasEllipsis);
    if (d.nArg < 0) { return NIL; }
    CFunc *cfunc = CFunc::alloc((tfunc) ffiCall, sizeof(FFIData));
    *(FFIData *)(cfunc->data) = d;
    return VAL_OBJ(cfunc);
}

void ffiConstruct(int op, void *data, Value *stack, int nCallArg) {
    stack[0] = op == 0 ? ffiConstructInt(stack, nCallArg) : NIL;
}

void ffiCall(int op, FFIData *d, Value *stack, int nCallArg) {
    if (op == 0) {
        u64 p1[8] = {0};        
        int nArg = d->nArg;
        byte *argType = d->argType;
        for (int i = 0; i < nArg; ++i) {
            p1[i] = castValue(stack[i], argType[i]);
        }
        unsigned code = makeCode(argType, nArg);        
        bool hasEllipsis = d->hasEllipsis;
        if (hasEllipsis) {
            ERR(nCallArg < nArg, E_FFI_N_ARGS);
            Value v = stack[nArg-1];
            ERR(!IS_STRING(v), E_FFI_TYPE_MISMATCH);
            const char *str = GET_CSTR(v);
            byte elipseTypes[8];
            int n = parseTypesVararg(str, 8, elipseTypes);
            for (int i = 0; i < n; ++i) {
                printf("elipse %d\n", (int) elipseTypes[i]);
            }
            ERR(nCallArg != nArg + n, E_FFI_N_ARGS);
            for (int i = nArg; i < nCallArg; ++i) {
                p1[i] = castValue(stack[i], elipseTypes[i - nArg]);
            }
            code = (code>>1) | (makeCode(elipseTypes, n) << 4);
            printf("* code %x\n", code);
        }
        unsigned p0[8] = {0};
        for (int i = 0; i < nCallArg; ++i) {
            p0[i] = (unsigned) p1[i];
        }
        void *f = d->f;

        typedef unsigned T0;
        typedef u64 T1;

#define F0        case 0b1:          ret = ((u64 (*)()) f)(); break;
#define F1(a)     case 0b1##a:       ret=((u64(*)(T##a))f)(p##a[0]); break;
#define F2(a,b)   case 0b1##a##b:    ret=((u64(*)(T##a,T##b))f)(p##a[0],p##b[1]); break;
#define F3(a,b,c) case 0b1##a##b##c: ret=((u64(*)(T##a,T##b,T##c))f)(p##a[0],p##b[1],p##c[2]); break;
#define FF F0; F1(0); F1(1); F2(0,0); F2(0,1); F2(1,0); F2(1,1); F3(0,0,0); F3(0,0,1); F3(0,1,0); F3(0,1,1); F3(1,0,0); F3(1,0,1); F3(1,1,0); F3(1,1,1);
        u64 ret = 0;
        switch (code) {
            FF;

#undef F0
#undef F1 
#undef F2 
#undef F3

#define F0 \
        case 0b10001: ret=((u64(*)(        char*, ...))f)((char*)p1[0]); break; \
        case 0b10010: ret=((u64(*)(int,    char*, ...))f)(p0[0], (char*)p1[1]); break; \
        case 0b10011: ret=((u64(*)(u64, char*, ...))f)(p1[0], (char*)p1[1]); break; \
        case 0b10100: ret=((u64(*)(int, int,       char*, ...))f)(p0[0], p0[1], (char*)p1[2]); break; \
        case 0b10101: ret=((u64(*)(int, u64,    char*, ...))f)(p0[0], p1[1], (char*)p1[2]); break; \
        case 0b10110: ret=((u64(*)(u64, int,    char*, ...))f)(p1[0], p0[1], (char*)p1[2]); break; \
        case 0b10111: ret=((u64(*)(u64, u64, char*, ...))f)(p1[0], p1[1], (char*)p1[2]); break;
            
#define F1(a) \
        case 0b1##a##0001: ret=((u64(*)(        char*, ...))f)((char*)p1[0], p##a[1]); break; \
        case 0b1##a##0010: ret=((u64(*)(int,    char*, ...))f)(p0[0], (char*)p1[1], p##a[2]); break; \
        case 0b1##a##0011: ret=((u64(*)(u64, char*, ...))f)(p1[0], (char*)p1[1], p##a[2]); break; \
        case 0b1##a##0100: ret=((u64(*)(int, int,       char*, ...))f)(p0[0], p0[1], (char*)p1[2], p##a[3]); break; \
        case 0b1##a##0101: ret=((u64(*)(int, u64,    char*, ...))f)(p0[0], p1[1], (char*)p1[2], p##a[3]); break; \
        case 0b1##a##0110: ret=((u64(*)(u64, int,    char*, ...))f)(p1[0], p0[1], (char*)p1[2], p##a[3]); break; \
        case 0b1##a##0111: ret=((u64(*)(u64, u64, char*, ...))f)(p1[0], p1[1], (char*)p1[2], p##a[3]); break;

#define F2(a, b) \
        case 0b1##a##b##0001: ret=((u64(*)(        char*, ...))f)((char*)p1[0], p##a[1], p##b[2]); break; \
        case 0b1##a##b##0010: ret=((u64(*)(int,    char*, ...))f)(p0[0], (char*)p1[1], p##a[2], p##b[3]); break; \
        case 0b1##a##b##0011: ret=((u64(*)(u64, char*, ...))f)(p1[0], (char*)p1[1], p##a[2], p##b[3]); break; \
        case 0b1##a##b##0100: ret=((u64(*)(int, int,       char*, ...))f)(p0[0], p0[1], (char*)p1[2], p##a[3], p##b[4]); break; \
        case 0b1##a##b##0101: ret=((u64(*)(int, u64,    char*, ...))f)(p0[0], p1[1], (char*)p1[2], p##a[3], p##b[4]); break; \
        case 0b1##a##b##0110: ret=((u64(*)(u64, int,    char*, ...))f)(p1[0], p0[1], (char*)p1[2], p##a[3], p##b[4]); break; \
        case 0b1##a##b##0111: ret=((u64(*)(u64, u64, char*, ...))f)(p1[0], p1[1], (char*)p1[2], p##a[3], p##b[4]); break;

#define F3(a, b, c) \
        case 0b1##a##b##c##0001: ret=((u64(*)(        char*, ...))f)((char*)p1[0], p##a[1], p##b[2], p##c[3]); break; \
        case 0b1##a##b##c##0010: ret=((u64(*)(int,    char*, ...))f)(p0[0], (char*)p1[1], p##a[2], p##b[3], p##c[4]); break; \
        case 0b1##a##b##c##0011: ret=((u64(*)(u64, char*, ...))f)(p1[0], (char*)p1[1], p##a[2], p##b[3], p##c[4]); break; \
        case 0b1##a##b##c##0100: ret=((u64(*)(int, int,       char*, ...))f)(p0[0], p0[1], (char*)p1[2], p##a[3], p##b[4], p##c[5]); break; \
        case 0b1##a##b##c##0101: ret=((u64(*)(int, u64,    char*, ...))f)(p0[0], p1[1], (char*)p1[2], p##a[3], p##b[4], p##c[5]); break; \
        case 0b1##a##b##c##0110: ret=((u64(*)(u64, int,    char*, ...))f)(p1[0], p0[1], (char*)p1[2], p##a[3], p##b[4], p##c[5]); break; \
        case 0b1##a##b##c##0111: ret=((u64(*)(u64, u64, char*, ...))f)(p1[0], p1[1], (char*)p1[2], p##a[3], p##b[4], p##c[5]); break;
            
            FF;
            
        }
        Value v = NIL;
        switch (d->retType) {
        case CVOID:     v = NIL; break;            
        case CINT:      v = VAL_INT((int)  ret); break;
        case CLONG:     v = VAL_INT((long) ret); break;
        case CLONGLONG: v = VAL_INT(ret);        break;
        case CDOUBLE:   v = VAL_DOUBLE(*(double*)&ret); break;
        case CCONST|CPTR|CCHAR:
        case CPTR|CCHAR: {
            char *p = (char *) ret;
            v = String::makeVal(p, strlen(p));
            break;
        }
        case PTRDIFF: v = VAL_INT(ret - p1[0]); break;
        }

        stack[0] = v;
    }
}
