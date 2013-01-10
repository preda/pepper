#include <stdio.h>
#include <dlfcn.h>
#include <string.h>

int main() {
    void *h = dlopen("libc.so", 0);
    printf("handle %p\n", h);
    const void *p = dlsym(h, "strlen");
    printf("%p\n", p);

    /*
    ffi_cif cif;
    ffi_type *signature[] = {&ffi_type_pointer, &ffi_type_double};
    ffi_prep_cif_var(&cif, FFI_DEFAULT_ABI, 1, 2, &ffi_type_sint, signature);

    void (*f)(void) = (void(*)(void))&printf;
    ffi_raw args[10];
    args[0].ptr = (void *) "hello %g\n";
    *(double*)& (args[1]) = 2.5;
    long long ret;
    ffi_raw_call(&cif, f, &ret, args);
    */
}
