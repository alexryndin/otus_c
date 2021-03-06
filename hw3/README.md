# hw3

## Требования
* GNU Make
* GCC or Clang or (possibly) tcc

## Сборка и тесты
```
make
```
## Сборка dev-версии
```
make dev
``` 

## Фичи
* Реализована хеш-мапа с открытой адресацией. В качестве хеш-функции используется функция Боба Дженкинса https://en.wikipedia.org/wiki/Jenkins_hash_function
* Хеш-мапа сильно опирается на ещё одну структуру, также реализованную в рамка дз -- FArray (Flexible Array),
по сути динамический массив, но с возможностью хранить как ссылки на структуры, так и сами структуры внутри массива.
* Всё по максимуму обмазано тестами
* valgrind.out содержит выхлоп после подсчёта слов в первом томике Войны и Мира
* В качестве параметров исполняемый файл принимает путь к целевому файлу, а также опциональный параметр -- keys или values
При его наличии выхлоп будет отсортирован библиотечной функцией qsort.
* Makefile написан в лучших традициях Unix -- максимально запутанно и непонятно.
* Поведение хеш-мапы параметризуется внешними функциями хеширования и сравнения. При желании можно подключить криптографический хеш :).
Эта фича активно используется как в самой программе, так и в тестах. Если передать NULL вместо функций, будут использовать дефолтные
функции хеширования и сравнения

## Внешние библиотеки
В полной соответствии с дз, целевой бинарь не использует внешних библиотек.
Тесты, однако, как и дефолтные реализации функций хеширования и сравнения, опираются на внешнюю библиотеку bstrlib.
Увы, в сях нету строк :(

