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
./bin/weather 'new yo'
I hope you was looking for the City of New York, woeid 2459115
Weather: Showers
Wind compass: WSW
Min temp: 3.810000
Max temp: 9.550000
Temp: 7.635000
Wind speed: 6.379943
Wind direction: 240.987967
Air pressure: 1014.000000
Humidity: 60
Visibility: 16.654923
Predictability: 73
```

## Непонятки
Valgrind показывает, что утечек нет, но при этом часть памяти всё равно доступна
По видимому, это некие библиотечки, подгружаемые libcurl, но почему они по-прежнему
доступны? Ведь я делаю `curl_easy_cleanup` в конце.
