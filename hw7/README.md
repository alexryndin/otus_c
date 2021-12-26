# HW7
## Как собрать и запустить
```
make
```
## Анализ утечек после фикса
```
==38846== Using Valgrind-3.17.0 and LibVEX; rerun with -h for copyright info
==38846== Command: ./asm-prog
==38846==
4 8 15 16 23 42
23 15
==38846==
==38846== HEAP SUMMARY:
==38846==     in use at exit: 0 bytes in 0 blocks
==38846==   total heap usage: 9 allocs, 9 frees, 1,152 bytes allocated
==38846==
==38846== All heap blocks were freed -- no leaks are possible
==38846==
==38846== For lists of detected and suppressed errors, rerun with: -s
==38846== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```
