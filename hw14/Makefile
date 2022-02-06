CFLAGS=-g -O2 -Wall -Wextra -Wstrict-prototypes -Wmissing-prototypes -pedantic  -DNDEBUG -I/usr/lib -L/usr/lib -I../contrib -Ilib -isystem../contrib/chan/src -pthread $(OPTFLAGS)
LDLIBS=-ldl -lpq -lsqlite3 -pedantic  $(OPTLIBS)
PREFIX?=/usr/local

BIN_SRC=$(wildcard *.c)
BIN=$(patsubst %.c,%,$(BIN_SRC))

TEST_SRC=$(wildcard tests/*_tests.c)
TESTS=$(patsubst %.c,%,$(TEST_SRC))

LIB_SRC=$(wildcard lib/*.c)
LIB=$(patsubst %.c,%.o,$(LIB_SRC))

SHARED_SRC=$(wildcard ../contrib/sqlite3-extension-functions/*.c)
SHARED=$(patsubst %.c,%.so,$(SHARED_SRC))

EXTERNAL_SRC=$(wildcard ../contrib/**/bstring/bstrlib.c ../contrib/**/src/*.c)
EXTERNAL_SRC_NO_TESTS=$(filter-out %test.c, $(EXTERNAL_SRC))
EXTERNAL=$(patsubst %.c,%.o,$(EXTERNAL_SRC_NO_TESTS))

all: $(BIN) $(SHARED) tests
	cp $(SHARED) .

dev: CFLAGS := $(filter-out -O2,$(CFLAGS))
dev: CFLAGS := $(filter-out -DNDEBUG,$(CFLAGS))
dev: CFLAGS := $(filter-out -pedantic,$(CFLAGS))
dev: all

$(BIN): $(LIB) $(EXTERNAL)

$(TESTS): $(LIB) $(EXTERNAL)

$(SHARED): $(SHARED_SRC)
	$(CC) -fPIC $(LDFLAGS) -shared -o $@ $(SHARED_SRC)

$(TARGET): CFLAGS += -fPIC
$(TARGET): build $(OBJECTS) $(EXTERNAL)
	ar rcs $@ $(OBJECTS) $(EXTERNAL)
	ranlib $@

# The Unit Tests
.PHONY: tests
tests: CFLAGS += $(TARGET)
tests: CFLAGS := $(filter-out -pedantic,$(CFLAGS))
tests: CFLAGS := $(filter-out -DNDEBUG,$(CFLAGS))
tests: $(TESTS)
	sh ./tests/runtests.sh


build:
	@mkdir -p build

# The Cleaner
clean:
	@echo $(EXTERNAL_SRC_NO_TESTS)
	rm -rf build $(OBJECTS) $(TESTS) $(BIN) $(LIB) $(EXTERNAL)
	rm -f tests/tests.log
	find . -name "*.gc*" -delete
