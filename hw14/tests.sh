#!/bin/bash

./sqlite3_create.sh
./main postgresql postgres://postgres:qwerty@pg public.oscar year
./main sqlite3 test.db oscar year
