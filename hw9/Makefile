CFLAGS=-g -O2 -Wall -Wextra -Isrc -lcurl -lm -isystem../contrib  -DNDEBUG $(OPTFLAGS) -pedantic
LIBS=-ldl $(OPTLIBS)
PREFIX?=/usr/local
CPATH=src external bin tests

BIN_SRC=$(wildcard *.c)
BIN=$(patsubst %.c,%,$(BIN_SRC))

EXTERNAL_SRC=$(wildcard ../contrib/**/bstring/bstrlib.c ../contrib/json-parser/json.c)
EXTERNAL=$(patsubst %.c,%.o,$(EXTERNAL_SRC))

TARGET=build/lib.a

all: $(TARGET) $(BIN) $(EXTERNAL)

dev: CFLAGS := $(filter-out -O2,$(CFLAGS))
dev: CFLAGS := $(filter-out -DNDEBUG,$(CFLAGS))
dev: CFLAGS := $(filter-out -pedantic,$(CFLAGS))
dev: all

$(BIN): $(TARGET)

$(TARGET): CFLAGS += -fPIC
$(TARGET): build $(OBJECTS) $(EXTERNAL)
	ar rcs $@ $(OBJECTS) $(EXTERNAL)
	ranlib $@

tests: CFLAGS += $(TARGET)

build:
	@mkdir -p build

# The Cleaner
clean:
	rm -rf build $(OBJECTS) $(TESTS) $(BIN)
	rm -f tests/tests.log
	find . -name "*.gc*" -delete
