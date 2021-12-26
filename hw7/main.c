#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

uint64_t data[] = {4, 8, 15, 16, 23, 32};
const int data_length = sizeof(data) / sizeof(uint64_t);

const char empty_str[] = {0};

void *add_element(uint64_t n, void *p) {
    void *mem = malloc(16);
    if (mem == 0)
        abort();

    *(uint64_t *)mem = n;
    *((void **)(mem) + 1) = p;
    return mem;
}

uint64_t f(void *ptr, uint64_t acc, uint64_t (*fun)(long n)) {
    uint64_t ret = 0;
    uint64_t r12 = acc;
    if (!ptr)
        goto outf;

    ret = fun(*(uint64_t *)ptr);
    if (ret != 0)
        acc = add_element(*(uint64_t *)ptr, acc);

    acc = f(*(void **)(ptr+8), acc, fun);

outf:
    return acc;
}

void m(uint64_t a, void (*fun)(long n)) {
    uint64_t rbx;
    void *rbp;

    if (!a)
        goto outm;

    rbx = a;
    rbp = f;

    a = *(uint64_t *)a;
    fun(a);

    m(*(void **)(rbx + 8), fun);

outm:
    return;
}

uint64_t p(uint64_t a) { return a & 1; }

void print_int(long n) {
    printf("%ld ", n);
    fflush(0);
}

int main() {
    uint64_t rax = 0;
    int rbx = data_length;
    void *ret;

    for (int i = data_length; i > 0; i--) {
        rax = (uint64_t)add_element(data[i - 1], (void *)rax);
    }
    m(rax, print_int);
    puts(empty_str);
    ret = f(rax, 0, p);
    m(ret, print_int);
    puts(empty_str);
    return 0;
}
