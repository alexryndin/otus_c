# HW 5
## Требования к сборке
Наличие libcurl в системе.
## Как собрать
```
make
```
## Как собрать с дебагом
```
make dev
```
## Как запускать
```./bin/weather query
query -- название города или часть названия
```
## Пример
```
./weather 'w yo'
I hope you was looking for the City of New York, woeid 2459115
Weather:        Showers
Wind compass:   WSW
Min temp:       4.75°C
Max temp:       9.55°C
Temp:   7.38°C
Wind speed:     7.449977 km/h
Wind direction: 241°
Air pressure:   760 mmhq
Humidity:       63%
Visibility:     20 km
Predictability: 73%
```

## Непонятки
Valgrind показывает, что утечек нет, но при этом часть памяти всё равно доступна
По видимому, это некие библиотечки, подгружаемые libcurl, но почему они по-прежнему
доступны? Ведь я делаю `curl_easy_cleanup` в конце.
