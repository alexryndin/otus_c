CFLAGS=-g -O2 -Wall -Wextra -Isrc -lcurl -lm -I../contrib -DNDEBUG -lpthread $(OPTFLAGS) -pedantic
LIBS=-ldl $(OPTLIBS)
PREFIX?=/usr/local
CPATH=src external bin tests

SOURCES=$(wildcard src/**/*.c src/*.c)
HEADERS=$(wildcard src/**/*.h src/*.h)
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))


BIN_SRC=$(wildcard bin/*.c)
BIN=$(patsubst %.c,%,$(BIN_SRC))

EXTERNAL_SRC=$(wildcard ../contrib/**/bstrlib.c ../contrib/json-parser/json.c)
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
