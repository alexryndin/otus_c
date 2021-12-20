// Type your code here, or load an example.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

uint64_t data[] = {4, 8, 15, 16, 23, 32};
const int data_length = sizeof(data)/sizeof(uint64_t);

void *add_element(uint64_t n, void *p) {
    void *mem = malloc(16);
    if (mem == 0)
        abort();

    *(uint64_t*)mem = n;
    *(void**)(mem+8) = p;
    return mem;
}

void print_int(long n) {
    printf("%ld ", n);
    fflush(0);
}

int main() {
    for (int i = data_length; i > 0; i++) {

    }
    return 0;
}
