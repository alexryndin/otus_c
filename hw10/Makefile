CFLAGS=-g -O2 -Wall -Wextra -DNDEBUG $(OPTFLAGS) -pedantic
LIBS=-ldl $(OPTLIBS)
PREFIX?=/usr/local

BIN_SRC=$(wildcard *.c)
BIN=$(patsubst %.c,%,$(BIN_SRC))

LIB_SRC=$(wildcard lib/*.c)
LIB=$(patsubst %.c,%.o,$(LIB_SRC))

all: $(BIN)

dev: CFLAGS := $(filter-out -O2,$(CFLAGS))
dev: CFLAGS := $(filter-out -DNDEBUG,$(CFLAGS))
dev: CFLAGS := $(filter-out -pedantic,$(CFLAGS))
dev: all

$(BIN): $(LIB)

$(TARGET): CFLAGS += -fPIC
$(TARGET): build $(OBJECTS) $(EXTERNAL)
	ar rcs $@ $(OBJECTS) $(EXTERNAL)
	ranlib $@

tests: CFLAGS += $(TARGET)

build:
	@mkdir -p build

# The Cleaner
clean:
	rm -rf build $(OBJECTS) $(TESTS) $(BIN) $(LIB)
	rm -f tests/tests.log
	find . -name "*.gc*" -delete
