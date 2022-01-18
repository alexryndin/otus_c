# HW10
## Требования к сборке
Наличие GNU make.
## Как собрать
```
make
```
## Как собрать с дебагом
```
make dev
```
## Как запускать
```
Usage: ./main filepath block_size [just_read|mmap]
```
block_size -- размер блока данных, который будет аллоцирован в качестве буфера.
just_read -- читать файл многочисленным вызовом fread.
mmap -- смапить файл в память и выкидывать лишние куски по ходу работы.
## Тест скорости работы
Первый запуск, блок 5к мегабайт
```
$ time ./main file.bin 5000 just_read
deadbeef
./main file.bin 5000 just_read  267.16s user 23.26s system 85% cpu 5:39.80 total
$  time ./main file.bin 5000 mmap
deadbeef
./main file.bin 5000 mmap  265.32s user 14.16s system 98% cpu 4:43.89 total
```
mmap показал себя почти на минуту быстрее.

Второй запуск, сутки спустя:

```
$ time ./main file.bin 5000 just_read
deadbeef
./main file.bin 5000 just_read  311.01s user 27.40s system 87% cpu 6:28.46 total
$ time ./main file.bin 5000 mmap
deadbeef
./main file.bin 5000 mmap  350.45s user 17.03s system 98% cpu 6:14.37 total
```
Преимущества уже не такие явные :(


