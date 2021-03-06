#include "FFI.h"
#include "CFunc.h"
#include "Object.h"
#include "String.h"
#include "VM.h"

#include <string.h>
#include <dlfcn.h>
#include <stdio.h>
#include <assert.h>

enum CTypes {
    RESERVED = 0,
    ELLIPSE,

    // return only
    VOID,

    // pointers
    PTRDIFF,
    CHAR_PTR,
    VOID_PTR,

    // 4byte
    INT,
    FLOAT,

    // 8byte
    LLONG,
    DOUBLE,

    // for pointer base only
    CHAR,
    SHORT,
};

ffi_type *ffiTypes[]  = {
    0, 0,
    &ffi_type_void, 
    &ffi_type_pointer, &ffi_type_pointer, &ffi_type_pointer,
    &ffi_type_sint32, &ffi_type_float,
    &ffi_type_sint64, &ffi_type_double,
};

static const char *nextType(const char *s, const char *delims, int *len, const char **next) {
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

static int strToType(const char *s, int len) {
    const char *names[] = {
        "void", "offset", "...",
        "char", "short", "int", "unsigned", 
        "long long", "float", "double",
    };

    int types[] = {
        VOID, PTRDIFF, ELLIPSE,
        CHAR, SHORT, INT, INT,
        LLONG, FLOAT, DOUBLE,
    };

    bool isPtr = false;
    if (s[len-1] == '*') {
        isPtr = true;
        --len;
        while (len>0 && s[len-1]==' ') {
            --len;
        }
    }
    if (len >= 6 && !strncmp(s, "const ", 6)) {
        s += 6; len -= 6;

    } else if (len >= 9 && !strncmp(s, "unsigned ", 9)) {
        s += 9; len -= 9;
    }
    while (len && *s == ' ') { ++s; --len; } 
    int n = sizeof(types) / sizeof(types[0]);
    for (int i = 0; i < n; ++i) {
        if (!strncmp(s, names[i], len)) {
            int t = types[i];            
            if (t == CHAR) {
                t = isPtr ? CHAR_PTR : INT;
            } else if (t == SHORT) {
                assert(!isPtr);
                t = INT;
            } else if (t == VOID && isPtr) {
                t = VOID_PTR;
            }
            return t;
        }
    }
    return 0;
}

static int parseTypesC(const char *decl, int maxSize, byte *outTypes, byte *retType, bool *hasEllipsis) {
    *retType = VOID;
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
        ERR(!t, E_FFI_INVALID_SIGNATURE);
        outTypes[n++] = t;
    }
    if (n > 1 && outTypes[n-1]==ELLIPSE) {
        --n;
        *hasEllipsis = true;
        int prev = outTypes[n-1];
        ERR(prev != CHAR_PTR, E_FFI_INVALID_SIGNATURE);
    }
    for (int i = 0; i < n; ++i) {
        ERR(outTypes[i] == ELLIPSE, E_FFI_INVALID_SIGNATURE);
    }
    ERR(*retType == PTRDIFF && (!n || outTypes[0] != CHAR_PTR), E_FFI_INVALID_SIGNATURE);
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
        case 's': t = CHAR_PTR; break;
        case 'p': t = VOID_PTR; break;
        case 'd': case 'u': case 'c': case 'x':
            ERR(lng==1, E_FFI_VARARG);
            t = lng==0 ? INT : LLONG; 
            break;
        case 'e': case 'E': case 'f': case 'F': case 'g': case 'G':
            t = DOUBLE;
            break;
        case 'l': ++lng; ++p; goto again;
        default: ERR(true, E_FFI_INVALID_SIGNATURE);
        }
        outTypes[n++] = t;
    }
    return n;
}

static void makeFfiSignature(ffi_type **sign, int nArg, byte *argType) {
    for (int i = 0; i < nArg; ++i) {
        sign[i] = ffiTypes[argType[i]];
    }
}

static Value ffiConstructInt(GC *gc, Value *stack, int nCallArg) {
    ERR(nCallArg != 4, E_FFI_N_ARGS);
    Value a = stack[1], b = stack[2], c = stack[3];
    ERR(!IS_STRING(a) || !IS_STRING(b), E_FFI_TYPE_MISMATCH);
    char buf[256];
    char *p = buf;
    if (strchr(GET_CSTR(c), '.')) {
        p = GET_CSTR(c);
    } else {
        snprintf(buf, sizeof(buf), "lib%s.so", GET_CSTR(c));
    }
    void *handle = dlopen(p, 0);
    void (*func)() = (void(*)()) dlsym(handle, GET_CSTR(a));
    if (!func) { return VNIL; }
    CFunc *cfunc = CFunc::alloc(gc, (tfunc) ffiCall, sizeof(FFIData));
    FFIData *d = (FFIData *) cfunc->data;
    d->func = func;
    // d->gc = gc;
    int nArg = d->nArg = parseTypesC(GET_CSTR(b), 8, d->argType, &d->retType, &d->hasEllipsis);
    if (nArg < 0) { return VNIL; }
    makeFfiSignature(d->ffiArgs, nArg, d->argType);
    if (!d->hasEllipsis) {
        int ret = ffi_prep_cif(&d->cif, FFI_DEFAULT_ABI, nArg, ffiTypes[d->retType], d->ffiArgs);
        ERR(ret != FFI_OK, E_FFI_CIF);
    }
    return VAL_OBJ(cfunc);
}

Value ffiConstruct(VM *vm, int op, void *data, Value *stack, int nCallArg) {
    return op == 0 ? ffiConstructInt(vm->getGC(), stack, nCallArg) : VNIL;
}

Value ffiCall(VM *vm, int op, FFIData *d, Value *stack, int nCallArg) {
    if (op != 0) { return VNIL; }
    ++stack;
    --nCallArg;
    const int nFixArg = d->nArg;
    int nArg = nFixArg;
    if (d->hasEllipsis) {
        ERR(nCallArg < nFixArg, E_FFI_N_ARGS);
        Value v = stack[nArg-1];
        ERR(!IS_STRING(v), E_FFI_TYPE_MISMATCH);
        const char *str = GET_CSTR(v);
        const int nVarArg = parseTypesVararg(str, 8-nArg, d->argType+nArg);
        nArg = nFixArg + nVarArg;
        makeFfiSignature(d->ffiArgs+nFixArg, nVarArg, d->argType+nFixArg);
        int ret = ffi_prep_cif_var(&d->cif, FFI_DEFAULT_ABI, nFixArg, nArg, ffiTypes[d->retType], d->ffiArgs);
        ERR(ret != FFI_OK, E_FFI_CIF);
    }
    ERR(nCallArg < nArg, E_FFI_N_ARGS);
    ffi_raw args[16];
    byte *argType = d->argType;
    ffi_raw *p = args;
    for (int i = 0; i < nArg; ++i, ++p) {
        Value &v = stack[i];
        switch (argType[i]) {
        case CHAR_PTR:
            ERR(!IS_STRING(v), E_FFI_TYPE_MISMATCH);
            p->ptr = GET_CSTR(v);
            break;

        case VOID_PTR:
            ERR(!IS_OBJ(v), E_FFI_TYPE_MISMATCH);
            p->ptr = (void *) v;
            break;

        case FLOAT:
            ERR(!IS_NUM(v), E_FFI_TYPE_MISMATCH);
            p->flt = GET_NUM(v);
            break;

        case INT:
            ERR(!IS_NUM(v), E_FFI_TYPE_MISMATCH);
            p->sint = (int) GET_NUM(v);
            break;

        case DOUBLE:
            ERR(!IS_NUM(v), E_FFI_TYPE_MISMATCH);
            *(double *)p = GET_NUM(v);
            p += sizeof(double) / sizeof(ffi_raw) - 1;
            break;

        case LLONG:
            ERR(!IS_NUM(v), E_FFI_TYPE_MISMATCH);
            *(long long *)p = (s64) GET_NUM(v);
            p += sizeof(long long) / sizeof(ffi_raw) - 1;
            break;

        default:
            assert(false);
        }
    }
    union {
        Value v;
        double dbl;
        float flt;
        int i;
        char *ptr;
    } ret;
    ffi_raw_call(&d->cif, d->func, &ret, args);

    Value v = VNIL;
    switch (d->retType) {
    case INT:      v = VAL_NUM(ret.i); break;
    case LLONG:    v = VAL_NUM(ret.v); break;
    case DOUBLE:   v = VAL_NUM(ret.dbl); break;
    case FLOAT:    v = VAL_NUM(ret.flt); break;
    case CHAR_PTR: v = String::value(vm->getGC(), ret.ptr, strlen(ret.ptr)); break;
    case PTRDIFF:  v = VAL_NUM(ret.ptr - GET_CSTR(stack[0])); break;
    case VOID: break;
    default: assert(false);
    }
    return v;
    // stack[-1] = v;
}
