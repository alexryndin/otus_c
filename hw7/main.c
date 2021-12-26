#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <assert.h>
static_assert(sizeof(void*) == 8, "Pointers are assumed to be exactly 8 bytes");

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

void *f(void *ptr, void *acc, uint64_t (*fun)(uint64_t n)) {
    uint64_t ret = 0;
    if (!ptr)
        goto outf;

    ret = fun(*(uint64_t *)ptr);
    if (ret != 0)
        acc = add_element(*(uint64_t *)ptr, (void *)acc);

    acc = f(*(void **)(ptr+8), acc, fun);

outf:
    return acc;
}

void m(uint64_t a, void (*fun)(long n)) {
    uint64_t rbx;

    if (!a)
        goto outm;

    rbx = a;

    a = *(uint64_t *)a;
    fun(a);

    m(*(uint64_t *)(rbx + 8), fun);

outm:
    return;
}

uint64_t p(uint64_t a) { return a & 1; }

void print_int(long n) {
    printf("%ld ", n);
    fflush(0);
}

int main() {
    void *list = 0;
    void *ret;

    for (int i = data_length; i > 0; i--) {
        list = add_element(data[i - 1], (void *)list);
    }
    m((uint64_t)list, print_int);
    puts(empty_str);
    ret = f(list, 0, p);
    m((uint64_t)ret, print_int);
    puts(empty_str);
    return 0;
}
