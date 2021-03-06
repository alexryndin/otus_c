
<!-- vim-markdown-toc GFM -->

* [HW14](#hw14)
    * [Сборка и тесты](#Сборка-и-тесты)
        * [Требования к сборке](#Требования-к-сборке)
        * [Как собрать](#Как-собрать)
        * [Как собрать с дебагом](#Как-собрать-с-дебагом)
        * [Как запускать](#Как-запускать)
        * [Как протестировать](#Как-протестировать)
    * [Фичи, особенности](#Фичи-особенности)
    * [Вопросы](#Вопросы)

<!-- vim-markdown-toc -->

# HW14
## Сборка и тесты
### Требования к сборке
* Наличие GNU make.
* Наличие в системе libpq и libsqlite
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
Usage: ./main db_type db_url table column"
```
* db_type -- тип используемой субд. На выбор -- sqlite3 и postgresql
* db_url -- url базы данных. Для postgresql это адрес в формате
`postgres://postgres:qwerty@localhost`, для sqlite3 -- путь к файлу
* table -- Имя таблицы, по которой будет считаться статистика.
* column -- имя колонки, по которой будет считаться статистика. Поддерживаются
только целочисленные колонки.
### Как протестировать
Так как это задание довольно комплесное и тянет разные библиотеки и модули, то для
тестирования подготовлен docker-compose.yaml файл, вместе с Dockerfile для
сборки готового приложения.

Таким образом есть два варианта как протестировать приложение:
* На локальной системе установить все необходимые библиотеки и запустить `make`
* `sudo docker-compose up`

Второй способ считаю предпочтительным, так как он гарантирует повторение сборки

После запуска увидим примерно следующее:
```
[+] Running 2/0
 ⠿ Container hw14-pg-1    Created                                                                       0.0s
 ⠿ Container hw14-hw14-1  Created                                                                       0.0s
Attaching to hw14-hw14-1, hw14-pg-1
hw14-pg-1    |
hw14-pg-1    | PostgreSQL Database directory appears to contain a database; Skipping initialization
hw14-pg-1    |
hw14-pg-1    | 2022-02-07 10:39:59.913 UTC [1] LOG:  starting PostgreSQL 13.5 (Debian 13.5-1.pgdg110+1) on x86_64-pc-linux-gnu, compiled by gcc (Debian 10.2.1-6) 10.2.1 20210110, 64-bit
hw14-pg-1    | 2022-02-07 10:39:59.914 UTC [1] LOG:  listening on IPv4 address "0.0.0.0", port 5432
hw14-pg-1    | 2022-02-07 10:39:59.914 UTC [1] LOG:  listening on IPv6 address "::", port 5432
hw14-pg-1    | 2022-02-07 10:39:59.916 UTC [1] LOG:  listening on Unix socket "/var/run/postgresql/.s.PGSQL.5432"
hw14-pg-1    | 2022-02-07 10:39:59.920 UTC [26] LOG:  database system was shut down at 2022-02-07 10:32:56 UTC
hw14-pg-1    | 2022-02-07 10:39:59.926 UTC [1] LOG:  database system is ready to accept connections
hw14-hw14-1  | stddev     sum        avg        min        max
hw14-hw14-1  | 25.8360213 175508     1972.00000 1928       2016
hw14-hw14-1  | stdev(t.c) sum(t.c)   avg(t.c)   min(t.c)   max(t.c)
hw14-hw14-1  | 25.8360213 175508     1972.0     1928       2016
hw14-hw14-1 exited with code 0
```
## Фичи, особенности
* Реализация обобщённых векторов :)
Очень понравилась идея, которую во всю использует автор библиотека klib, а именно:
создание обобщённых контейнеров с исользованием стандартных сишных макросов.  
К сожалению, klib мне показалась очень далёкой от production-quality библиотеки,
поэтому принял решение сделать похожее самостоятельно :)  
Файлик rvec.h как раз содержит реализацию такого вектора. Основная идея в том,
что вовсе не обязательно терять тип вектора и орудовать сплошными `void*`, если
создавать необходимые структуры ad-hoc с помощью удобных макросов.  
В dbw.h, о котором ниже, как раз можно найти пример активного использования этого
подхода: структура DWBResult хранит в себе векторы разных типов: вектор строк с 
названиями колонок и вектор целых чисел с типами колонок.
* Обобщённый интерфейс к разным базам данных :)  
Чтобы основной код оставался красивым и удобочитаемым, был реализован интерфейс 
и соответствующая прослойка (DataBase Wrapper -- DBW) к двум различным СУБД -- 
PostgreSQL и SQLite3.  
Прослойка позоволяет обобщить базовые функции над СУБД:
  * Подключение (dbw_connect)
  * Закрытие (dbw_close)
  * Запрос (dbw_query)
  * Запрос описания указанной таблицы (колонки и их типы) (dbw_get_table_types)
  * Вывести результат на экран (dbw_print)
 
Таким образом, реализовав необходимые функции можно сделать интерфейс к любой СУБД.  
В рамках дз, однако, была реализована поддержка только целочисленных типов.  
* Поскольку SQLite беден на стастистические функции типа stddev, было принято
решение собрать и подлкючить сторонний модуль `extension-functions.so`,
содержащий искомые функции.

## Вопросы
C, как оказалось, очень не любит, когда кто-то начинает играть с типами :)

Например, попытаемся создать функцию, которая возвращает ссылку на rvec_t(bstring):
```
rvec_t(bstring) *foo() {
    rvec_t(bstring) *t;

    return t;
}
```
Получаем очень интересную ошибку:
```
lib/dbw.c:494:12: warning: returning ‘struct <anonymous> *’ from a function with incompatible return type ‘struct <anonymous> *’ [-Wincompatible-pointer-types]
```
Анонимная стуктура != Анонимная структура?

Если же попытаться возвращать структуру вместо ссылки, то получаем уже ошибку, вместо предупреждения:
```
lib/dbw.c:494:12: error: incompatible types when returning type ‘struct <anonymous>’ but ‘struct <anonymous>’ was expected
```

В принципе понятно, что типов в сях нет :) а саму эту ошибку таки можно обойти,
заранее объявив наш вектор отдельным типом:
```
typedef rvec_t(bstring) rvec_t_bstring;
```
Но как-то это немного костыльно...

Можно ли лучше?
