CFLAGS=-g -O2 -Wall -Wextra -Isrc -rdynamic -DNDEBUG $(OPTFLAGS) -pedantic

all: clean main tests

dev: CFLAGS=-g -Wall -Isrc -Wall -Wextra $(OPTFLAGS)
dev: clean main

# The Unit Tests
.PHONY: tests
tests: all
	./main
	./main sdfsdfsdf || true
	./main zipjpeg.jpg
	./main -o new non-zipjpeg.jpg || true
	@echo Tests passed

clean:
	rm -rf main
