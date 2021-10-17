CFLAGS=-g -O2 -Wall -Wextra -Isrc -rdynamic -DNDEBUG $(OPTFLAGS) -pedantic

all: clean main tests

dev: CFLAGS=-g -Wall -Isrc -Wall -Wextra $(OPTFLAGS)
dev: clean main

# The Unit Tests
.PHONY: tests
tests:
	./main || true
	./main sdfsdfsdf || true
	./main cp1251.txt output-w1251.txt win1251
	./main koi8.txt output-koi8.txt koi8
	./main iso-8859-5.txt output-iso8859.txt iso8859
	diff output-iso8859.txt output-koi8.txt
	diff output-koi8.txt output-w1251.txt
	@echo Tests passed

clean:
	rm -rf main
