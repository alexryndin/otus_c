# HW13

<!-- vim-markdown-toc GFM -->

* [Сборка и тесты](#Сборка-и-тесты)
    * [Требования к сборке](#Требования-к-сборке)
    * [Как собрать](#Как-собрать)
    * [Как собрать с дебагом](#Как-собрать-с-дебагом)
    * [Как запускать](#Как-запускать)
* [Результаты нагрузочного тестирования](#Результаты-нагрузочного-тестирования)
* [Фичи, особенности](#Фичи-особенности)

<!-- vim-markdown-toc -->

## Сборка и тесты
### Требования к сборке
* Наличие GNU make.
### Как собрать
```
make
```
### Как собрать с дебагом
```
make dev
```
### Как запускать
```
Usage: Usage: ./main port num_of_workers"
```
* port -- на какой порт забиндиться.
* num_of_workers -- количество воркеров.

## Результаты нагрузочного тестирования
Сервер запущен с одним тредом
```
$  ./wrk -t1 -c1 -d1s --no-keepalive http://127.0.0.1:8080/index.html
Running 1s test @ http://127.0.0.1:8080/index.html
  1 threads and 1 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   116.48us   34.48us   1.25ms   88.31%
    Req/Sec     5.60k   469.26     6.61k    63.64%
  6128 requests in 1.10s, 3.96MB read
Requests/sec:   5571.01
Transfer/sec:      3.60MB
```
Сравним с nginx
```
$ ./wrk -t1 -c1 -d1s --no-keepalive http://127.0.0.1:80/index.html
Running 1s test @ http://127.0.0.1:80/index.html
  1 threads and 1 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    79.45us  351.20us   7.42ms   99.36%
    Req/Sec    10.07k   602.49    11.09k    63.64%
  11012 requests in 1.10s, 8.93MB read
Requests/sec:  10016.28
Transfer/sec:      8.12MB
```
Как видим, производительность в пару раз меньше. Я думаю это связано прежде
всего с тем, что я отправляю header и body двумя разными запросами, это очевидная
точка для оптимизации, но пока не придумал, как побороть её красиво в
архитектурном плане.

Теперь запусти с двумя тредами и повторим.
```
$ ./wrk -t1 -c1 -d1s --no-keepalive http://127.0.0.1:8080/index.html
Running 1s test @ http://127.0.0.1:8080/index.html
  1 threads and 1 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    47.00us   15.76us 369.00us   91.36%
    Req/Sec    10.41k   756.75    11.18k    81.82%
  11401 requests in 1.10s, 7.36MB read
Requests/sec:  10370.55
Transfer/sec:      6.70MB
```
Чтож, уже ближе к однотредовому nginx :)

Мне кажется весьма неплохо

## Фичи, особенности
Изначальное стремление выжать по максимуму rps было заменено стремлением
сделать как-нибудь. Всё же самостоятельно разобрать http, при этом стараясь
отвечать клиентам асинхронно оказалось довольно сложной задачей. Тем не менее:
* Реализована поддержка многопоточной обработки. При старте можно указать,
сколько тредов будут обрабатывать запросы
* EPOLLET + EPOLLEXCLUSIVE по идее гарантируют, что событие будет доставлено
только в один поток
* Нет нормальной обработки постоянных соединений. Ожидается, что клиент
разорвёт соединение как только получит ожидаемый набор данных
