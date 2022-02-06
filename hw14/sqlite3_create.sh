#!/bin/bash

[[ ! -f "test.db" ]] && sqlite3 test.db -init oscar.sql
