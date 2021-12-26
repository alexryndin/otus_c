AS = nasm  # NOTE: yasm тоже подойдёт

all: asm-prog mainc run

asm-prog: main.o
	$(CC) -no-pie $^ -o $@

mainc: main.c
	$(CC) $^ -o $@

main.o: main.asm
	$(AS) -felf64 $^

clean:
	rm -f asm-prog *.o core mainc

run:
	@echo "Running original prog:"
	./asm-prog
	@echo "Running homework prog:"
	./mainc
	@echo "Running modified prog with valgrind:"
	valgrind ./asm-prog

PHONY: clean
