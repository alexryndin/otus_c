# HW12
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
Usage: ./main message [font]
where font is one of the following:
  3-d                3x5                5lineoblique       acrobatic
  alligator          alligator2         alphabet           avatar
  banner             banner3            banner3-D          banner4
  barbwire           basic              bell               big
  bigchief           binary             block              bubble
  bulbhead           calgphy2           caligraphy         catwalk
  chunky             coinstak           colossal           computer
  contessa           contrast           cosmic             cosmike
  cricket            cursive            cyberlarge         cybermedium
  cybersmall         diamond            digital            doh
  doom               dotmatrix          drpepper           eftichess
  eftifont           eftipiti           eftirobot          eftitalic
  eftiwall           eftiwater          epic               fender
  fourtops           fuzzy              goofy              gothic
  graffiti           hollywood          invita             isometric1
  isometric2         isometric3         isometric4         italic
  ivrit              jazmine            jerusalem          katakana
  kban               larry3d            lcd                lean
  letters            linux              lockergnome        madrid
  marquee            maxfour            mike               mini
  mirror             mnemonic           morse              moscow
  nancyj             nancyj-fancy       nancyj-underlined  nipples
  ntgreek            o8                 ogre               pawp
  peaks              pebbles            pepper             poison
  puffy              pyramid            rectangles         relief
  relief2            rev                roman              rot13
  rounded            rowancap           rozzo              runic
  runyc              sblood             script             serifcap
  shadow             short              slant              slide
  slscript           small              smisome1           smkeyboard
  smscript           smshadow           smslant            smtengwar
  speed              stampatello        standard           starwars
  stellar            stop               straight           tanja
  tengwar            term               thick              thin
  threepoint         ticks              ticksslant         tinker-toy
  tombstone          trek               tsalagi            twopoint
  univers            usaflag            wavy               weird
```
# Особенности
Старался сделать код как можно короче, но вышло как всегда :)

В любом случае удалось, как минимум, не прибегать к использований всяких
`select` и `epoll`, вместо этого взаимодействие с сервером ограничивается
таймаутом в 10 секунд.
# Вопросы
Valgring показывает, что есть странные утечки, приложил выхлоп в valgrind.out
По всей видимости во время работы идёт обращение к некой разделяемой
библиотеке, что и приводит к подвишей ссылке на неё в юзерспейсе.
