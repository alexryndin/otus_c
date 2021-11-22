# HW6
## Как собрать
```
make
```
# Как собрать dev сборку
```
make dev
```
# Как запустить
```
./bin/main sink|stderr [sink|stderr...]
```
sink -- имя файла
# Что произойдёт после запуска
В указанные sink's будет записано 10к строк с рандомными сообщениями.
# Детали, особенности, вопросы
* Многопоточная безопасность реализована через mutex. По скорости получается
не очень, так как второй тред ждёт, когда запишет первый. Насколько я понимаю,
функции записи (printf, vfprintf) могут быть вызваны безопасно из разных
потоков, но я это не смог на 100% проверить.
* С трейс-логом отдельная сложность. Так, сначала мы должны вызвать функцию
backtrace, которая к тому же не переносима (only linux). К тому же, она
должна сложить указатели в отдельный буфер. malloc'ать этот буфер, по мне
это такая себе идея как для логгера, поэтому я его создаю заранее в структуре
Logger. Далее, мы должны пресечь возможность перетереть этот буфер из другого
треда, поэтому макрос `LOGGER_FATAL` прежде всего лочит логгер, и далее уже
вызывает функцию `logger_log_fatal`, задача которого будет разлочить логгер.
Почему не перенести лок в `logger_log_fatal`? Потому что тогда `backtrace()`
выдаст неправльный список функций.
* Можно было бы перед локом логгера готовить данные к записи в некоем буфере,
но опять же, это лишние malloc'и, которые я старался по максимуму избегать в
пользу явных локов при записи.
* Получилось не оч быстро -- 1,5 секунды на 10к строк.
* UPD1 Поизучал и поиспользовал perf. Как оказалось, предыдущая версия много
времени тратила на rand() (кто бы мог подумать). Подрезал rand(), сохранив
функционал. Теперь основной оверхед добавляет функция `_dl_addr`. Откуда,
как и почему :) ?:
```
Samples: 6K of event 'cycles:u', Event count (approx.): 3471750932
Overhead  Command  Shared Object       Symbol
  67.11%  main     libc-2.33.so        [.] _dl_addr
   8.97%  main     main                [.] fuzzer
   5.30%  main     libc-2.33.so        [.] __vfprintf_internal
   1.34%  main     libc-2.33.so        [.] __strftime_internal
   1.20%  main     libgcc_s.so.1       [.] execute_cfa_program
   1.18%  main     libc-2.33.so        [.] _IO_file_xsputn@@GLIBC_2.2.5
   1.12%  main     libc-2.33.so        [.] _IO_default_xsputn
   0.96%  main     libpthread-2.33.so  [.] __pthread_mutex_lock
   0.95%  main     libgcc_s.so.1       [.] _Unwind_IteratePhdrCallback
   0.88%  main     libc-2.33.so        [.] __random
   0.80%  main     libc-2.33.so        [.] __strchrnul_avx2
   0.79%  main     libgcc_s.so.1       [.] uw_update_context_1
   0.78%  main     libpthread-2.33.so  [.] __pthread_mutex_unlock_usercnt
   0.75%  main     libc-2.33.so        [.] __memmove_avx_unaligned_erms
   0.62%  main     libc-2.33.so        [.] _itoa_word
   0.60%  main     libc-2.33.so        [.] __strlen_avx2
   0.47%  main     libc-2.33.so        [.] __tz_convert
   0.47%  main     main                [.] logger_log
   0.47%  main     libc-2.33.so        [.] fputc
   0.39%  main     libgcc_s.so.1       [.] uw_frame_state_for
```
