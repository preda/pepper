#include <stdio.h>

int main() {
    unsigned long long a=1, b=-1, c=(1ULL<<48);
    __int128 h = 1;
    printf("%d %d %d %d\n", (int)a, (int)b, (int)c, 1/0);
}
