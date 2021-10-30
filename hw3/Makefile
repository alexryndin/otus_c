CFLAGS=-g -O2 -Wall -Wextra -Isrc -Iexternal -DNDEBUG $(OPTFLAGS) -pedantic
LIBS=-ldl $(OPTLIBS)
PREFIX?=/usr/local
CPATH=src external bin tests

SOURCES=$(wildcard src/**/*.c src/*.c)
HEADERS=$(wildcard src/**/*.h src/*.h)
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))

TEST_SRC=$(wildcard tests/*_tests.c)
TESTS=$(patsubst %.c,%,$(TEST_SRC))

BIN_SRC=$(wildcard bin/*.c)
BIN=$(patsubst %.c,%,$(BIN_SRC))

EXTERNAL_SRC=$(wildcard external/**/bstrlib.c)
EXTERNAL=$(patsubst %.c,%.o,$(EXTERNAL_SRC))

TARGET=build/lib.a
SO_TARGET=$(patsubst %.a,%.so,$(TARGET))

all: $(TARGET) $(SO_TARGET) $(BIN) $(EXTERNAL) tests 

dev: CFLAGS=-g -Wall -Isrc -Iexternal -Wall -Wextra $(OPTFLAGS)
dev: all

$(TESTS): $(TARGET) $(SO_TARGET)

$(BIN): $(TARGET)

$(TARGET): CFLAGS += -fPIC
$(TARGET): build $(OBJECTS) $(EXTERNAL)
	ar rcs $@ $(OBJECTS) $(EXTERNAL)
	ranlib $@

tests: CFLAGS += $(TARGET)
$(SO_TARGET): $(TARGET) $(OBJECTS)
	$(CC) $(LDFLAGS) -shared -o $@ $(OBJECTS) $(EXTERNAL)

build:
	@mkdir -p build
	@mkdir -p bin

# The Unit Tests
.PHONY: tests
tests: CFLAGS += $(TARGET)
tests: CFLAGS := $(filter-out -pedantic,$(CFLAGS))
tests: CFLAGS := $(filter-out -DNDEBUG,$(CFLAGS))
tests: $(TESTS)
	sh ./tests/runtests.sh
	./bin/main test
	./bin/main test keys
	./bin/main test values
	./bin/main test valusssse || true
	./bin/main tessdfsft valusssse || true

# The Cleaner
clean:
	rm -rf build $(OBJECTS) $(TESTS)
	rm -f tests/tests.log
	find . -name "*.gc*" -delete

# The Install
install: all
	install -d $(DESTDIR)/$(PREFIX)/lib/
	install $(TARGET) $(DESTDIR)/$(PREFIX)/lib/


#The Checker
check:
	@echo Files with potentially dangerous functions.
	@egrep '[^.>a-zA-Z0-9](str(n?cpy|n?cat|xfrm|n?dup|str|pbrk|tok|_)|\
		stpn?cpy|a?sn?printf|byte_)' $(SOURCES) || true
